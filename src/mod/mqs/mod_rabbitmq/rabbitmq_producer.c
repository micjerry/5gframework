#include "mod_rabbitmq.h"

void agcmq_producer_msg_destroy(agcmq_message_t **msg)
{
	if (!msg || !*msg) return;
	agc_safe_free((*msg)->pjson);
	agc_safe_free(*msg);
}

agc_status_t agcmq_producer_create(char *name, agcmq_connection_info_t *conn_infos, agcmq_conn_parameter_t *parameters, agc_memory_pool_t *pool)
{
	agcmq_producer_profile_t *producer;
	int event_numbers = 0;
	char  *argv[EVENT_ID_LIMIT];
	int arg, event_id;
	agc_threadattr_t *thd_attr = NULL;

	if (!name || !conn_infos || !parameters)
		return AGC_STATUS_GENERR;

	producer = (agcmq_producer_profile_t *)agc_memory_alloc(pool, sizeof(*producer));
	assert(producer);

	producer->name = name;
	producer->conn_infos = conn_infos;
	producer->conn_parameter = parameters;
	producer->pool = pool;
	producer->running = 1;

	event_numbers = agc_separate_string(parameters->event_filter, ',', argv, (sizeof(argv) / sizeof(argv[0])));

	for (arg = 0; arg < event_numbers; arg++) {
		if (agc_event_get_id(argv[arg], &event_id) == AGC_STATUS_SUCCESS) {
			if (event_id == EVENT_ID_ALL) {
				for (x = 0; x < EVENT_ID_LIMIT; x++) {
					producer->event_list[x] = 1;
				}
			} else {
				producer->event_list[event_id] = 1;
			}
		}
	}

	if (agcmq_connection_open(producer->conn_infos, &producer->conn_active, producer->name) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not connect to mq producer %s create failed.\n", producer->name);
		agcmq_producer_destroy(&producer);
		return AGC_STATUS_GENERR;
	}

	agc_threadattr_create(&thd_attr, producer->pool);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);

	if (agc_thread_create(&producer->producer_thread, thd_attr, agcmq_producer_thread, producer, producer->pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not create thread producer %s create failed.\n", producer->name);
		agcmq_producer_destroy(&producer);
		return AGC_STATUS_GENERR;
	}

	agc_hash_set(agcmq_global.producer_hash, producer->name, AGC_HASH_KEY_STRING, producer);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] Successfully started.\n", producer->name);
	return AGC_STATUS_SUCCESS;
}

agc_status_t agcmq_producer_destroy(agcmq_producer_profile_t **profile)
{
	agcmq_producer_profile_t *producer;
	agcmq_message_t *msg = NULL;
	agc_memory_pool_t *pool;
	agc_status_t status = AGC_STATUS_SUCCESS;

	if (!profile || !*profile) {
		return AGC_STATUS_SUCCESS;
	}

	producer = *profile;
	pool = producer->pool;

	if (producer->name) {
		agc_hash_set(agcmq_global.producer_hash, producer->name, AGC_HASH_KEY_STRING, NULL);
	}

	producer->running = 0;

	if (producer->producer_thread) {
		agc_thread_join(&status, producer->producer_thread);
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] Closing.\n", producer->name);

	if (producer->conn_active) {
		agcmq_connection_close(producer->conn_active);
	}

	producer->conn_infos = NULL;
	producer->conn_active = NULL;

	while (producer->send_queue && agc_queue_trypop(producer->send_queue, (void**)&msg) == AGC_STATUS_SUCCESS) {
		agcmq_producer_msg_destroy(&msg);
	}

	if (pool) {
		agc_memory_destroy_pool(&pool);
	}

	*profile = NULL;

	return AGC_STATUS_SUCCESS;
}

void *agcmq_producer_thread(agc_thread_t *thread, void *data)
{
	agcmq_message_t *msg = NULL;
	agcmq_producer_profile_t *producer;
	agc_status_t status =AGC_STATUS_SUCCESS;
	amqp_connection_state_t state;
	agcmq_conn_parameter_t *para;

	producer = (agcmq_producer_profile_t *)data;

	while (producer->running) {
		if (!producer->conn_active) {
			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] reconnectiong.\n", producer->name);

			status = agcmq_connection_open(producer->conn_infos, &producer->conn_active, producer->name);
			if ( status== AGC_STATUS_SUCCESS ) {
				state = *(producer->conn_active);
				para = producer->conn_parameter;
				amqp_exchange_declare(state, 1,
									  amqp_cstring_bytes(para->ex_name),
									  amqp_cstring_bytes(para->ex_type),
									  0,  //passive
									  1,  //durable
									  amqp_empty_table);
				if (!agcmq_parse_amqp_reply(amqp_get_rpc_reply(state), "Declaring exchange")) {
					agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] reconnect success.\n", producer->name);
					continue;
				}

				agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Producer[%s] reconnect failed.\n", producer->name);
				agc_sleep(para->reconnect_interval_ms* 1000);
				continue;
			}
		}

		if (!msg && agc_queue_trypop(producer->send_queue, (void **)&msg) != AGC_STATUS_SUCCESS) {
			agc_yield(100000);
			continue;
		}

		if (msg) {
			switch (agcmq_producer_send(producer, msg)) {
				case AGC_STATUS_SUCCESS:
					agcmq_producer_msg_destroy(&msg);
					break;
				case AGC_STATUS_NOT_INITALIZED:
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] send failed with not initialised.\n", producer->name);
					break;
				case AGC_STATUS_SOCKERR:
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] send failed with socket eror.\n", producer->name);
					break;
				default:
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] send failed with generic eror.\n", producer->name);
					break;
			}
		}
	}

	agcmq_producer_msg_destroy(&msg);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] Event sender thread stopped.\n", producer->name);
	agc_thread_exit(thread, AGC_STATUS_SUCCESS);
	return NULL;
}

