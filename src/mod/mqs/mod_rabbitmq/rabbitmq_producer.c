#include "mod_rabbitmq.h"

#define MQ_EVENT_LIMIT 10000

static void add_producer(agcmq_producer_profile_t *producer);

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
	int arg, event_id, x;
	agc_threadattr_t *thd_attr = NULL;
	agcmq_connection_t *new_conn;

	if (!name || !conn_infos || !parameters)
		return AGC_STATUS_GENERR;

	producer = (agcmq_producer_profile_t *)agc_memory_alloc(pool, sizeof(*producer));
	assert(producer);

	producer->name = name;
	producer->conn_infos = conn_infos;
	producer->conn_parameter = parameters;
	producer->pool = pool;
	producer->running = 1;

	new_conn = agc_memory_alloc(producer->pool, sizeof(*new_conn));
	assert(new_conn);
	producer->mq_conn = new_conn;
	
	event_numbers = agc_separate_string(parameters->event_filter, ',', argv, (sizeof(argv) / sizeof(argv[0])));

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Create event subs for %s event_filter.\n", producer->name, parameters->event_filter);
	for (arg = 0; arg < event_numbers; arg++) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] subs event %s.\n", argv[arg]);
		if (agc_event_get_id(argv[arg], &event_id) == AGC_STATUS_SUCCESS) {
			if (event_id == EVENT_ID_ALL) {
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] subs all event.\n", producer->name);
				for (x = 0; x < EVENT_ID_LIMIT; x++) {
					producer->event_list[x] = 1;
				}
			} else {
				producer->event_list[event_id] = 1;
			}
		}
	}

	if (agcmq_connection_open(producer->conn_infos, producer->mq_conn, producer->name) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not connect to mq producer %s create failed.\n", producer->name);
		agcmq_producer_destroy(&producer);
		return AGC_STATUS_GENERR;
	}

	amqp_exchange_declare(producer->mq_conn->state, 1,
						  amqp_cstring_bytes(parameters->ex_name),
						  amqp_cstring_bytes(parameters->ex_type),
						  0,  //passive
						  1,  //durable
						  0, //auto delete
					  	  0, //internal
						  amqp_empty_table);
	if (agcmq_parse_amqp_reply(amqp_get_rpc_reply(producer->mq_conn->state), "Declaring exchange")) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Producer[%s] exchange name %s type %s create failed.\n", producer->name, parameters->ex_name, parameters->ex_type);
		agcmq_producer_destroy(&producer);
		return AGC_STATUS_GENERR;
	}

	agc_queue_create(&producer->send_queue, MQ_EVENT_LIMIT, pool);

	agc_threadattr_create(&thd_attr, producer->pool);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);

	if (agc_thread_create(&producer->producer_thread, thd_attr, agcmq_producer_thread, producer, producer->pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Can not create thread producer %s create failed.\n", producer->name);
		agcmq_producer_destroy(&producer);
		return AGC_STATUS_GENERR;
	}

	add_producer(producer);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] Successfully started.\n", producer->name);
	return AGC_STATUS_SUCCESS;
}

agc_status_t agcmq_producer_destroy(agcmq_producer_profile_t **profile)
{
	agcmq_producer_profile_t *producer;
	agcmq_message_t *msg = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;

	if (!profile || !*profile) {
		return AGC_STATUS_SUCCESS;
	}

	producer = *profile;
	
	producer->running = 0;

	if (producer->producer_thread) {
		agc_thread_join(&status, producer->producer_thread);
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] Closing.\n", producer->name);

	if (agcmq_is_conn_active(producer->mq_conn)) {
		agcmq_connection_close(producer->mq_conn);
	}

	producer->conn_infos = NULL;

	while (producer->send_queue && agc_queue_trypop(producer->send_queue, (void**)&msg) == AGC_STATUS_SUCCESS) {
		agcmq_producer_msg_destroy(&msg);
	}

	*profile = NULL;

	return AGC_STATUS_SUCCESS;
}

void *agcmq_producer_thread(agc_thread_t *thread, void *data)
{
	agcmq_message_t *msg = NULL;
	agcmq_producer_profile_t *producer;
	agc_status_t status =AGC_STATUS_SUCCESS;
	agcmq_conn_parameter_t *para;

	producer = (agcmq_producer_profile_t *)data;

	while (producer->running) {
		if (!agcmq_is_conn_active(producer->mq_conn)) {
			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Producer[%s] reconnecting.\n", producer->name);

			status = agcmq_connection_open(producer->conn_infos, producer->mq_conn, producer->name);
			if ( status== AGC_STATUS_SUCCESS ) {
				para = producer->conn_parameter;
				amqp_exchange_declare(producer->mq_conn->state, 1,
									  amqp_cstring_bytes(para->ex_name),
									  amqp_cstring_bytes(para->ex_type),
									  0,  //passive
									  1,  //durable
									  0, //auto delete
								  	  0, //internal
									  amqp_empty_table);
				if (!agcmq_parse_amqp_reply(amqp_get_rpc_reply(producer->mq_conn->state), "Declaring exchange")) {
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
					agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Producer[%s] send event %s success.\n", producer->name, msg->routing_key);
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

agc_status_t agcmq_producer_send(agcmq_producer_profile_t *producer, agcmq_message_t *msg)
{
	agcmq_conn_parameter_t *para;
	amqp_table_entry_t messageTableEntries[1];
	amqp_basic_properties_t props;
	int status;
	
	if (!agcmq_is_conn_active(producer->mq_conn)) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Producer[%s] not active.\n", producer->name);
		return AGC_STATUS_NOT_INITALIZED;
	}
	
	para = producer->conn_parameter;

	memset(&props, 0, sizeof(amqp_basic_properties_t));

	props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
	props.content_type = amqp_cstring_bytes(MQ_DEFAULT_CONTENT_TYPE);

	if(para->delivery_mode > 0) {
		props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.delivery_mode = para->delivery_mode;
	}

	if(para->delivery_timestamp) {
		props._flags |= AMQP_BASIC_TIMESTAMP_FLAG | AMQP_BASIC_HEADERS_FLAG;
		props.timestamp = (uint64_t)time(NULL);
		props.headers.num_entries = 1;
		props.headers.entries = messageTableEntries;
		messageTableEntries[0].key = amqp_cstring_bytes("x_Liquid_MessageSentTimeStamp");
		messageTableEntries[0].value.kind = AMQP_FIELD_KIND_TIMESTAMP;
		messageTableEntries[0].value.value.u64 = (uint64_t)agc_timer_curtime();
	}

	status = amqp_basic_publish(producer->mq_conn->state,
								1,
								amqp_cstring_bytes(para->ex_name),
								amqp_cstring_bytes(msg->routing_key),
								0,
								0,
								&props,
								amqp_cstring_bytes(msg->pjson));

	if (status < 0) {
		const char *errstr = amqp_error_string2(-status);
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Producer[%s] send event failed %s.\n", producer->name, errstr);
		agcmq_connection_close(producer->mq_conn);
		return AGC_STATUS_SOCKERR;
	}

	return AGC_STATUS_SUCCESS;
}

static void add_producer(agcmq_producer_profile_t *producer)
{
	if (agcmq_global.last_producer) {
		agcmq_global.last_producer->next = producer;
	} else {
		agcmq_global.producers = producer;
	}

	agcmq_global.last_producer = producer;
}

