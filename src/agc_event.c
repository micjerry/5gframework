#include <agc.h>

struct agc_event_node {
	/*! the id of the node */
	char *id;
	/*! the event id enumeration to bind to */
	int event_id;
	/*! a callback function to execute when the event is triggered */
	agc_event_callback_func callback;
	/*! private data */
	void *user_data;
	struct agc_event_node *next;
};

struct fast_event_node {
	agc_event_header_t **headers;
	int type;
	int header_numbers;
};

typedef struct fast_event_node fast_event_node_t;

static volatile int SYSTEM_RUNNING = 0;
static int DISPATCH_THREAD_COUNT = 0;
static agc_memory_pool_t *RUNTIME_POOL = NULL;
static unsigned int MAX_DISPATCHER = 64;
#define DISPATCH_QUEUE_LIMIT 10000
static uint32_t EVENT_SOURCE_ID = 0;
static agc_mutex_t *SOURCEID_MUTEX = NULL;
static agc_mutex_t *EVENTSTATE_MUTEX = NULL;

static agc_thread_rwlock_t *EVENT_TEMPLATES_RWLOCK = NULL;
static char *event_templates[EVENT_ID_LIMIT] = { NULL };

static agc_queue_t **EVENT_DISPATCH_QUEUES = NULL;
static agc_thread_t **EVENT_DISPATCH_THREADS = NULL;
static uint8_t *EVENT_DISPATCH_QUEUE_RUNNING = NULL;

static agc_queue_t **FAST_EVENT_QUEUES = NULL;

static agc_thread_rwlock_t *EVENT_NODES_RWLOCK = NULL;
static agc_event_node_t *EVENT_NODES[EVENT_ID_LIMIT] = { NULL };

static void agc_event_launch_dispatch_threads();

static void *agc_event_dispatch_thread(agc_thread_t *thread, void *obj);

static void agc_event_deliver(agc_event_t **event);

static agc_status_t agc_event_base_add_header(agc_event_t *event, const char *header_name, char *data);

static agc_status_t agc_event_base_add_header_nocheck(agc_event_t *event, const char *header_name, char *data);

static agc_event_header_t *agc_event_get_header_ptr(agc_event_t *event, const char *header_name);

static agc_event_header_t *new_header(const char *header_name);

static void init_ids();

static agc_status_t fast_event_create(agc_event_t **event, int fast_event_type, int event_id, const char **headers, 
	const int *valuelengths, int headernumbers, int bodylength);

static inline void fast_add_header(agc_event_t *event, agc_event_header_t *header);

static inline agc_event_header_t *fast_get_header(agc_event_t *event, int index);

AGC_DECLARE(agc_status_t) agc_event_init(agc_memory_pool_t *pool)
{
	int i = 0;
    
	assert(pool != NULL);
    
	MAX_DISPATCHER = (2 * agc_core_cpu_count())  + 1;
	if (MAX_DISPATCHER < 2) {
		MAX_DISPATCHER = 2;
	}
    
	RUNTIME_POOL = pool;
    
	agc_mutex_init(&SOURCEID_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
	agc_mutex_init(&EVENTSTATE_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
	agc_thread_rwlock_create(&EVENT_TEMPLATES_RWLOCK, RUNTIME_POOL);
	agc_thread_rwlock_create(&EVENT_NODES_RWLOCK, RUNTIME_POOL);


	FAST_EVENT_QUEUES = agc_memory_alloc(RUNTIME_POOL, EVENT_FAST_TYPE_Invalid * sizeof(agc_queue_t *));
    
	EVENT_DISPATCH_QUEUES = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER * sizeof(agc_queue_t *));
	EVENT_DISPATCH_QUEUE_RUNNING = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER * sizeof(uint8_t));

	init_ids();
    
	// create dispatch queues
	for (i = 0; i < MAX_DISPATCHER; i++)
	{
		EVENT_DISPATCH_QUEUE_RUNNING[i] = 0;
		agc_queue_create(&EVENT_DISPATCH_QUEUES[i], DISPATCH_QUEUE_LIMIT, RUNTIME_POOL);
	}
    
	SYSTEM_RUNNING = 1;
    
	agc_event_launch_dispatch_threads();

	if (agc_event_fast_initial(EVENT_FAST_TYPE_CallBack, 0, 1000, NULL, NULL, 0, 0) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Event init failed.\n");
		return AGC_STATUS_FALSE;
	}
    
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Event init success.\n");
    
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_shutdown(void)
{
    int x = 0;
    int last = 0;
    
	SYSTEM_RUNNING = 0;
    
    // wait all thread stop
	while (x < 100 && DISPATCH_THREAD_COUNT) {
		agc_yield(100000);
		if (DISPATCH_THREAD_COUNT == last) {
			x++;
		}
		last = DISPATCH_THREAD_COUNT;
	}
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Event shutdown success.\n");
    
    return AGC_STATUS_SUCCESS;
}

static void init_ids()
{
	memset(event_templates, 0, sizeof(event_templates));
	event_templates[EVENT_ID_ALL] = EVENT_NAME_ALL;
	event_templates[EVENT_ID_EVENTSOCKET] = EVENT_NAME_EVENTSOCKET;
	event_templates[EVENT_ID_CMDRESULT] = EVENT_NAME_CMDRESULT;
	event_templates[EVENT_ID_SIG2MEDIA] = EVENT_NAME_SIG2MEDIA;
	event_templates[EVENT_ID_MEDIA2SIG] = EVENT_NAME_MEDIA2SIG;
	event_templates[EVENT_ID_JSONCMD] = EVENT_NAME_JSONCMD;
}

AGC_DECLARE(uint32_t) agc_event_alloc_source(const char *source_name)
{
	uint32_t source_id;
	agc_mutex_lock(EVENTSTATE_MUTEX);
	source_id = ++EVENT_SOURCE_ID;
	if (EVENT_SOURCE_ID == UINT32_MAX)
		EVENT_SOURCE_ID = 0;
	
	agc_mutex_unlock(EVENTSTATE_MUTEX);
    
	return source_id;
}

AGC_DECLARE(agc_status_t) agc_event_register(int event_id, const char *event_name)
{
	if (EVENT_ID_IS_INVALID(event_id))
		return AGC_STATUS_GENERR;
    
	agc_thread_rwlock_wrlock(EVENT_TEMPLATES_RWLOCK);
	if (event_templates[event_id] != NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Event id %d was alread registed.\n", event_id);
		agc_thread_rwlock_unlock(EVENT_TEMPLATES_RWLOCK);
		return AGC_STATUS_GENERR;
	}
    
	event_templates[event_id] = agc_core_strdup(RUNTIME_POOL, event_name);
    
	agc_thread_rwlock_unlock(EVENT_TEMPLATES_RWLOCK);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_get_id(const char *event_name, int *event_id)
{
	int i = 0;
	int found = 0;
    
	if (!event_name || !event_id)
		return AGC_STATUS_GENERR;
    
	agc_thread_rwlock_rdlock(EVENT_TEMPLATES_RWLOCK);
	
	for (i = 0; i < EVENT_ID_LIMIT; i++) {
		if (!event_templates[i])
			continue;
		
		if (strcasecmp(event_name, event_templates[i]) == 0) {
			*event_id = i;
			found = 1;
			break;
		}
	}
        
	agc_thread_rwlock_unlock(EVENT_TEMPLATES_RWLOCK);
    
	if (!found) {
		return AGC_STATUS_GENERR;
	}
        
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(const char *) agc_event_get_name(int event_id)
{
	const char *event_name = NULL;

	if (EVENT_ID_IS_INVALID(event_id)) {
		return NULL;
	}

	agc_thread_rwlock_rdlock(EVENT_TEMPLATES_RWLOCK);
	event_name = event_templates[event_id];
	agc_thread_rwlock_unlock(EVENT_TEMPLATES_RWLOCK);
	return event_name;
}

AGC_DECLARE(agc_status_t) agc_event_create(agc_event_t **event, int event_id, uint32_t source_id)
{
	agc_event_t * new_event;
	new_event= malloc(sizeof(agc_event_t));

	memset(new_event, 0, sizeof(agc_event_t));
	new_event->event_id = event_id;
	new_event->source_id = source_id;

	*event = new_event;
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_set_id(agc_event_t *event, int event_id)
{
	if (event)
		event->event_id = event_id;

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_create_callback(agc_event_t **event,  
                                                    uint32_t source_id, 
                                                    void *data, 
                                                    agc_event_callback_func callback)
{
	agc_event_t * new_event;

	if (agc_event_create(&new_event, 0, source_id) != AGC_STATUS_SUCCESS) {
		return AGC_STATUS_GENERR;
	}
	new_event->call_back = callback;
	new_event->context = data;

	*event = new_event;
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
    
	return agc_event_base_add_header_nocheck(event, header_name, data);
}

AGC_DECLARE(agc_status_t) agc_event_add_header_repeatcheck(agc_event_t *event, const char *header_name, const char *fmt, ...)
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

AGC_DECLARE(agc_status_t) agc_event_add_header_string(agc_event_t *event, const char *header_name, const char *data)
{
    if (data) {
		return agc_event_base_add_header(event, header_name, strdup(data));
	}
    
	return AGC_STATUS_GENERR;
}

AGC_DECLARE(agc_status_t) agc_event_del_header(agc_event_t *event, const char *header_name)
{
	agc_event_header_t *hp, *lp = NULL, *tp;

	tp = event->headers;

	while (tp) {
		hp = tp;
		tp = tp->next;
	    
		if (!strcasecmp(header_name, hp->name)) {
			if (lp) {
				lp->next = hp->next;
			} else {
				event->headers = hp->next;
			}
	        
			if (hp == event->last_header || !hp->next) {
				event->last_header = lp;
			}
	        
			agc_safe_free(hp->name);
			agc_safe_free(hp->value);
			agc_safe_free(hp);
	    } else {
			lp = hp;
	    }
	}

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(const char *) agc_event_get_header(agc_event_t *event, const char *header_name)
{
	agc_event_header_t *hp;
	if ((hp = agc_event_get_header_ptr(event, header_name))) {
		return hp->value;
	}

	return NULL;
}

AGC_DECLARE(agc_status_t) agc_event_add_body(agc_event_t *event, const char *fmt, ...)
{
	int ret = 0;
	char *data;

	assert(event);
	va_list ap;
    
	if (fmt) {
		va_start(ap, fmt);
		ret = agc_vasprintf(&data, fmt, ap);
		va_end(ap);

		if (ret == -1) {
			return AGC_STATUS_GENERR;
		} else {
			agc_safe_free(event->body);
			event->body = data;
			return AGC_STATUS_SUCCESS;
		}
	} 
    
	return AGC_STATUS_GENERR;
}

AGC_DECLARE(agc_status_t) agc_event_set_body(agc_event_t *event, const char *body)
{
	assert(event);

	if (!body) {
		return AGC_STATUS_GENERR;
	}
	
	agc_safe_free(event->body);
	if (body)
		event->body = strdup(body);
    
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(const char *) agc_event_get_body(agc_event_t *event)
{
    return (event ? event->body : NULL);
}

AGC_DECLARE(agc_status_t) agc_event_dup(agc_event_t **event, agc_event_t *todup)
{
    agc_event_header_t *hp;
    
    assert(todup);
    
    if (agc_event_create(event, todup->event_id, todup->source_id) != AGC_STATUS_SUCCESS) {
        return AGC_STATUS_GENERR;
    }
    
    for (hp = todup->headers; hp; hp = hp->next) {
        agc_event_add_header_string(*event, hp->name, hp->value);
    }
    
    if (todup->body) {
		(*event)->body = strdup(todup->body);
	}
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_bind(const char *id, 
                                         int event_id, 
                                         agc_event_callback_func callback)
{
    return agc_event_bind_removable(id, event_id, callback, NULL);
}

AGC_DECLARE(agc_status_t) agc_event_bind_removable(const char *id, 
                                                   int event_id, 
                                                   agc_event_callback_func callback, 
                                                   agc_event_node_t **node)
{
	agc_event_node_t *event_node;
    
	if (event_id > EVENT_ID_LIMIT)
		return AGC_STATUS_GENERR;
    
	event_node = (agc_event_node_t *)malloc(sizeof(agc_event_node_t));
	assert(event_node);
	memset(event_node, 0, sizeof(agc_event_node_t));
    
	event_node->id = strdup(id);
	event_node->event_id = event_id;
	event_node->callback = callback;
          
	agc_thread_rwlock_wrlock(EVENT_NODES_RWLOCK);
	if (EVENT_NODES[event_id]) {
		event_node->next = EVENT_NODES[event_id];
	}
    
	EVENT_NODES[event_id] = event_node;
	agc_thread_rwlock_unlock(EVENT_NODES_RWLOCK);

	if (node)
		*node = event_node;
    
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fire(agc_event_t **event)
{
	agc_queue_t *event_queue = NULL;
	int queue_index = 0;
	agc_event_t *eventp = *event;

	if (!eventp) {
		return AGC_STATUS_GENERR;
	}

	if (SYSTEM_RUNNING == 0) {
		agc_event_destroy(event);
		return AGC_STATUS_SUCCESS;
	}

	if (eventp->debug_id) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event: %d received.\n", eventp->debug_id);
	}

	if (eventp->source_id != EVENT_NULL_SOURCEID) {
		queue_index = eventp->source_id % MAX_DISPATCHER;
	} else {
		queue_index = agc_random(MAX_DISPATCHER);
	}

	event_queue = EVENT_DISPATCH_QUEUES[queue_index];
	if (!event_queue) {
		return AGC_STATUS_GENERR;
	}

	agc_queue_push(event_queue, eventp);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_unbind(agc_event_node_t **node)
{
	int event_id = 0;
	agc_event_node_t *event_node, *np, *lnp = NULL;
	agc_status_t status = AGC_STATUS_FALSE;
    
	event_node = *node;
    
	if (!event_node) {
		return AGC_STATUS_GENERR;
	}
    
	event_id = event_node->event_id;
	agc_thread_rwlock_wrlock(EVENT_NODES_RWLOCK);
    
	for (np = EVENT_NODES[event_id]; np; np = np->next) {
		if (np == event_node) {
			if (lnp) {
				lnp->next = event_node->next;
			} else {
				EVENT_NODES[event_id] = event_node->next;
			}
            
			agc_safe_free(event_node->id);
			agc_safe_free(event_node);
			*node = NULL;
			status = AGC_STATUS_SUCCESS;
			break;
		} else {
			lnp = np;
		}
	}
    
	agc_thread_rwlock_unlock(EVENT_NODES_RWLOCK);
    
	return status;    
}

AGC_DECLARE(agc_status_t) agc_event_unbind_callback(agc_event_callback_func callback)
{
	agc_event_node_t *event_node, *np, *lnp = NULL;
	agc_status_t status = AGC_STATUS_FALSE;
	int id = 0;

	agc_thread_rwlock_wrlock(EVENT_NODES_RWLOCK);

	for (id = 0; id < EVENT_ID_LIMIT; id++) {
		lnp = NULL;
	    
		for (np = EVENT_NODES[id]; np;) {
			event_node = np;
			np = np->next;
	        
			if (event_node->callback == callback) {
				if (lnp) {
					lnp->next = event_node->next;
				} else {
					EVENT_NODES[id] = event_node->next;
				}
	            
				agc_safe_free(event_node->id);
				agc_safe_free(event_node);
				status = AGC_STATUS_SUCCESS;
				break;
			} else {
				lnp = event_node;
			}
		}
	}

	agc_thread_rwlock_unlock(EVENT_NODES_RWLOCK);
	return status;
}

AGC_DECLARE(agc_status_t) agc_event_serialize_json_obj(agc_event_t *event, cJSON **json)
{
	agc_event_header_t *hp;
	cJSON *cj;
	char str_evtid[25];

	cj = cJSON_CreateObject();

	for (hp = event->headers; hp; hp = hp->next) {
		cJSON_AddItemToObject(cj, hp->name, cJSON_CreateString(hp->value));
	}
    
	agc_snprintf(str_evtid, sizeof(str_evtid), "%d", event->event_id);
	 cJSON_AddItemToObject(cj, "_id", cJSON_CreateString(str_evtid));

	if (event_templates[event->event_id]) {
		cJSON_AddItemToObject(cj, "_name", cJSON_CreateString(event_templates[event->event_id]));
	}

	if (event->body) {
		int blen = (int) strlen(event->body);
		char tmp[25];

		agc_snprintf(tmp, sizeof(tmp), "%d", blen);

		cJSON_AddItemToObject(cj, "_length", cJSON_CreateString(tmp));
		cJSON_AddItemToObject(cj, "_body", cJSON_CreateString(event->body));
	}

	*json = cj;

	return AGC_STATUS_SUCCESS;

}

AGC_DECLARE(agc_status_t) agc_event_create_json(agc_event_t **event, const char *json)
{
	agc_event_t *new_event = NULL;
	cJSON *cj, *cjp;

	if (!(cj = cJSON_Parse(json))) {
		return AGC_STATUS_FALSE;
	}

	if (agc_event_create(&new_event, 0, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) {
		cJSON_Delete(cj);
		return AGC_STATUS_FALSE;
	}

	for (cjp = cj->child; cjp; cjp = cjp->next)  {
		int event_id = 0;
		char *name = cjp->string;
		char *value = cjp->valuestring;

		if (name && value) {
			if (!strcasecmp(name, "_body")) {
				agc_event_set_body(new_event, value);
			} else if (!strcasecmp(name, "_id")) {
				event_id = atoi(value);
				if (EVENT_ID_IS_INVALID(event_id)) {
					cJSON_Delete(cj);
					agc_event_destroy(&new_event);
					return AGC_STATUS_FALSE;
				}
				agc_event_set_id(new_event, event_id);
			} else {
				agc_event_add_header_string(new_event, name, value);
			}
		}
	}

	cJSON_Delete(cj);
	
	if (EVENT_ID_IS_INVALID(new_event->event_id)) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Invalid event received.\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}
	
	*event = new_event;
	return AGC_STATUS_SUCCESS;
	
}

AGC_DECLARE(agc_status_t) agc_event_serialize_json(agc_event_t *event, char **str)
{
	cJSON *cj;
	*str = NULL;

	if (agc_event_serialize_json_obj(event, &cj) == AGC_STATUS_SUCCESS) {
		*str = cJSON_PrintUnformatted(cj);
		cJSON_Delete(cj);
		
		return AGC_STATUS_SUCCESS;
	}

	return AGC_STATUS_FALSE;
}

static void agc_event_launch_dispatch_threads()
{
	agc_threadattr_t *thd_attr;
	int index = 0;
	uint32_t wait_times = 0;

	EVENT_DISPATCH_THREADS = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(agc_thread_t *));
    
	for (index = 0; index < MAX_DISPATCHER; index++)
	{
		wait_times = 200;
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
		agc_time_t time_start;
		int debug_id = 0;
		

		if (!SYSTEM_RUNNING) {
			break;
		}

		if (agc_queue_trypop(queue, &pop) != AGC_STATUS_SUCCESS) {
			agc_yield(10000);
			continue;
		}

		if (!pop) {
			break;
		}

		event = (agc_event_t *) pop;
		if ((debug_id = event->debug_id)) {
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event: %d handle by event_thread %d .\n", debug_id, my_id);
			time_start = agc_time_now();
		}
		agc_event_deliver(&event);
		if (debug_id) {
			int time_used = 0;
			time_used = (int)((agc_time_now() - time_start)/1000);
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event: %d handle by event_thread %d finished %d milliseconds used .\n", debug_id, my_id, time_used);
		}
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
	agc_event_t *pevent = *event;
    
	assert(pevent);
    
	if (SYSTEM_RUNNING) {
		if (pevent->call_back) {
			//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Dispatch callback.\n");
			if (pevent->debug_id) {
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event: %d callback trigger.\n", pevent->debug_id);
			}
			
			pevent->call_back(pevent->context);
		} else {
			agc_thread_rwlock_rdlock(EVENT_NODES_RWLOCK);
			if (pevent->debug_id) {
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "event: %d subs trigger.\n", pevent->debug_id);
			}
			
			event_id = pevent->event_id;
			for (node = EVENT_NODES[event_id]; node; node = node->next) {
				node->callback(pevent);
			}
            
			for (node = EVENT_NODES[EVENT_ID_ALL]; node; node = node->next) {
				node->callback(pevent);
			}
        
			agc_thread_rwlock_unlock(EVENT_NODES_RWLOCK);
		}
	}

	if (pevent->fast) {
		agc_event_fast_release(event);
	} else {
		agc_event_destroy(event);
	}
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


static agc_status_t agc_event_base_add_header_nocheck(agc_event_t *event, const char *header_name, char *data)
{
	agc_event_header_t *header = NULL;

	header = new_header(header_name);

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

AGC_DECLARE(agc_status_t) agc_event_fast_initial(int fast_event_type, int event_id, int capacity, 
										const char **headers, const int *valuelengths, int headernumbers, int bodylength)
{
	int i = 0;
	agc_event_t *new_event = NULL;
	
	if (fast_event_type >= EVENT_FAST_TYPE_Invalid) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent invalid event type:%d.\n", fast_event_type);
		return AGC_STATUS_GENERR;
	}

	if (FAST_EVENT_QUEUES[fast_event_type] != NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d already initialed.\n", fast_event_type);
		return AGC_STATUS_GENERR;
	}

	agc_queue_create(&FAST_EVENT_QUEUES[fast_event_type], (capacity + 10), RUNTIME_POOL);

	for (i = 0; i < capacity; i++ ) {
		if (fast_event_create(&new_event, fast_event_type, event_id, headers, valuelengths, headernumbers, bodylength) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d create event failed.\n", fast_event_type);
			return AGC_STATUS_GENERR;
		}

		if (agc_queue_trypush(FAST_EVENT_QUEUES[fast_event_type], new_event) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d push event failed.\n", fast_event_type);
			return AGC_STATUS_GENERR;
		}

		new_event = NULL;
	}

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_alloc(agc_event_t **event, int fast_event_type)
{
	void *pop = NULL;
	
	if (fast_event_type >= EVENT_FAST_TYPE_Invalid) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent invalid event type:%d.\n", fast_event_type);
		return AGC_STATUS_GENERR;
	}

	if (FAST_EVENT_QUEUES[fast_event_type] == NULL) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d not initialed.\n", fast_event_type);
		return AGC_STATUS_GENERR;
	}

	if (agc_queue_trypop(FAST_EVENT_QUEUES[fast_event_type], &pop) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d pop failed.\n", fast_event_type);
		return AGC_STATUS_GENERR;
	}

	*event = (agc_event_t *)pop;

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_create_callback(agc_event_t **event, void *data, agc_event_callback_func callback)
{
	agc_event_t *call_event;

	if (agc_event_fast_alloc(&call_event, EVENT_FAST_TYPE_CallBack) != AGC_STATUS_SUCCESS ) {
		return AGC_STATUS_GENERR;
	}

	call_event->call_back = callback;
	call_event->context = data;

	*event = call_event;
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_set_strheader(agc_event_t *event, int index, const char *name, const char *value)
{
	agc_event_header_t *header = NULL;
	
	header = fast_get_header(event, index);
	if (!header) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_event_fast_set_strheader  header not exist index %d.\n",  index);
		return AGC_STATUS_GENERR;
	}

	strcpy(header->name, name);
	strcpy(header->value, value);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_set_intheader(agc_event_t *event, int index, const char *name, int value)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, index);

	if (!header) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_event_fast_set_intheader  header not exist index %d.\n", index);
		return AGC_STATUS_GENERR;
	}

	strcpy(header->name, name);
	sprintf(header->value, "%d", value);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_set_uintheader(agc_event_t *event, int index,  const char *name, uint32_t value)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, index);

	if (!header) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_event_fast_set_uintheader  header not exist index %d.\n", index);
		return AGC_STATUS_GENERR;
	}

	strcpy(header->name, name);
	sprintf(header->value, "%u", value);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_get_type(agc_event_t *event, int *type)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, EVENT_FAST_HEADERINDEX1);
	if (!header) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_event_fast_get_type  header not exist index %d.\n", index);
		return AGC_STATUS_GENERR;
	}

	*type = atoi(header->value);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_release(agc_event_t **event) {
	fast_event_node_t *fast;
	agc_event_t *pevent = *event;
	
	if (!pevent || !pevent->fast) {
		return AGC_STATUS_FALSE;
	}

	fast = pevent->fast;

	agc_queue_trypush(FAST_EVENT_QUEUES[fast->type], pevent);

	*event = NULL;
		
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_event_fast_set_body(agc_event_t *event, const char *body, int len)
{
	if (!event || !event->body) {
		return AGC_STATUS_FALSE;
	}

	memcpy(event->body, body, len);
	return AGC_STATUS_SUCCESS;
}

static agc_status_t fast_event_create(agc_event_t **event, int fast_event_type, int event_id, const char **headers, 
								const int *valuelengths, int headernumbers, int bodylength)
{
	agc_event_t *new_event = NULL;
	int payload_size = 0;
	int i = 0;
	char *buff = NULL;
	fast_event_node_t *fast = NULL;
	agc_event_header_t *new_header = NULL;

	new_event= malloc(sizeof(agc_event_t));
	memset(new_event, 0, sizeof(agc_event_t));
	assert(new_event);
	new_event->event_id = event_id;

	/* payload
	*  fast_event_node_t
	*  agc_event_header_t* * headernumbers
	*  agc_event_header_t * headernumbers
	*  name->value name->value
	*  body
	*/
	payload_size += sizeof(fast_event_node_t);
	payload_size += headernumbers * sizeof(agc_event_header_t *);
	payload_size += headernumbers * sizeof(agc_event_header_t);
	
	for (i = 0; i < headernumbers; i++) {
		payload_size += (strlen(headers[i]) + 1);
		payload_size += valuelengths[i];
	}

	payload_size += bodylength;

	buff = malloc(payload_size);
	assert(buff);
	memset(buff, 0, payload_size);
	
	fast = (fast_event_node_t *)buff;
	buff += sizeof(fast_event_node_t);

	//fill fast
	new_event->fast = fast;
	fast->type = fast_event_type;
	fast->header_numbers = headernumbers;
	fast->headers = (agc_event_header_t **)buff;
	buff += headernumbers * sizeof(agc_event_header_t *);

	//fill headers
	for (i = 0; i < headernumbers; i++) {
		fast->headers[i] = (agc_event_header_t *)buff;
		fast_add_header(new_event, fast->headers[i]);
		buff += sizeof(agc_event_header_t);
	}

	for (i = 0; i < headernumbers; i++) {
		new_header = fast->headers[i];
		new_header->name = buff;
		buff += (strlen(headers[i]) + 1);
		new_header->value = buff;
		buff += valuelengths[i];
	}

	if (bodylength > 0) {
		new_event->body = buff;
	}

	if (headernumbers > 0) {
		new_header = fast->headers[EVENT_FAST_HEADERINDEX1];
		strcpy(new_header->name, EVENT_HEADER_TYPE);
		sprintf(new_header->value, "%d", fast_event_type);
	}

	*event = new_event;
	
	return AGC_STATUS_SUCCESS;
}

static inline void fast_add_header(agc_event_t *event, agc_event_header_t *header)
{
	if (event->last_header) {
		event->last_header->next = header;
	} else {
		event->headers = header;
	}

	event->last_header = header;
}

static inline agc_event_header_t *fast_get_header(agc_event_t *event, int index)
{
	fast_event_node_t *fast;
	agc_event_header_t *header;
	
	if (!event || !event->fast) {
		return NULL;
	}

	fast = event->fast;
	if (index >= fast->header_numbers) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d invalid header index %d.\n", fast->type, index);
		return NULL;
	}

	header = fast->headers[index];

	if (!header) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "fastevent event type:%d header not exist index %d.\n", fast->type, index);
		return NULL;
	}

	return header;
}

