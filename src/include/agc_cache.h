#ifndef AGC_CACHE_H
#define AGC_CACHE_H

struct keys_s {
    char *key;
    keys_t *next;
};

struct keyvalues_s {
    char *key;
    char *value;
    keyvalues_t *next;
};

AGC_DECLARE(agc_status_t) agc_cache_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_cache_shutdown(void);

AGC_DECLARE(agc_status_t) agc_cache_set(const char *key, const char *value, int expires);

AGC_DECLARE(char *) agc_cache_get(const char *key);

AGC_DECLARE(agc_status_t) agc_cache_delete(const char *key);

AGC_DECLARE(agc_status_t) agc_cache_set_pipeline(keyvalues_t *keyvalues);

AGC_DECLARE(keyvalues_t *) agc_cache_get_pipeline(keys_t *keys);

AGC_DECLARE(agc_status_t) agc_cache_delete_pipeline(keys_t *keys);

AGC_DECLARE(agc_status_t) agc_cache_expire(const char *key);

AGC_DECLARE(agc_status_t) agc_cache_incr(const char *key, int *value);

AGC_DECLARE(agc_status_t) agc_cache_decr(const char *key, int *value);

AGC_DECLARE(agc_status_t) agc_cache_hashset(const char *tablename, keyvalues_t *keyvalue);

AGC_DECLARE(char *) agc_cache_hashget(const char *tablename, const char *key);

AGC_DECLARE(agc_status_t) agc_cache_hashdel(const char *tablename, const char *key);

AGC_DECLARE(keys_t *) agc_cache_hashkeys(const char *tablename);

AGC_DECLARE(keyvalues_t *) agc_cache_hashgetall(const char *tablename);

typedef struct {
    agc_status_t (*agc_cach_set_func)(const char *key, const char *value, int expires);
    char * (*agc_cache_get_func)(const char *key);
    agc_status_t (*agc_cache_delete_func)(const char *key);
    agc_status_t (*agc_cache_set_pipeline_func)(keyvalues_t *keyvalues);
    keyvalues_t * (*agc_cache_get_pipeline_func)(keys_t *keys);
    agc_status_t (*agc_cache_delete_pipeline_func)(keys_t *keys);
    agc_status_t (*agc_cache_expire_func)(const char *key);
    agc_status_t (*agc_cache_incr_func)(const char *key, int *value);
    agc_status_t (*agc_cache_decr_func)(const char *key, int *value);
    agc_status_t (*agc_cache_hashset_func)(const char *tablename, keyvalues_t *keyvalue);
    char * (*agc_cache_hashget_func)(const char *tablename, const char *key);
    agc_status_t (*agc_cache_hashdel_func)(const char *tablename, const char *key);
    keys_t * (*agc_cache_hashkeys_func)(const char *tablename);
    keyvalues_t * (*agc_cache_hashgetall_func)(const char *tablename);
} agc_cache_actions_t;

AGC_DECLARE(void) agc_cache_register_impl(agc_cache_actions_t *actions);


#endif