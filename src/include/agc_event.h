#ifndef AGC_EVENT_H
#define AGC_EVENT_H

#include <agc.h>

#define EVENT_NULL_SOURCEID 0
#define EVENT_ID_LIMIT 1024

struct agc_event_header {
	/*! the header name */
	char *name;
	/*! the header value */
	char *value;
	/*! hash of the header name */
	unsigned long hash;
	struct agc_event_header *next;
};

struct agc_event {
	/*! the event id (descriptor) */
	int event_id;
    
    /*! the name of the event */
    char *event_name;
    
    /*! the source of event, the same soure will be handled by same thread */
    int source_id;
    
	/*! the event headers */
	agc_event_header *headers; 
    
    agc_event_header *last_header;
    
    /*! the context of event */
    void *context;
    
	/*! the body of the event */
	char *body;
    
	/*! unique key */
	unsigned long key;
	struct agc_event *next;
};

struct agc_event_node;

typedef struct agc_event agc_event_t;

typedef struct agc_event_node agc_event_node_t;

typedef void (*agc_event_callback_func) (agc_event_t *);

AGC_DECLARE(agc_status_t) agc_event_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_event_shutdown(void);

AGC_DECLARE(int) agc_event_alloc_source(const char *source_name);

AGC_DECLARE(agc_status_t) agc_event_register(int event_id, const char *event_name);

AGC_DECLARE(agc_status_t) agc_event_create(agc_event_t **event, int event_id, int source_id);

AGC_DECLARE(void) agc_event_destroy(agc_event_t **event);

AGC_DECLARE(agc_status_t) agc_event_add_header(agc_event_t *event, const char *header_name, const char *fmt, ...) PRINTF_FUNCTION(3, 4);

AGC_DECLARE(agc_status_t) agc_event_add_header_string(agc_event_t *event, const char *header_name, const char *data);

AGC_DECLARE(agc_status_t) agc_event_del_header(agc_event_t *event, const char *header_name);

AGC_DECLARE(agc_event_header_t *) agc_event_get_header(agc_event_t *event, const char *header_name);

AGC_DECLARE(agc_status_t) agc_event_add_body(agc_event_t *event, const char *fmt, ...) PRINTF_FUNCTION(2, 3);

AGC_DECLARE(agc_status_t) agc_event_set_body(agc_event_t *event, const char *body);

AGC_DECLARE(char *) agc_event_get_body(agc_event_t *event);

AGC_DECLARE(agc_status_t) agc_event_dup(agc_event_t **event, agc_event_t *todup);

AGC_DECLARE(void) agc_event_merge(agc_event_t *event, agc_event_t *tomerge);

//dispatch event
AGC_DECLARE(agc_status_t) agc_event_bind(const char *id, int event_id, const char *event_name, agc_event_callback_t callback, void *user_data);

AGC_DECLARE(agc_status_t) agc_event_bind_removable(const char *id, int event_id, const char *event_name,
															agc_event_callback_t callback, void *user_data, agc_event_node_t **node);

AGC_DECLARE(agc_status_t) agc_event_fire(agc_event_t **event);

AGC_DECLARE(agc_status_t) agc_event_unbind(agc_event_node_t **node);

AGC_DECLARE(agc_status_t) agc_event_unbind_callback(agc_event_callback_t callback);

AGC_DECLARE(agc_status_t) agc_event_serialize_json(agc_event_t *event, char **str);

AGC_DECLARE(agc_status_t) agc_event_create_json(agc_event_t **event, const char *json);

#endif
