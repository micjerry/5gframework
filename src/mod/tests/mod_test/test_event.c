#include "mod_test.h"

void test_event_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	stream->write_function(stream, "not implement.\n");
}
