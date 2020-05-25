#include "mod_test.h"

#define TEST_SOURCE_NAME "test"
static uint32_t g_source_id = 0;
static int g_event_id = 21;
#define TEST_EVENT_NAME "test_event"
#define TEST_HEADER_NAME "theader"

static agc_event_t *g_event = NULL;

static void event_callback(void *data);

static agc_status_t test_allocte_source(agc_stream_handle_t *stream) ;
static agc_status_t test_register(agc_stream_handle_t *stream);
static agc_status_t test_getid(agc_stream_handle_t *stream);
static agc_status_t test_getname(agc_stream_handle_t *stream);
static agc_status_t test_create(agc_stream_handle_t *stream);
static agc_status_t test_create_callback(agc_stream_handle_t *stream);
static agc_status_t test_add_header(agc_stream_handle_t *stream);


void test_event_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	stream->write_function(stream, "not implement.\n");
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
	}
		
	if (agc_event_add_header(new_event, TEST_HEADER_NAME, "some thing") == AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test agc_event_add_header [ok].\n");
		satus = AGC_STATUS_SUCCESS;
	}

	agc_event_destroy(&new_event);

	return satus;
}

static void event_callback(void *data)
{
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "test callback [ok].\n");
}



