#include <agc.h>

static agc_cache_actions_t *cache_actions = NULL;

AGC_DECLARE(agc_status_t) agc_cache_init(agc_memory_pool_t *pool)
{
	 agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Cache init success.\n");
}

AGC_DECLARE(agc_status_t) agc_cache_shutdown(void)
{
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Cache shutdown success.\n")
}

AGC_DECLARE(void) agc_cache_register_impl(agc_cache_actions_t *actions)
{
	if (!actions)
		return;
	
	cache_actions = actions;
}

AGC_DECLARE(agc_status_t) agc_cache_set(const char *key, const char *value, int size, int expires)
{
	if (!cache_actions || !cache_actions->agc_cach_set_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cach_set_func(key, value, size, expires);
}

AGC_DECLARE(agc_status_t) agc_cache_get(const char *key, char **result, int *len)
{
	if (!cache_actions || !cache_actions->agc_cache_get_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_get_func(key, result, len);
}

AGC_DECLARE(agc_status_t) agc_cache_delete(const char *key)
{
	if (!cache_actions || !cache_actions->agc_cache_delete_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_delete_func(key);
}

AGC_DECLARE(agc_status_t) agc_cache_set_pipeline(keyvalues_t *keyvalues)
{
	if (!cache_actions || !cache_actions->agc_cache_set_pipeline_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_set_pipeline_func(keyvalues);
}

AGC_DECLARE(agc_status_t) agc_cache_get_pipeline(keys_t *keys, keyvalues_t **keyvalues)
{
	if (!cache_actions || !cache_actions->agc_cache_get_pipeline_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_get_pipeline_func(keys, keyvalues);
}

AGC_DECLARE(agc_status_t) agc_cache_delete_pipeline(keys_t *keys)
{
	if (!cache_actions || !cache_actions->agc_cache_delete_pipeline_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_delete_pipeline_func(keys);
}

AGC_DECLARE(agc_status_t) agc_cache_expire(const char *key, int expires)
{
	if (!cache_actions || !cache_actions->agc_cache_expire_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_expire_func(key, expires);
}

AGC_DECLARE(agc_status_t) agc_cache_incr(const char *key, int *value)
{
	if (!cache_actions || !cache_actions->agc_cache_incr_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_incr_func(key, value);
}

AGC_DECLARE(agc_status_t) agc_cache_decr(const char *key, int *value)
{
	if (!cache_actions || !cache_actions->agc_cache_decr_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_decr_func(key, value);
}

AGC_DECLARE(agc_status_t) agc_cache_hashset(const char *tablename, const char *key, const char *value)
{
	if (!cache_actions || !cache_actions->agc_cache_hashset_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_hashset_func(tablename, key, value);
}

AGC_DECLARE(agc_status_t) agc_cache_hashget(const char *tablename, const char *key, char **value, int *len)
{
	if (!cache_actions || !cache_actions->agc_cache_hashget_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_hashget_func(tablename, key, value, len);
}

AGC_DECLARE(agc_status_t) agc_cache_hashmset(const char *tablename, keyvalues_t *keyvalues)
{
	if (!cache_actions || !cache_actions->agc_cache_hashmset_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_hashmset_func(tablename, keyvalues);
}

AGC_DECLARE(agc_status_t) agc_cache_hashmget(const char *tablename, keys_t *keys,  keyvalues_t *keyvalues)
{
	if (!cache_actions || !cache_actions->agc_cache_hashmget_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_hashmget_func(tablename, keys, keyvalues);
}

AGC_DECLARE(agc_status_t) agc_cache_hashdel(const char *tablename, keys_t *keys)
{
	if (!cache_actions || !cache_actions->agc_cache_hashdel_func)
		return AGC_STATUS_GENERR;

	return cache_actions->agc_cache_hashdel_func(tablename, keys);
}

AGC_DECLARE(void)  agc_cache_free_keyvalues(keyvalues_t *keyvalues)
{
	keyvalues_t *iter = keyvalues;
	keyvalues_t *current = NULL;

	while (iter) {
		current = iter;
		iter = iter->next;

		agc_safe_free(current->key);
		agc_safe_free(current->value);
		agc_safe_free(current);
	}
}

AGC_DECLARE(void) agc_cache_free_keys(keys_t *keys)
{
	keys_t *iter = keys;
	keys_t *current = NULL;	

	while (iter) {
		current = iter;
		iter = iter->next;

		agc_safe_free(current->key);
		agc_safe_free(current);
	}	
}

