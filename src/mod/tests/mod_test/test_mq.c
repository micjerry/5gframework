#include "mod_test.h"

#define TEST_MQ_EVENT_NAME "test_event"

#define TEST_MQ_EVENT_ID  21

void test_producer(agc_stream_handle_t *stream);
void test_customrouting(agc_stream_handle_t *stream);

void test_mq_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	test_producer(stream);
	test_customrouting(stream);
	//stream->write_function(stream, "not implement.\n");
}

void test_producer(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	if (agc_event_create(&new_event, EVENT_ID_CMDRESULT, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event create [fail].\n");
		return;
	} 

	if (agc_event_add_header(new_event, EVENT_HEADER_UUID, "cddfddd") != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer add header [fail].\n");
		return;
	}

	if (agc_event_set_body(new_event, "some thing") != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event set body [fail].\n");
		return;
	}

	if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event fire [fail].\n");
		agc_event_destroy(&new_event);
		return;
	}

	stream->write_function(stream, "test test_producer [ok].\n");
	return;
}

void test_customrouting(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	if (agc_event_create(&new_event, EVENT_ID_SIG2MEDIA, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event create [fail].\n");
		return;
	} 

	if (agc_event_add_header(new_event, EVENT_HEADER_ROUTING, "5gtest.api_res") != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer add header [fail].\n");
		return;
	}

	if (agc_event_set_body(new_event, "some thing") != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event set body [fail].\n");
		return;
	}

	if (agc_event_fire(&new_event) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event fire [fail].\n");
		agc_event_destroy(&new_event);
		return;
	}

	stream->write_function(stream, "test test_producer [ok].\n");
	return;
}



