#ifndef AGC_MEMORY_H
#define AGC_MEMORY_H

AGC_DECLARE(void) agc_memory_pool_tag(agc_memory_pool_t *pool, const char *tag);

AGC_DECLARE(agc_status_t) agc_memory_create_pool(agc_memory_pool_t **pool);

AGC_DECLARE(agc_status_t) agc_memory_destroy_pool(agc_memory_pool_t **pool);

AGC_DECLARE(void *) agc_memory_alloc(agc_memory_pool_t *pool, agc_size_t memory);

AGC_DECLARE(void) agc_memory_pool_set_data(agc_memory_pool_t *pool, const char *key, void *data);

AGC_DECLARE(void *) agc_memory_pool_get_data(agc_memory_pool_t *pool, const char *key);

AGC_DECLARE(void *) agc_memory_permanent_alloc(agc_size_t memsize);

#endif