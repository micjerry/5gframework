#include "mod_test.h"

static void test_set(agc_stream_handle_t *stream, int argc, char **argv);
static void test_get(agc_stream_handle_t *stream, int argc, char **argv);

void test_cache_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	if (argc && argv[0]) {
		if (strcasecmp(argv[0], "set") == 0) {
			return test_set(stream, argv);
		} else if (strcasecmp(argv[0], "get") == 0) {
			return test_get(stream, argv);
		}
	} else {
		//todo test all
	}
}

static void test_set(agc_stream_handle_t *stream, int argc, char **argv)
{
	if (argc == 3) {
		if (agc_cache_set(argv[1], argv[2], strlen(argv[2]), 0) == AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "set success.\n");
		} else {
			stream->write_function(stream, "set failed.\n");
		}

		return;
	}

	if (argc == 4) {
		if (agc_cache_set(argv[1], argv[2], strlen(argv[2]), atoi(argv[3])) == AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "set success.\n");
		} else {
			stream->write_function(stream, "set failed.\n");
		}

		return;
	}

	stream->write_function(stream, "invalid set command.\n");		
}

static void test_get(agc_stream_handle_t *stream, int argc, char **argv)
{
	char *result = NULL;
	int len = 0;
	
	if (argc == 2) {
		if (agc_cache_get(argv[1], &result, &len) ==AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "get success %*.s.\n", len, result);
		} else {
			stream->write_function(stream, "get failed.\n");
		}
		
		agc_safe_free(result);
		return;
	}

	stream->write_function(stream, "invalid set command.\n");		
}

