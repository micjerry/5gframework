#ifndef AGC_EVENT_H
#define AGC_EVENT_H

#include <agc.h>

#define EVENT_NULL_SOURCEID 0
#define EVENT_ID_LIMIT 1024

#define EVENT_ID_ALL 0
#define EVENT_ID_EVENTSOCKET 1

#define EVENT_ID_IS_INVALID(x) (x == EVENT_ID_ALL || x >= EVENT_ID_LIMIT)

struct agc_event_header {
	/*! the header name */
	char *name;
	/*! the header value */
	char *value;

	struct agc_event_header *next;
};

typedef struct agc_event_header agc_event_header_t;

struct agc_event {
	/*! the event id (descriptor) */
	int event_id;
        
	/*! the source of event, the same soure will be handled by same thread */
	uint32_t source_id;
    
	/*! the event headers */
	agc_event_header_t *headers; 
    
	agc_event_header_t *last_header;
    
	 /*! the context of event */
	void *context;
    
	/*! the body of the event */
	char *body;
    
	agc_event_callback_func call_back;
    
	/* timer event */
	char timer_set;
    
	agc_rbtree_node_t  timer;
    
	struct agc_event *next;
};

struct agc_event_node;

typedef struct agc_event agc_event_t;

typedef struct agc_event_node agc_event_node_t;

AGC_DECLARE(agc_status_t) agc_event_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_event_shutdown(void);

AGC_DECLARE(uint32_t) agc_event_alloc_source(const char *source_name);

AGC_DECLARE(agc_status_t) agc_event_register(int event_id, const char *event_name);

AGC_DECLARE(agc_status_t) agc_event_get_id(const char *event_name, int *event_id);

AGC_DECLARE(const char *) agc_event_get_name(int event_id);

AGC_DECLARE(agc_status_t) agc_event_create(agc_event_t **event, int event_id, uint32_t source_id);

AGC_DECLARE(agc_status_t) agc_event_create_callback(agc_event_t **event, uint32_t source_id, void *data, agc_event_callback_func callback);

AGC_DECLARE(void) agc_event_destroy(agc_event_t **event);

AGC_DECLARE(agc_status_t) agc_event_add_header(agc_event_t *event, const char *header_name, const char *fmt, ...) PRINTF_FUNCTION(3, 4);

AGC_DECLARE(agc_status_t) agc_event_add_header_string(agc_event_t *event, const char *header_name, const char *data);

AGC_DECLARE(agc_status_t) agc_event_del_header(agc_event_t *event, const char *header_name);

AGC_DECLARE(const char *) agc_event_get_header(agc_event_t *event, const char *header_name);

AGC_DECLARE(agc_status_t) agc_event_add_body(agc_event_t *event, const char *fmt, ...) PRINTF_FUNCTION(2, 3);

AGC_DECLARE(agc_status_t) agc_event_set_body(agc_event_t *event, const char *body);

AGC_DECLARE(const char *) agc_event_get_body(agc_event_t *event);

AGC_DECLARE(agc_status_t) agc_event_dup(agc_event_t **event, agc_event_t *todup);

//dispatch event
AGC_DECLARE(agc_status_t) agc_event_bind(const char *id, int event_id, agc_event_callback_func callback);

AGC_DECLARE(agc_status_t) agc_event_unbind_callback(agc_event_callback_func callback);

AGC_DECLARE(agc_status_t) agc_event_bind_removable(const char *id, int event_id, agc_event_callback_func callback, agc_event_node_t **node);

AGC_DECLARE(agc_status_t) agc_event_unbind(agc_event_node_t **node);

AGC_DECLARE(agc_status_t) agc_event_fire(agc_event_t **event);

AGC_DECLARE(agc_status_t) agc_event_serialize_json_obj(agc_event_t *event, cJSON **json);

AGC_DECLARE(agc_status_t) agc_event_serialize_json(agc_event_t *event, char **str);

#endif
