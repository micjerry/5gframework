#include "mod_test.h"

AGC_MODULE_LOAD_FUNCTION(mod_test_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_test_shutdown);
AGC_MODULE_DEFINITION(mod_test, mod_test_load, mod_test_shutdown, NULL);

test_api_command_t test_api_commands[] = {
	{"cache", test_cache_api, "cache", ""},
	{"driver", test_driver_api, "driver", ""},
	{"event", test_event_api, "event", ""},
	{"mq", test_mq_api, "mq", ""}
};

AGC_STANDARD_API(test_api_main)
{
	return test_main_real(cmd, stream);
}

AGC_MODULE_LOAD_FUNCTION(mod_test_load)
{
	memset(&test_global, 0, sizeof(test_global));
	test_global.pool = pool;
	
	*module_interface = agc_loadable_module_create_interface(test_global.pool, modname);

	agc_api_register("test", "test API", "syntax", test_api_main);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Test init success.\n");

	return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_test_shutdown)
{
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Test shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

agc_status_t test_main_real(const char *cmd, agc_stream_handle_t *stream)
{
	char *cmdbuf = NULL;
	agc_status_t status = AGC_STATUS_SUCCESS;
	int argc, i;
	int found = 0;
	char *argv[25] = { 0 };

	if (!(cmdbuf = strdup(cmd))) {
		return status;
	}
	
	argc = agc_separate_string(cmdbuf , ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc && argv[0]) {
		for (i = 0; i < TEST_API_SIZE; i++) {
			if (strcasecmp(argv[0], test_api_commands[i].pname) == 0) {
				test_api_func pfunc = test_api_commands[i].func;
				int sub_argc = argc - 1;
				char **sub_argv = NULL;

				if (argc > 1)
					sub_argv = &argv[1];
				
				pfunc(stream, sub_argc, sub_argv);

				found = 1;
				break;
			}
		}

		if (!found) {
			status = AGC_STATUS_GENERR;
			stream->write_function(stream, "command '%s' not found.\n", argv[0]);
		} 
	}

	agc_safe_free(cmdbuf);
	
	return status;
	
}

