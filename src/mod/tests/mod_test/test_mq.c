#include "mod_test.h"

#define TEST_MQ_EVENT_NAME "test_event"

#define TEST_MQ_EVENT_ID  21

void test_producer(agc_stream_handle_t *stream);

void test_mq_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	test_producer(stream);
	//stream->write_function(stream, "not implement.\n");
}

void test_producer(agc_stream_handle_t *stream)
{
	agc_event_t *new_event = NULL;
	
	agc_event_register(TEST_MQ_EVENT_ID, TEST_MQ_EVENT_NAME);
	
	if (agc_event_create(&new_event, TEST_MQ_EVENT_ID, EVENT_NULL_SOURCEID) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_producer event create [fail].\n");
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

