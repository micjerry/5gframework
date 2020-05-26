#include <agc.h>
#include "private/agc_core_pvt.h"

static struct {
	agc_queue_t *pool_queue;
	agc_queue_t *pool_recycle_queue;
	agc_memory_pool_t *memory_pool;
	int pool_thread_running;
} memory_manager;

static agc_thread_t *pool_thread_p = NULL;

static void *pool_thread(agc_thread_t *thread, void *obj)
{
	memory_manager.pool_thread_running = 1;
	while (memory_manager.pool_thread_running == 1) {
		int len = agc_queue_size(memory_manager.pool_queue);
		if (len) {
			int x = len, done = 0;
			agc_yield(1000000);
			while (x > 0) { 
				void *pop = NULL;
				if (agc_queue_pop(memory_manager.pool_queue, &pop) != AGC_STATUS_SUCCESS || !pop) {
					done = 1;
					break;
				}
				apr_pool_destroy(pop);
				x--;
			}
            
			if (done) {
				break;
			}
		} else {
			agc_yield(1000000);
        	}
    }
    
	void *pop = NULL;
	while (agc_queue_trypop(memory_manager.pool_queue, &pop) == AGC_STATUS_SUCCESS && pop) {
		apr_pool_destroy(pop);
		pop = NULL;
	}
                
    memory_manager.pool_thread_running = 0;
    
    return NULL;
}

agc_memory_pool_t *agc_core_memory_init(void)
{
	agc_threadattr_t *thd_attr;
	apr_allocator_t *my_allocator = NULL;
	apr_thread_mutex_t *my_mutex;

	memset(&memory_manager, 0, sizeof(memory_manager));
    
	if ((apr_allocator_create(&my_allocator)) != APR_SUCCESS) {
		abort();
	}
    
	if ((apr_pool_create_ex(&memory_manager.memory_pool, NULL, NULL, my_allocator)) != APR_SUCCESS) {
		apr_allocator_destroy(my_allocator);
		my_allocator = NULL;
		abort();
	}
    
	if ((apr_thread_mutex_create(&my_mutex, APR_THREAD_MUTEX_NESTED, memory_manager.memory_pool)) != APR_SUCCESS) {
		abort();
	}
    
	apr_allocator_mutex_set(my_allocator, my_mutex);
	apr_allocator_owner_set(my_allocator, memory_manager.memory_pool);
	apr_pool_tag(memory_manager.memory_pool, "core_pool");  

	agc_queue_create(&memory_manager.pool_queue, 50000, memory_manager.memory_pool);
	agc_queue_create(&memory_manager.pool_recycle_queue, 50000, memory_manager.memory_pool);

	agc_threadattr_create(&thd_attr, memory_manager.memory_pool);

	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
	agc_thread_create(&pool_thread_p, thd_attr, pool_thread, NULL, memory_manager.memory_pool);
    
	while (!memory_manager.pool_thread_running) {
		agc_cond_next();
	}
    
	return memory_manager.memory_pool;
}

AGC_DECLARE(void) agc_memory_pool_tag(agc_memory_pool_t *pool, const char *tag)
{
    apr_pool_tag(pool, tag);
}

AGC_DECLARE(agc_status_t) agc_memory_destroy_pool(agc_memory_pool_t **pool)
{
	agc_assert(pool != NULL);

	if (*pool == NULL) 
		return AGC_STATUS_SUCCESS;

	if ((memory_manager.pool_thread_running != 1) || (agc_queue_push(memory_manager.pool_queue, *pool) != AGC_STATUS_SUCCESS)) {
		apr_pool_destroy(*pool);
	}

	*pool = NULL;

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_memory_create_pool(agc_memory_pool_t **pool)
{
    //apr_allocator_t *my_allocator = NULL;
   // apr_thread_mutex_t *my_mutex;

	agc_assert(pool != NULL);

    /*if ((apr_allocator_create(&my_allocator)) != APR_SUCCESS) {
        abort();
    }

    if ((apr_pool_create_ex(pool, NULL, NULL, my_allocator)) != APR_SUCCESS) {
        abort();
    }

    if ((apr_thread_mutex_create(&my_mutex, APR_THREAD_MUTEX_NESTED, *pool)) != APR_SUCCESS) {
        abort();
    }

    apr_allocator_mutex_set(my_allocator, my_mutex);
    apr_allocator_owner_set(my_allocator, *pool); */

    //TODO apr_pool_mutex_set(*pool, my_mutex);

	if (apr_pool_create(pool, NULL) != APR_SUCCESS) {
		abort();
	}
	
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void *) agc_memory_alloc(agc_memory_pool_t *pool, agc_size_t memory)
{
    void *ptr = NULL;

    agc_assert(pool != NULL);

    ptr = apr_palloc(pool, memory);
    agc_assert(ptr != NULL);
    memset(ptr, 0, memory);

    return ptr;
}

AGC_DECLARE(void) agc_memory_pool_set_data(agc_memory_pool_t *pool, const char *key, void *data)
{
    apr_pool_userdata_set(data, key, NULL, pool);
}

AGC_DECLARE(void *) agc_memory_pool_get_data(agc_memory_pool_t *pool, const char *key)
{
    void *data = NULL;

	apr_pool_userdata_get(&data, key, pool);

	return data;
}

AGC_DECLARE(void *) agc_memory_permanent_alloc(agc_size_t memsize)
{
    void *ptr = NULL;
	agc_assert(memory_manager.memory_pool != NULL);
    
    ptr = apr_palloc(memory_manager.memory_pool, memsize);
    
    agc_assert(ptr != NULL);
	memset(ptr, 0, memsize);
    
    return ptr;   
}

