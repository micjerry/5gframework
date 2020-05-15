#include "mod_rabbitmq.h"

agc_status_t agcmq_consumer_create(char *name, agcmq_connection_info_t *conn_infos, agcmq_conn_parameter_t *parameters, agc_memory_pool_t *pool)
{
	agcmq_consumer_profile_t *consumer;
	agc_threadattr_t *thd_attr = NULL;

	if (!name || !conn_infos || !parameters)
		return AGC_STATUS_GENERR;

	consumer = (agcmq_consumer_profile_t *)agc_memory_alloc(pool, sizeof(*consumer));
	assert(consumer);

	consumer->name = name;
	consumer->conn_infos = conn_infos;
	consumer->conn_parameter = parameters;
	consumer->pool = pool;
	consumer->running = 1;

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

	if (consumer->conn_active) {
		agcmq_connection_close(consumer->conn_active);
	}

	consumer->conn_infos = NULL;
	consumer->conn_active = NULL;	

	if (pool) {
		agc_memory_destroy_pool(&pool);
	}

	*profile = NULL;

	return AGC_STATUS_SUCCESS;	
}
