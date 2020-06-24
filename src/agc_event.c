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

static agc_thread_rwlock_t *EVENT_NODES_RWLOCK = NULL;
static agc_event_node_t *EVENT_NODES[EVENT_ID_LIMIT] = { NULL };

static void agc_event_launch_dispatch_threads();

static void *agc_event_dispatch_thread(agc_thread_t *thread, void *obj);

static void agc_event_deliver(agc_event_t **event);

static agc_status_t agc_event_base_add_header(agc_event_t *event, const char *header_name, char *data);

static agc_event_header_t *agc_event_get_header_ptr(agc_event_t *event, const char *header_name);

static agc_event_header_t *new_header(const char *header_name);

static void init_ids();

AGC_DECLARE(agc_status_t) agc_event_init(agc_memory_pool_t *pool)
{
	int i = 0;
    
	assert(pool != NULL);
    
	MAX_DISPATCHER = (agc_core_cpu_count() / 2) + 1;
	if (MAX_DISPATCHER < 2) {
		MAX_DISPATCHER = 2;
	}
    
	RUNTIME_POOL = pool;
    
	agc_mutex_init(&SOURCEID_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
	agc_mutex_init(&EVENTSTATE_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
	agc_thread_rwlock_create(&EVENT_TEMPLATES_RWLOCK, RUNTIME_POOL);
	agc_thread_rwlock_create(&EVENT_NODES_RWLOCK, RUNTIME_POOL);
    
	EVENT_DISPATCH_QUEUES = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(agc_queue_t *));
	EVENT_DISPATCH_QUEUE_RUNNING = agc_memory_alloc(RUNTIME_POOL, MAX_DISPATCHER*sizeof(uint8_t));

	init_ids();
    
	// create dispatch queues
	for (i = 0; i < MAX_DISPATCHER; i++)
	{
		EVENT_DISPATCH_QUEUE_RUNNING[i] = 0;
		agc_queue_create(&EVENT_DISPATCH_QUEUES[i], DISPATCH_QUEUE_LIMIT, RUNTIME_POOL);
	}
    
	SYSTEM_RUNNING = 1;
    
	agc_event_launch_dispatch_threads();
    
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
	event_templates[EVENT_ID_ALL] = "all";
	event_templates[EVENT_ID_EVENTSOCKET] ="esl";
	event_templates[EVENT_ID_CMDRESULT] ="api_res";
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
	agc_event_t *pevent = *event;
    
	assert(pevent);
    
	if (SYSTEM_RUNNING) {
		if (pevent->call_back) {
			//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Dispatch callback.\n");
			pevent->call_back(pevent->context);
		} else {
			agc_thread_rwlock_rdlock(EVENT_NODES_RWLOCK);
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


