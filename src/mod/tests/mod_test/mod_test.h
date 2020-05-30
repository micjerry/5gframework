#ifndef MOD_TEST_H
#define MOD_TEST_H

#include <agc.h>

struct {
	agc_memory_pool_t *pool;
} test_global;

#define TEST_API_SIZE (sizeof(test_api_commands)/sizeof(test_api_commands[0]))

typedef void (*test_api_func) (agc_stream_handle_t *stream, int argc, char **argv);

typedef struct test_api_command {
	char *pname;
	test_api_func func;
	char *pcommand;
	char *psyntax;
} test_api_command_t;

extern test_api_command_t test_api_commands[];

agc_status_t test_main_real(const char *cmd, agc_stream_handle_t *stream);

void test_cache_api(agc_stream_handle_t *stream, int argc, char **argv);
void test_driver_api(agc_stream_handle_t *stream, int argc, char **argv);
void test_event_api(agc_stream_handle_t *stream, int argc, char **argv);
void test_mq_api(agc_stream_handle_t *stream, int argc, char **argv);
void test_timer_api(agc_stream_handle_t *stream, int argc, char **argv);
void test_db_api(agc_stream_handle_t *stream, int argc, char **argv);

#endif