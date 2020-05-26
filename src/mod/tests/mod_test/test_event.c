#include "mod_test.h"


typedef agc_status_t (*test_event_func) (agc_stream_handle_t *stream);

typedef struct test_event_command {
	char *pname;
	test_event_func func;
} test_event_command_t;

#define TEST_SOURCE_NAME "test"
static uint32_t g_source_id = 0;
static int g_event_id = 21;
#define TEST_EVENT_NAME "test_event"
#define TEST_HEADER_NAME "theader"
#define TEST_HEADER_FMTVALUE "river"
#define TEST_HEADER_VALUE "thvalue"
#define TEST_BIND_NAME "akls"

static agc_event_t *g_event = NULL;
static agc_event_node_t *event_node = NULL;

static void event_callback(void *data);
static void bind_callback(void *data);

static agc_status_t test_allocte_source(agc_stream_handle_t *stream) ;
static agc_status_t test_register(agc_stream_handle_t *stream);
static agc_status_t test_getid(agc_stream_handle_t *stream);
static agc_status_t test_getname(agc_stream_handle_t *stream);
static agc_status_t test_create(agc_stream_handle_t *stream);
static agc_status_t test_create_callback(agc_stream_handle_t *stream);
static agc_status_t test_add_header(agc_stream_handle_t *stream);
static agc_status_t test_add_header_str(agc_stream_handle_t *stream);
static agc_status_t test_del_header(agc_stream_handle_t *stream);
static agc_status_t test_get_header(agc_stream_handle_t *stream);
static agc_status_t test_add_body(agc_stream_handle_t *stream);
static agc_status_t test_set_body(agc_stream_handle_t *stream);
static agc_status_t test_get_body(agc_stream_handle_t *stream);
static agc_status_t test_dup(agc_stream_handle_t *stream);
static agc_status_t test_bind(agc_stream_handle_t *stream);
static agc_status_t test_unbind(agc_stream_handle_t *stream);
static agc_status_t test_bindremove(agc_stream_handle_t *stream);
static agc_status_t test_unbindremove(agc_stream_handle_t *stream);
static agc_status_t test_json(agc_stream_handle_t *stream);


test_event_command_t event_commands[] = {
	{"test_allocte_source", test_allocte_source},
	{"test_register", test_register},
	{"test_getid", test_getid},
	{"test_getname", test_getname},
	{"test_create", test_create},
	{"test_create_callback", test_create_callback},
	{"test_add_header", test_add_header},
	{"test_add_header_str", test_add_header_str},
	{"test_del_header", test_del_header},
	{"test_get_header", test_get_header},
	{"test_add_body", test_add_body},
	{"test_set_body", test_set_body},
	{"test_get_body", test_get_body},
	{"test_dup", test_dup},
	{"test_bind", test_bind},
	{"test_unbind", test_unbind},
	{"test_bindremove", test_bindremove},
	{"test_unbindremove", test_unbindremove},
	{"test_json", test_json}
};

#define TEST_EVENTCMD_SIZE (sizeof(event_commands)/sizeof(event_commands[0]))

void test_event_api(agc_stream_handle_t *stream, int argc, char **argv)
{	
	int i = 0;

	for (i = 0; i < TEST_EVENTCMD_SIZE; i++) {
		test_event_func pfunc = event_commands[i].func;
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "begin execute %s.\n", event_commands[i].pname);
		if (pfunc(stream) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "execute %s failed.\n", event_commands[i].pname);
			break;
		}
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "end execute %s.\n", event_commands[i].pname);
	}
}

static agc_status_t test_allocte_source(agc_stream_handle_t *stream) 
{
	g_source_id = agc_event_alloc_source(TEST_SOURCE_NAME);

	if (g_source_id > 0) {
		stream->write_function(stream, "test agc_event_alloc_source [ok].\n");
		return AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_alloc_source [fail].\n");
	}

	return AGC_STATUS_FALSE;
}

static agc_status_t test_register(agc_stream_handle_t *stream)
{
	if (agc_event_register(g_event_id, TEST_EVENT_NAME) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_register [ok].\n");
		return AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_register [fail].\n");
	}

	return AGC_STATUS_FALSE;
}

static agc_status_t test_getid(agc_stream_handle_t *stream)
{
	int event_id;

	 if (agc_event_get_id(TEST_EVENT_NAME, &event_id) != AGC_STATUS_SUCCESS) {
	 	stream->write_function(stream, "test agc_event_get_id [fail].\n");
		return AGC_STATUS_FALSE;
	 }

	if (event_id != g_event_id) {
		stream->write_function(stream, "test agc_event_get_id id error[fail].\n");
		return AGC_STATUS_FALSE;
	}

	stream->write_function(stream, "test agc_event_get_id [ok].\n");
	return AGC_STATUS_SUCCESS;
}

static agc_status_t test_getname(agc_stream_handle_t *stream)
{
	const char *result = NULL;

	result = agc_event_get_name(g_event_id);
	if (strcmp(result, TEST_EVENT_NAME) == 0) {
		stream->write_function(stream, "test agc_event_get_name [ok].\n");
		return AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_get_name [fail].\n");
	}

	return AGC_STATUS_FALSE;
}

static agc_status_t test_create(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) == AGC_STATUS_SUCCESS) && new_event) {
		stream->write_function(stream, "test agc_event_create [ok].\n");
	} else {
		stream->write_function(stream, "test agc_event_create [fail].\n");
		return satus;
	}

	agc_event_destroy(&new_event);

	if (new_event == NULL) {
		stream->write_function(stream, "test agc_event_destroy [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_destroy [fail].\n");
	}

	return satus;
}

static agc_status_t test_create_callback(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	if ((agc_event_create_callback(&new_event, g_source_id, NULL, event_callback)	== AGC_STATUS_SUCCESS) && new_event) {
		stream->write_function(stream, "test agc_event_create_callback [ok].\n");
	} else {
		stream->write_function(stream, "test agc_event_create_callback [fail].\n");
		return AGC_STATUS_FALSE;
	}

	agc_event_destroy(&new_event);

	return AGC_STATUS_SUCCESS;
}

static agc_status_t test_add_header(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_add_header [fail].\n");
		return satus;
	}
		
	if (agc_event_add_header(new_event, TEST_HEADER_NAME, "some thing %s", TEST_HEADER_FMTVALUE) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_add_header [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	}

	agc_event_destroy(&new_event);

	return satus;
}

static agc_status_t test_add_header_str(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_add_header_string [fail].\n");
		return satus;
	}
		
	if (agc_event_add_header_string(new_event, TEST_HEADER_NAME, "some thing") == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_add_header_string [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	}

	agc_event_destroy(&new_event);

	return satus;	
}

static agc_status_t test_del_header(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_del_header [fail].\n");
		return satus;
	}
		
	if (agc_event_add_header(new_event, TEST_HEADER_NAME, "some thing %s", TEST_HEADER_FMTVALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_del_header [fail].\n");
		agc_event_destroy(&new_event);
		return satus;
	}

	if (agc_event_del_header(new_event, TEST_HEADER_NAME) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_del_header [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_del_header [fail].\n");
	}
	
	agc_event_destroy(&new_event);

	return satus;	
}

static agc_status_t test_get_header(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;
	const char *value = NULL;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_get_header [fail].\n");
		return satus;
	}
		
	if (agc_event_add_header(new_event, TEST_HEADER_NAME, TEST_HEADER_VALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_get_header [fail].\n");
		agc_event_destroy(&new_event);
		return satus;
	}

	value = agc_event_get_header(new_event, TEST_HEADER_NAME);
	if (!value) {
		stream->write_function(stream, "test agc_event_get_header [fail].\n");
		agc_event_destroy(&new_event);
		return satus;
	}

	if (strcmp(value, TEST_HEADER_VALUE) == 0) {
		satus = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test agc_event_get_header [ok].\n");
	} else {
		stream->write_function(stream, "test agc_event_get_header [fail].\n");
	}
		
}

static agc_status_t test_add_body(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_add_body [fail].\n");
		return satus;
	}
		
	if (agc_event_add_body(new_event, "some thing %s", TEST_HEADER_FMTVALUE) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_add_body [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_add_body [fail].\n");
	}

	agc_event_destroy(&new_event);

	return satus;
}

static agc_status_t test_set_body(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_set_body [fail].\n");
		return satus;
	}

	if (agc_event_set_body(new_event, "some thing") == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_set_body [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_set_body [fail].\n");
	}

	agc_event_destroy(&new_event);
	return satus;
}

static agc_status_t test_get_body(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_status_t satus = AGC_STATUS_FALSE;
	const char *value = NULL;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_get_body [fail].\n");
		return satus;
	}

	if (agc_event_set_body(new_event, TEST_HEADER_VALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_get_body [fail].\n");
		agc_event_destroy(&new_event);
		return satus;
	} 

	value = agc_event_get_body(new_event);

	if (!value) {
		stream->write_function(stream, "test agc_event_get_body [fail].\n");
		agc_event_destroy(&new_event);
		return satus;
	}

	if (strcmp(value, TEST_HEADER_VALUE) == 0) {
		stream->write_function(stream, "test agc_event_get_body [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_get_body [fail].\n");
	}

	agc_event_destroy(&new_event);

	return satus;
	
}

static agc_status_t test_dup(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	agc_event_t *clone = NULL;
	agc_status_t status = AGC_STATUS_FALSE;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_dup [fail].\n");
		return status;
	}

	if ((agc_event_dup(&clone, new_event) == AGC_STATUS_SUCCESS) && clone){
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test agc_event_dup [ok].\n");
	} else {
		stream->write_function(stream, "test agc_event_dup [fail].\n");
	}

	if (new_event)
		agc_event_destroy(&new_event);

	if (clone)
		agc_event_destroy(&clone);

	return status;
}

static agc_status_t test_bind(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	if (agc_event_bind(TEST_BIND_NAME, g_event_id, bind_callback) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind [fail].\n");
		return AGC_STATUS_FALSE;
	} else {
		stream->write_function(stream, "test agc_event_bind [ok].\n");
	}

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_bind create event [fail].\n");
		return AGC_STATUS_FALSE;
	}

	if (agc_event_add_header(new_event, TEST_HEADER_NAME, TEST_HEADER_VALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind add header [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}	

	if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind event fire [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}

	stream->write_function(stream, "test agc_event_bind [ok].\n");
	agc_yield(500000); //wait execute
	return AGC_STATUS_SUCCESS;
}

static agc_status_t test_unbind(agc_stream_handle_t *stream)
{
	
	if (agc_event_unbind_callback(bind_callback) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_unbind_callback [ok].\n");
		return AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_unbind_callback [fail].\n");
		return AGC_STATUS_FALSE;
	}
}

static agc_status_t test_bindremove(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	if (agc_event_bind_removable(TEST_BIND_NAME, g_event_id, bind_callback, &event_node) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind_removable [fail].\n");
		return AGC_STATUS_FALSE;
	} 

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_bind_removable create event [fail].\n");
		return AGC_STATUS_FALSE;
	}

	if (agc_event_add_header(new_event, TEST_HEADER_NAME, TEST_HEADER_VALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind_removable add header [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}	

	if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_bind_removable event fire [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}

	stream->write_function(stream, "test agc_event_bind_removable [ok].\n");
	agc_yield(500000); //wait execute
	return AGC_STATUS_SUCCESS;
}

static agc_status_t test_unbindremove(agc_stream_handle_t *stream)
{
	if (agc_event_unbind(&event_node) == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_unbind [ok].\n");
		return AGC_STATUS_SUCCESS;
	} else {
		stream->write_function(stream, "test agc_event_unbind [fail].\n");
	}

	return AGC_STATUS_FALSE;
}

static agc_status_t test_json(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	char *result = NULL;

	if ((agc_event_create(&new_event, g_event_id, g_source_id) != AGC_STATUS_SUCCESS) ||! new_event) {
		stream->write_function(stream, "test agc_event_serialize_json_obj create event [fail].\n");
		return AGC_STATUS_FALSE;
	}

	if (agc_event_add_header(new_event, TEST_HEADER_NAME, TEST_HEADER_VALUE) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_serialize_json_obj add header [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}

	if (agc_event_set_body(new_event, "some thing") != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_serialize_json_obj set body [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	} 

	if (agc_event_serialize_json(new_event, &result) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_serialize_json_obj [fail].\n");
		agc_event_destroy(&new_event);
		return AGC_STATUS_FALSE;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "test agc_event_serialize_json_obj  %s .\n", result);
	agc_event_destroy(&new_event);
	stream->write_function(stream, "test agc_event_serialize_json_obj [ok].\n");
	return AGC_STATUS_SUCCESS;	
}

static void event_callback(void *data)
{
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "test callback [ok].\n");
}

static void bind_callback(void *data)
{
	agc_event_t *event = (agc_event_t *)data;
	const char *header = NULL;

	if (!event)
		return;

	header = agc_event_get_header(event, TEST_HEADER_NAME);
	if (header) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "bind_callback received event header %s.\n", header);
	}
	
}

