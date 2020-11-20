#ifndef AGC_EVENT_H
#define AGC_EVENT_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

#define EVENT_NULL_SOURCEID 0
#define EVENT_ID_LIMIT 1024

#define EVENT_ID_ALL 0
#define EVENT_NAME_ALL "all"

#define EVENT_ID_EVENTSOCKET 1
#define EVENT_NAME_EVENTSOCKET "esl"

#define EVENT_ID_CMDRESULT 2
#define EVENT_NAME_CMDRESULT "api_res"

#define EVENT_ID_SIG2MEDIA 3
#define EVENT_NAME_SIG2MEDIA "sig2media"

#define EVENT_ID_MEDIA2SIG 4
#define EVENT_NAME_MEDIA2SIG "media2sig"

#define EVENT_ID_JSONCMD 5
#define EVENT_NAME_JSONCMD "om_cmd"

#define EVENT_ID_IS_INVALID(x) (x == EVENT_ID_ALL || x >= EVENT_ID_LIMIT)

#define EVENT_HEADER_ROUTING "_routingkey"
#define EVENT_HEADER_UUID "_uuid"
#define EVENT_HEADER_CODE "_code"
#define EVENT_HEADER_DESC "_desc"
#define EVENT_HEADER_SUBNAME "_subname" 
#define EVENT_HEADER_TYPE "_t"

#define EVENT_HEADER_TYPE_LEN 4

#define EVENT_FAST_HEADER_INDEX1 0

typedef enum {
	EVENT_FAST_HEADERINDEX1,
	EVENT_FAST_HEADERINDEX2,	
	EVENT_FAST_HEADERINDEX3,
	EVENT_FAST_HEADERINDEX4,
	EVENT_FAST_HEADERINDEX5,
	EVENT_FAST_HEADERINDEX6,
	EVENT_FAST_HEADERINDEX7,
} agc_fast_event_headerindex_t;

typedef enum {
	EVENT_FAST_TYPE_CallBack,
	EVENT_FAST_TYPE_S2M_UpfSetUpReq,
	EVENT_FAST_TYPE_S2M_UpfSetUpRsp,
	EVENT_FAST_TYPE_S2M_GnpSetUpReq,
	EVENT_FAST_TYPE_S2M_GnpSetUpRsp,
	EVENT_FAST_TYPE_S2M_SessCheckReq,
	EVENT_FAST_TYPE_S2M_SessCheckRsp,
	EVENT_FAST_TYPE_S2M_SessReleaseReq,
	EVENT_FAST_TYPE_S2M_SessReleaseRsp,
	EVENT_FAST_TYPE_S2M_SessReleaseInd,
	EVENT_FAST_TYPE_S2M_UpfUpdateReq,
	EVENT_FAST_TYPE_S2M_UpfUpdateRsp,
	EVENT_FAST_TYPE_S2M_GnpUpdateReq,
	EVENT_FAST_TYPE_S2M_GnpUpdateRsp,
	EVENT_FAST_TYPE_M2S_UpfSetUpReq,
	EVENT_FAST_TYPE_M2S_UpfSetUpRsp,
	EVENT_FAST_TYPE_M2S_GnpSetUpReq,
	EVENT_FAST_TYPE_M2S_GnpSetUpRsp,
	EVENT_FAST_TYPE_M2S_SessCheckReq,
	EVENT_FAST_TYPE_M2S_SessCheckRsp,
	EVENT_FAST_TYPE_M2S_SessReleaseReq,
	EVENT_FAST_TYPE_M2S_SessReleaseRsp,
	EVENT_FAST_TYPE_M2S_SessReleaseInd,
	EVENT_FAST_TYPE_M2S_UpfUpdateReq,
	EVENT_FAST_TYPE_M2S_UpfUpdateRsp,
	EVENT_FAST_TYPE_M2S_GnpUpdateReq,
	EVENT_FAST_TYPE_M2S_GnpUpdateRsp,
	EVENT_FAST_TYPE_Invalid
} agc_fast_event_types_t;

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

	int debug_id;
    
	/*! the event headers */
	agc_event_header_t *headers; 
    
	agc_event_header_t *last_header;
    
	 /*! the context of event */
	void *context;

	/*! the fast thing*/
	void *fast;
    
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

AGC_DECLARE(agc_status_t) agc_event_set_id(agc_event_t *event, int event_id);

AGC_DECLARE(agc_status_t) agc_event_create_callback(agc_event_t **event, uint32_t source_id, void *data, agc_event_callback_func callback);

AGC_DECLARE(void) agc_event_destroy(agc_event_t **event);

AGC_DECLARE(agc_status_t) agc_event_add_header(agc_event_t *event, const char *header_name, const char *fmt, ...) PRINTF_FUNCTION(3, 4);

AGC_DECLARE(agc_status_t) agc_event_add_header_repeatcheck(agc_event_t *event, const char *header_name, const char *fmt, ...) PRINTF_FUNCTION(3, 4);

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

AGC_DECLARE(agc_status_t) agc_event_create_json(agc_event_t **event, const char *json);

AGC_DECLARE(agc_status_t) agc_event_fast_initial(int fast_event_type, int event_id, int capacity, const char **headers, const int *valuelengths, int headernumbers, int bodylength);

AGC_DECLARE(agc_status_t) agc_event_fast_alloc(agc_event_t **event, int fast_event_type);

AGC_DECLARE(agc_status_t) agc_event_fast_create_callback(agc_event_t **event, void *data, agc_event_callback_func callback);

AGC_DECLARE(agc_status_t) agc_event_fast_set_strheader(agc_event_t *event, int index, const char *name, const char *value);

AGC_DECLARE(agc_status_t) agc_event_fast_set_intheader(agc_event_t *event, int index, const char *name, int value);

AGC_DECLARE(agc_status_t) agc_event_fast_set_uintheader(agc_event_t *event, int index, const char *name, uint32_t value);

AGC_DECLARE(agc_status_t) agc_event_fast_set_body(agc_event_t *event, const char *body, int len);

AGC_DECLARE(agc_status_t) agc_event_fast_get_type(agc_event_t *event, int *type);

AGC_DECLARE(agc_status_t) agc_event_fast_release(agc_event_t **event);

AGC_END_EXTERN_C

#endif
