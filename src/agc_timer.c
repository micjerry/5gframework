#include <agc.h>

static agc_rbtree_t agc_timer_rbtree;

static agc_rbtree_node_t  agc_timer_sentinel;

static agc_mutex_t *TIMER_RBTREE_MUTEX = NULL;

static agc_memory_pool_t *RUNTIME_POOL = NULL;

static agc_thread_t *TIMER_DISPATCH_THREAD = NULL;

static volatile int SYSTEM_RUNNING = 0;

static volatile int SYSTEM_SHUTDOWN = 0;

static agc_time_t current_time;

static void agc_timer_launch_dispatch_thread();

static void *agc_timer_dispatch_timer(agc_thread_t *thread, void *obj);

AGC_DECLARE(agc_status_t) agc_timer_init(agc_memory_pool_t *pool)
{
    assert(pool);
    RUNTIME_POOL = pool;
    agc_mutex_init(&TIMER_RBTREE_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
    agc_rbtree_init(&agc_timer_rbtree, &agc_timer_sentinel,
                    agc_rbtree_insert_timer_value);
    
    agc_timer_launch_dispatch_thread();
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Timer init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_timer_shutdown(void)
{
    int wait_times = 200;
    
    SYSTEM_RUNNING = 0;
    while(--wait_times && !SYSTEM_SHUTDOWN) {
        agc_yield(10000);
    }
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Timer shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_timer_del_timer(agc_event_t *ev)
{
    agc_mutex_lock(TIMER_RBTREE_MUTEX);
    agc_rbtree_delete(&agc_timer_rbtree, &ev->timer);
    agc_mutex_unlock(TIMER_RBTREE_MUTEX);
}

AGC_DECLARE(void) agc_timer_add_timer(agc_event_t *ev, agc_msec_t timer)
{
    agc_msec_t      key;
    agc_time_t current_ms = (current_time / 1000);

    key = current_ms + timer;
    
    if (ev->timer_set) {
        agc_timer_del_timer(ev);
    }
    
    ev->timer.key = key;
    
    agc_mutex_lock(TIMER_RBTREE_MUTEX);
    agc_rbtree_insert(&agc_timer_rbtree, &ev->timer);
    agc_mutex_unlock(TIMER_RBTREE_MUTEX);

    ev->timer_set = 1;
}

AGC_DECLARE(agc_time_t) agc_timer_curtime()
{
    return current_time;
}

static void agc_timer_launch_dispatch_thread()
{
    int wait_times = 200;
    agc_threadattr_t *thd_attr;
    
    agc_threadattr_create(&thd_attr, RUNTIME_POOL);
    agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
    agc_threadattr_priority_set(thd_attr, AGC_PRI_REALTIME);
    agc_thread_create(&TIMER_DISPATCH_THREAD, thd_attr, agc_timer_dispatch_timer, &agc_timer_rbtree, RUNTIME_POOL);
    
    while(--wait_times && !SYSTEM_RUNNING) {
        agc_yield(10000);
    }
}

static void *agc_timer_dispatch_timer(agc_thread_t *thread, void *obj)
{
    agc_rbtree_t *timertree = (agc_rbtree_t *)obj;  
    agc_event_t        *ev;
    agc_rbtree_node_t  *node, *root, *sentinel;
    agc_time_t current_ms;
    
    SYSTEM_RUNNING = 1;
    SYSTEM_SHUTDOWN = 0;
    
    sentinel = timertree->sentinel;
    for ( ;; ) {
        current_time = agc_time_now();
        current_ms = (current_time / 1000);
        if (!SYSTEM_RUNNING)
            break;
        
        root = timertree->root;
        if (root == sentinel) {
            agc_os_yield();
            continue;
        }
        
        agc_mutex_lock(TIMER_RBTREE_MUTEX);
        node = agc_rbtree_min(root, sentinel);
        agc_mutex_unlock(TIMER_RBTREE_MUTEX);
        
        if ((agc_msec_int_t) (node->key - current_ms) > 0) {
            agc_os_yield();
            continue;
        }
        
        ev = (agc_event_t *) ((char *) node - offsetof(agc_event_t, timer));
        ev->timer_set = 0;
        
        agc_mutex_lock(TIMER_RBTREE_MUTEX);
        agc_rbtree_delete(timertree, &ev->timer);
        agc_mutex_unlock(TIMER_RBTREE_MUTEX);
        agc_event_fire(&ev);
    }
    
    SYSTEM_SHUTDOWN = 1;
}

