#include "mod_rabbitmq.h"

agc_status_t agcmq_consumer_create(char *name, agcmq_connection_info_t *conn_infos, agcmq_conn_parameter_t *parameters, agc_memory_pool_t *pool)
{
	agcmq_consumer_profile_t *consumer;
	agc_threadattr_t *thd_attr = NULL;
	agcmq_connection_t *new_conn = NULL;

	if (!name || !conn_infos || !parameters)
		return AGC_STATUS_GENERR;

	consumer = (agcmq_consumer_profile_t *)agc_memory_alloc(pool, sizeof(*consumer));
	assert(consumer);

	consumer->name = name;
	consumer->conn_infos = conn_infos;
	consumer->conn_parameter = parameters;
	consumer->pool = pool;
	consumer->running = 1;

	new_conn = agc_memory_alloc(consumer->pool , sizeof(*new_conn));
	assert(new_conn);
	consumer->mq_conn = new_conn;

	agc_threadattr_create(&thd_attr, consumer->pool);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);

	if (agc_thread_create(&consumer->consumer_thread, thd_attr, agcmq_consumer_thread, consumer, consumer->pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not create thread consumer %s create failed.\n", consumer->name);
		agcmq_consumer_destroy(&consumer);
		return AGC_STATUS_GENERR;
	}

	agc_hash_set(agcmq_global.consumer_hash, consumer->name, AGC_HASH_KEY_STRING, consumer);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] Successfully started.\n", consumer->name);
	return AGC_STATUS_SUCCESS;
}

agc_status_t agcmq_consumer_destroy(agcmq_consumer_profile_t **profile)
{
	agc_status_t ret;
	agcmq_consumer_profile_t *consumer;
	agc_memory_pool_t *pool;

	if (!profile || !*profile) {
		return AGC_STATUS_SUCCESS;
	}

	consumer = *profile;
	pool = consumer->pool;

	if (consumer->name) {
		agc_hash_set(agcmq_global.consumer_hash, consumer->name, AGC_HASH_KEY_STRING, NULL);
	}

	consumer->running = 0;

	if (consumer->consumer_thread) {
		agc_thread_join(&ret, consumer->consumer_thread);
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] Closing.\n", consumer->name);

	if (agcmq_is_conn_active(consumer->mq_conn)) {
		agcmq_connection_close(consumer->mq_conn);
	}

	consumer->conn_infos = NULL;

	if (pool) {
		agc_memory_destroy_pool(&pool);
	}

	*profile = NULL;

	return AGC_STATUS_SUCCESS;	
}

void *agcmq_consumer_thread(agc_thread_t *thread, void *data)
{
	agcmq_consumer_profile_t *consumer = (agcmq_consumer_profile_t *)data;
	agc_status_t status =AGC_STATUS_SUCCESS;
	agcmq_conn_parameter_t *para;
	amqp_queue_declare_ok_t *recv_queue;
	amqp_bytes_t queueName = { 0, NULL };

	para = consumer->conn_parameter;
	
	while (consumer->running) {
		if (!agcmq_is_conn_active(consumer->mq_conn)) {
			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] reconnecting.\n", consumer->name);

			status = agcmq_connection_open(consumer->conn_infos, consumer->mq_conn, consumer->name);

			if (status != AGC_STATUS_SUCCESS) {
				agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Consumer[%s] failed to connect.\n", consumer->name);
				agc_sleep(para->reconnect_interval_ms* 1000);
				continue;
			}

			amqp_exchange_declare(consumer->mq_conn->state, 1,
								  amqp_cstring_bytes(para->ex_name),
								  amqp_cstring_bytes("topic"),
								  0, //passive
								  1, //durable
								  0, //auto delete
								  0, //internal
								  amqp_empty_table);
			
			if (agcmq_parse_amqp_reply(amqp_get_rpc_reply(consumer->mq_conn->state), "Declaring exchange")) {
				agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Consumer[%s] declaring exchange failed.\n", consumer->name);
				agc_sleep(para->reconnect_interval_ms* 1000);
				continue;
			}

			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] reconnect success.\n", consumer->name);

			recv_queue = amqp_queue_declare(consumer->mq_conn->state, // state
										1,                           // channel
										amqp_cstring_bytes(para->queuename), // queue name amqp_empty_bytes
										0, 0,                        // passive, durable
										0, 1,                        // exclusive, auto-delete
										amqp_empty_table);
			if (agcmq_parse_amqp_reply(amqp_get_rpc_reply(consumer->mq_conn->state), "Declaring queue"))  {
				agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Consumer[%s] declaring queue failed.\n", consumer->name);
				agc_sleep(para->reconnect_interval_ms* 1000);
				continue;
			}

			if (queueName.bytes) {
				amqp_bytes_free(queueName);
			}

			queueName = amqp_bytes_malloc_dup(recv_queue->queue);

			if (!queueName.bytes) {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Consumer[%s] out of memory when copying queue name.\n", consumer->name);
				break;
			}

			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] Binding command queue to exchange %s.\n", consumer->name, para->ex_name);

			amqp_queue_bind(consumer->mq_conn->state,                                             // state
						1,                                                         // channel
						queueName,                                          // queue
						amqp_cstring_bytes(para->ex_name),      // exchange
						amqp_cstring_bytes(para->bind_key),      // routing key
						amqp_empty_table);       

			if (agcmq_parse_amqp_reply(amqp_get_rpc_reply(consumer->mq_conn->state), "Binding queue"))  {
				agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Consumer[%s] binding queue failed.\n", consumer->name);
				agcmq_connection_close(consumer->mq_conn);
				agc_sleep(para->reconnect_interval_ms* 1000);
				continue;
			}

			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] Amqp reconnect successful- connected.\n", consumer->name);
			continue;		
		}
		
		amqp_basic_consume(consumer->mq_conn->state,     // state
						   1,                               // channel
						   queueName,                       // queue
						   amqp_empty_bytes,                // command tag
						   0, 1, 0,                         // no_local, no_ack, exclusive
						   amqp_empty_table);               // args
						   
		if (agcmq_parse_amqp_reply(amqp_get_rpc_reply(consumer->mq_conn->state), "Consuming command"))  {
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Consumer[%s] consuming command failed.\n", consumer->name);
			agcmq_connection_close(consumer->mq_conn);
			agc_sleep(para->reconnect_interval_ms* 1000);
			continue;
		}

		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] consume successful.\n", consumer->name);

		while (consumer->running && agcmq_is_conn_active(consumer->mq_conn)) {
			amqp_rpc_reply_t res;
			amqp_envelope_t envelope;
			char command[1024];
			enum ECommandFormat {
				COMMAND_FORMAT_UNKNOWN,
				COMMAND_FORMAT_PLAINTEXT,
				COMMAND_FORMAT_JSONTEXT
			} cmdfmt = COMMAND_FORMAT_PLAINTEXT;

			amqp_maybe_release_buffers(consumer->mq_conn->state);

			res = amqp_consume_message(consumer->mq_conn->state, &envelope, NULL, 0);
			
			if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
				if (res.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
					/* Unexpected frame. Discard it then continue */
					amqp_frame_t decoded_frame;
					amqp_simple_wait_frame(consumer->mq_conn->state, &decoded_frame);
				}

				if (res.library_error == AMQP_STATUS_SOCKET_ERROR) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Consumer[%s] A socket error occurred. Tearing down and reconnecting.\n", consumer->name);
					break;
				}

				if (res.library_error == AMQP_STATUS_CONNECTION_CLOSED) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Consumer[%s] AMQP connection was closed. Tearing down and reconnecting.\n", consumer->name);
					break;
				}

				if (res.library_error == AMQP_STATUS_TCP_ERROR) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Consumer[%s] A TCP error occurred. Tearing down and reconnecting.\n", consumer->name);
					break;
				}

				continue;
			}

			if (res.reply_type != AMQP_RESPONSE_NORMAL) {
				break;
			}

			if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
				if (strncasecmp("text/plain", envelope.message.properties.content_type.bytes, strlen("text/plain")) == 0) {
					cmdfmt = COMMAND_FORMAT_PLAINTEXT;
				} else if (strncasecmp(MQ_DEFAULT_CONTENT_TYPE, envelope.message.properties.content_type.bytes, strlen(MQ_DEFAULT_CONTENT_TYPE)) == 0) {
					cmdfmt = COMMAND_FORMAT_JSONTEXT;
				} else {
					cmdfmt = COMMAND_FORMAT_UNKNOWN;
				}
			}

			if (cmdfmt == COMMAND_FORMAT_PLAINTEXT) {
				snprintf(command, sizeof(command), "%.*s", (int) envelope.message.body.len, (char *) envelope.message.body.bytes);

				agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] execute command %s.\n", consumer->name, command);

				agc_parse_execute(command);
			}

			if (cmdfmt = COMMAND_FORMAT_JSONTEXT) {
				agc_event_t *event = NULL;
				if (agc_event_create_json(&event, (const char *) envelope.message.body.bytes) == AGC_STATUS_SUCCESS) {
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Consumer[%s] fire event %d.\n", consumer->name, event->event_id);
					agc_event_fire(&event);
				}
			}

			amqp_destroy_envelope(&envelope);
		}

		amqp_bytes_free(queueName);
		queueName.bytes = NULL;

		agcmq_connection_close(consumer->mq_conn);

		if (consumer->running) {
			agc_yield(1000000);
		}
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Consumer[%s] stopped.\n", consumer->name);
	agc_thread_exit(thread, AGC_STATUS_SUCCESS);
	return NULL;
}

