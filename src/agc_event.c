#include <agc.h>

struct agc_event_node {
	/*! the id of the node */
	char *id;
	/*! the event id enumeration to bind to */
	int event_id;
	/*! the event subclass to bind to for custom events */
	char *event_name;
	/*! a callback function to execute when the event is triggered */
	agc_event_callback_t callback;
	/*! private data */
	void *user_data;
	struct agc_event_node *next;
};

struct agc_event_tpl {
    int event_id;
    char *event_name;
};

typedef struct agc_event_tpl agc_event_tpl_t;

static int SYSTEM_RUNNING = 0;
static int DISPATCH_THREAD_COUNT = 0;
static agc_memory_pool_t *RUNTIME_POOL = NULL;
static unsigned int MAX_DISPATCHER = 64;
#define DISPATCH_QUEUE_LIMIT 10000
static int EVENT_SOURCE_ID = 0;
static agc_mutex_t *SOURCEID_MUTEX = NULL;
static agc_mutex_t *EVENTSTATE_MUTEX = NULL;

static agc_thread_rwlock_t *EVENT_TPLHASH_RWLOCK = NULL;
static agc_hash_t *EVENT_TPL_HASH = NULL;

static agc_queue_t **EVENT_DISPATCH_QUEUES = NULL;
static agc_thread_t **EVENT_DISPATCH_THREADS = NULL;
static uint8_t *EVENT_DISPATCH_QUEUE_RUNNING = NULL;

static agc_thread_rwlock_t *EVENT_NODES_RWLOCK = NULL;
static agc_event_node_t *EVENT_NODES[EVENT_ID_LIMIT + 1] = { NULL };

static void agc_event_launch_dispatch_threads();

static void *agc_event_dispatch_thread(agc_thread_t *thread, void *obj);

static void agc_event_deliver(agc_event_t **event);

static agc_status_t agc_event_base_add_header(agc_event_t *event, const char *header_name, char *data);

static agc_event_header_t *agc_event_get_header_ptr(agc_event_t *event, const char *header_name);

static agc_event_header_t *new_header(const char *header_name);

AGC_DECLARE(agc_status_t) agc_event_init(agc_memory_pool_t *pool)
{
    int i = 0;
    
    assert(pool != NULL);
    
    MAX_DISPATCHER = agc_core_cpu_count() + 1;
	if (MAX_DISPATCHER < 2) {
		MAX_DISPATCHER = 2;
	}
    
    RUNTIME_POOL = pool;
    
    agc_mutex_init(&SOURCEID_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
    agc_mutex_init(&EVENTSTATE_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
    agc_thread_rwlock_create(&EVENT_TPLHASH_RWLOCK, RUNTIME_POOL);
    agc_thread_rwlock_create(&EVENT_NODES_RWLOCK, RUNTIME_POOL);
    EVENT_TPL_HASH = agc_hash_make(RUNTIME_POOL);
    
    EVENT_DISPATCH_QUEUES = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(agc_queue_t *));
    EVENT_DISPATCH_QUEUE_RUNNING = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(uint8_t));
    
    // create dispatch queues
    for (i = 0; i < MAX_DISPATCHER; i++)
    {
        EVENT_DISPATCH_QUEUE_RUNNING[i] = 0;
        agc_queue_create(&EVENT_DISPATCH_QUEUES[i], DISPATCH_QUEUE_LIMIT, RUNTIME_POOL);
    }
    
    agc_mutex_lock(EVENTSTATE_MUTEX);
	SYSTEM_RUNNING = 1;
	agc_mutex_unlock(EVENTSTATE_MUTEX);
    
    agc_event_launch_dispatch_threads();
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_shutdown(void)
{
    int x = 0;
    
    agc_mutex_lock(EVENTSTATE_MUTEX);
	SYSTEM_RUNNING = 1;
	agc_mutex_unlock(EVENTSTATE_MUTEX);
    
    // wait all thread stop
	while (x < 100 && DISPATCH_THREAD_COUNT) {
		agc_yield(100000);
		if (DISPATCH_THREAD_COUNT == last) {
			x++;
		}
		last = DISPATCH_THREAD_COUNT;
	}
    
    //clear event hash
    agc_thread_rwlock_rwlock(EVENT_TPLHASH_RWLOCK);
    agc_hash_clear(EVENT_TPL_HASH);
    agc_thread_rwlock_unlock(EVENT_TPLHASH_RWLOCK);
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(int) agc_event_alloc_source(const char *source_name)
{
    int source_id;
    agc_mutex_lock(EVENTSTATE_MUTEX);
    source_id = EVENT_SOURCE_ID++;
    agc_mutex_unlock(EVENTSTATE_MUTEX);
    
    return source_id;
}

AGC_DECLARE(agc_status_t) agc_event_register(int event_id, const char *event_name)
{
    int evtid = event_id;
    agc_event_tpl_t *event_tpl;
    
    agc_thread_rwlock_rwlock(EVENT_TPLHASH_RWLOCK);
    if (agc_hash_get(EVENT_TPL_HASH, &evtid, AGC_HASH_KEY_STRING)) {
        agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Event id %d was alread registed.\n", evtid);
        agc_thread_rwlock_unlock(EVENT_TPLHASH_RWLOCK);
        return AGC_STATUS_GENERR;
    }
    
    event_tpl = (agc_event_tpl_t *)agc_memory_alloc(RUNTIME_POOL, sizeof(agc_event_tpl_t));
    assert(event_tpl);
    event_tpl->event_id = evtid;
    event_tpl->event_name = agc_core_strdup(RUNTIME_POOL, event_name);
    agc_hash_set(EVENT_TPL_HASH, &event_tpl->event_id, AGC_HASH_KEY_STRING, event_tpl->event_name);
    
    agc_thread_rwlock_unlock(EVENT_TPLHASH_RWLOCK);
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_create(agc_event_t **event, int event_id, int source_id)
{
    *event = NULL;
    *event = malloc(sizeof(agc_event_t));
    
    memset(*event, 0, sizeof(agc_event_t));
    event->event_id = event_id;
    event->source_id = source_id;
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_event_destroy(agc_event_t **event)
{
    agc_event_t *ep = *event;
	agc_event_header_t *hp, *this;
    
    if (ep) {
        for (hp = ep->headers; hp;) {
            this = hp;
            hp = hp->next;
            agc_safe_free(this->name);
			agc_safe_free(this->value);
            agc_safe_free(this);
        }
        
        agc_safe_free(ep->body);
    }
    
    *event = NULL;
}

AGC_DECLARE(agc_status_t) agc_event_add_header(agc_event_t *event, const char *header_name, const char *fmt, ...)
{
    int ret = 0;
	char *data;
	va_list ap;
    
    va_start(ap, fmt);
    ret = agc_vasprintf(&data, fmt, ap);
    va_end(ap);
    
    if (ret == -1) {
		return AGC_STATUS_MEMERR;
	}
    
    return agc_event_base_add_header(event, header_name, data);
}

static void agc_event_launch_dispatch_threads()
{
    agc_threadattr_t *thd_attr;
    int index = 0;
    uint32_t wait_times = 200;
    
    EVENT_DISPATCH_THREADS = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(agc_thread_t *));
    
    for (index = 0; index < MAX_DISPATCHER; i++)
    {
        agc_threadattr_create(&thd_attr, RUNTIME_POOL);
        agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
		agc_threadattr_priority_set(thd_attr, AGC_PRI_REALTIME);
        agc_thread_create(&EVENT_DISPATCH_THREADS[index], thd_attr, agc_event_dispatch_thread, EVENT_DISPATCH_QUEUES[index], RUNTIME_POOL);
        
        while(--wait_times && !EVENT_DISPATCH_QUEUE_RUNNING[index]) {
            agc_yield(10000);
        }
        
        agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Create event dispatch thread %d.\n", index);
    }
}

static void *agc_event_dispatch_thread(agc_thread_t *thread, void *obj)
{
    agc_queue_t *queue = (agc_queue_t *) obj;
    int my_id = 0;
    
    for (my_id = 0; my_id < MAX_DISPATCHER; my_id++) {
		if (EVENT_DISPATCH_THREADS[my_id] == thread) {
			break;
		}
	}
    
    agc_mutex_lock(EVENTSTATE_MUTEX);
    EVENT_DISPATCH_QUEUE_RUNNING[my_id] = 1;
    DISPATCH_THREAD_COUNT++;
    agc_mutex_unlock(EVENTSTATE_MUTEX);
    
    for (;;) {
		void *pop = NULL;
		agc_event_t *event = NULL;

		if (!SYSTEM_RUNNING) {
			break;
		}

		if (agc_queue_pop(queue, &pop) != AGC_STATUS_SUCCESS) {
			continue;
		}

		if (!pop) {
			break;
		}

		event = (agc_event_t *) pop;
		agc_event_deliver(&event);
		agc_os_yield();
	}

    agc_mutex_lock(EVENTSTATE_MUTEX);
    EVENT_DISPATCH_QUEUE_RUNNING[my_id] = 0;
    DISPATCH_THREAD_COUNT--;
    agc_mutex_unlock(EVENTSTATE_MUTEX);
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Dispatch thread %d ended.\n", my_id);
}

static void agc_event_deliver(agc_event_t **event)
{
    int event_id = 0;
    agc_event_node_t *node;
    
    if (SYSTEM_RUNNING) {
        agc_thread_rwlock_rdlock(EVENT_NODES_RWLOCK);
        event_id = (*event)->event_id;
        for (node = EVENT_NODES[event_id]; node; node = node->next) {
            node->callback(*event);
        }
        
        agc_thread_rwlock_unlock(EVENT_NODES_RWLOCK);
    }
    
    agc_event_destroy(event);
}

static agc_status_t agc_event_base_add_header(agc_event_t *event, const char *header_name, char *data)
{
    agc_event_header_t *header = NULL;
    
    header = agc_event_get_header_ptr(event, header_name);
    
    if (!header) {
        header = new_header(header_name);
    } else {
        agc_safe_free(header->value);
    }
    
    assert(header);
    
    header->value = data;
    
    if (event->last_header) {
        event->last_header->next = header;
    } else {
        event->headers = header;
        header->next = NULL;
    }
    
    event->last_header = header;
    
    return AGC_STATUS_SUCCESS;
}

static agc_event_header_t *agc_event_get_header_ptr(agc_event_t *event, const char *header_name)
{
    agc_event_header_t *hp;
    
    assert(event);
    
    if (!header_name)
		return NULL;
    
    for (hp = event->headers; hp; hp = hp->next) {
		if (!strcasecmp(hp->name, header_name)) {
			return hp;
		}
	}
	return NULL;
}

static agc_event_header_t *new_header(const char *header_name)
{
    agc_event_header_t *header;
    
    header = malloc(sizeof(*header));
    memset(header, 0, sizeof(*header));
    header->name = strdup(header_name);
    return header;
}


