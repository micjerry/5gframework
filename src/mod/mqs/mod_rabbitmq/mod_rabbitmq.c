#include <agc.h>

AGC_MODULE_LOAD_FUNCTION(mod_rabbitmq_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_rabbitmq_shutdown);
AGC_MODULE_DEFINITION(mod_rabbitmq, mod_rabbitmq_load, mod_rabbitmq_shutdown, NULL);

static agc_event_node_t *mq_subscribe;

static void handle_event(void *data);

static void make_routingkey(char routingKey[MAX_MQ_ROUTING_KEY_LENGTH], agc_event_t *evt);

AGC_STANDARD_API(agcmq_load)
{
	return agcmq_load_config();
}

AGC_MODULE_LOAD_FUNCTION(mod_rabbitmq_load)
{
	*module_interface = agc_loadable_module_create_interface(module_pool, modname);
	memset(&agcmq_global, 0, sizeof(agcmq_global));
	agcmq_global.pool = pool;

	agcmq_global.producer_hash = agc_hash_make(agcmq_global.pool);
	agcmq_global.consumer_hash = agc_hash_make(agcmq_global.pool);

	if (agcmq_load_config() != AGC_STATUS_SUCCESS) {
		return AGC_STATUS_GENERR;
	}

	agc_api_register("agcmq", "agcmq API", "syntax", agcmq_load);

	if (agc_event_bind_removable("agcmq", EVENT_ID_ALL, handle_event, NULL, &mq_subscribe) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "%s subscribe event failed.\n", modname);
		return AGC_STATUS_GENERR;
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Mq init success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_rabbitmq_shutdown)
{
	 agc_hash_index_t *hi;
	 void *val;
	 agcmq_producer_profile_t *producer;
	 agcmq_consumer_profile_t *consumer;

	agc_event_unbind(&mq_subscribe);
	for (hi = agc_hash_first(agcmq_global.pool, agcmq_global.producer_hash); hi; hi = agc_hash_next(hi)) {
		val = agc_hash_this_val(hi);
		producer = (agcmq_producer_profile_t *) val;
		agcmq_producer_destroy(producer);
	}

	for (hi = agc_hash_first(agcmq_global.pool, agcmq_global.consumer_hash); hi; hi = agc_hash_next(hi)) {
		val = agc_hash_this_val(hi);
		consumer = (agcmq_consumer_profile_t *) val;
		agcmq_consumer_destroy(consumer);
	}
		
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Mq  shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

static void handle_event(void *data)
{
	agc_hash_index_t *hi;
	void *val;
	agcmq_producer_profile_t *producer;
	agc_event_t *event = (agc_event_t *)data;
	agc_event_t *clone = NULL;
	agcmq_message_t *msg = NULL;
	agcmq_conn_parameter_t *para;
	agc_time_t now = agc_timer_curtime();

	for (hi = agc_hash_first(agcmq_global.pool, agcmq_global.producer_hash); hi; hi = agc_hash_next(hi))  {
		val = agc_hash_this_val(hi);
		producer = (agcmq_producer_profile_t *) val;
		if (!producer || !producer->running)
			continue;

		para = producer->conn_parameter;

		if (now < producer->reset_time) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Producer[%s] reset wait.\n", producer->name);
			continue;
		}

		if (producer->event_list[event->event_id]) {
			msg = malloc(sizeof(agcmq_message_t));
			if (!msg) {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Alloc memory failed.\n");
				return;
			}

			agc_event_serialize_json(event, &msg->pjson);
			make_routingkey(msg->routing_key, event);
			if (agc_queue_trypush(producer->send_queue, msg) != AGC_STATUS_SUCCESS) {
				producer->reset_time = now + para->circuit_breaker_ms * 1000;
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Message queue full.\n");
				agcmq_producer_msg_destroy(msg);
			}
		}
	}

}

static void make_routingkey(char routingKey[MAX_MQ_ROUTING_KEY_LENGTH], agc_event_t *evt)
{
	const char *hostname = NULL;
	int len;

	hostname = agc_core_get_hostname();
	len = strlen(hostname);
	
	memset(routingKey, 0, sizeof(routingKey));
	strcpy(routingKey, hostname);
	routingKey[len] = '.';
	len++;
	strcpy(routingKey + len, agc_event_get_name(evt->event_id));
}

