#include "mod_test.h"

typedef struct {
	int intvalue;
	char *value;
} test_timer_data_t;

static void timer_callback(void *data);

#define TIMER_EVENT_NAME "timer_event"

static int g_timer_event_id = 22;

void test_timer_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_event_t *new_event = NULL;
	test_timer_data_t *data = NULL;

	agc_event_register(g_timer_event_id, TIMER_EVENT_NAME); 

	data = malloc(sizeof(test_timer_data_t));
	memset(data, 0, sizeof(test_timer_data_t));
	data->intvalue = 999;

	assert(data);

	if (agc_event_create_callback(&new_event, EVENT_NULL_SOURCEID, (void *)data, timer_callback) != AGC_STATUS_SUCCESS) {
		if (stream)
			stream->write_function(stream, "test timer agc_event_create [fail].\n");
		return;
	} 

	agc_timer_add_timer(new_event, 10000);

	if (stream)
		stream->write_function(stream, "test timer [ok].\n");

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "test_timer_api add timer.\n");
	
}

static void timer_callback(void *data)
{
	test_timer_data_t *timer_data = (test_timer_data_t *)data;

	if (!timer_data) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "timer_callback invalid data.\n");
		return;
	}
	
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "timer_callback %d .\n", timer_data->intvalue);
	agc_safe_free(data);
}
