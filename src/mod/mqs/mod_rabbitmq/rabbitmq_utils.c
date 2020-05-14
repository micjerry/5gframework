#include "mod_rabbitmq.h"
#include <yaml.h>

static agc_status_t agcmq_load_profile(const char *filename);

agc_status_t agcmq_load_config()
{
	agc_dir_t *dir = NULL;
	char *dir_path = NULL;
	char buffer[256];
	const char *fname;
	const char *fname_ext;
	agc_status_t status;
	char *pname;
	char *ptype;
	agcmq_connection_info_t *conn;
	agcmq_conn_parameter_t *para;

	if ((status = agc_dir_open(&dir, dir_path, agcmq_global.pool)) != AGC_STATUS_SUCCESS) {
		return status;
	}

	while((fname = agc_dir_next_file(dir, buffer, sizeof(buffer)))) {
		if ((fname_ext = strrchr(fname, '.'))) {
			if (!strcmp(fname_ext, ".yml")) {
				if ((agcmq_load_profile(fname, &pname, &ptype, &conn, &para) != AGC_STATUS_SUCCESS)
					|| !pname || !ptype || !conn || !para) {
					agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Parse file %s failed.\n", fname);
				} else {
					if (!strcmp(ptype, "producer")) {
						
					} else if (!strcmp(ptype, "consumer")) {
						
					} else {
						agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Unknown profile %s.\n", fname);
					}
				}
			}
		}
	}
	
}

static agc_status_t agcmq_load_profile(const char *filename, 
										char **ppname,
										char **pptype;
										agcmq_connection_info_t **connection_info, 
										agcmq_conn_parameter_t **parameter)
{
	FILE *file;
	yaml_parser_t parser;
	yaml_token_t token;
	int done = 0;
	int error = 0;
	int iskey = 0;
	agcmq_connection_info_t *conn_header = NULL;
	agcmq_connection_info_t *conn = NULL;
	agcmq_conn_parameter_t *para = NULL;
	char **strvalue = NULL;
	unsigned int *intvalue = NULL;
	char *profile_name = NULL;
	char *profile_type = NULL;
	enum {
		BLOCK_CONNECTION,
		BLOCK_PARAMETER,
		BLOCK_UNKOWN
	} blocktype = BLOCK_UNKOWN;

	enum {
		KEY_STR,
		KEY_INT,
		KEY_UNKOWN
	} keytype = KEY_UNKOWN;

	if (!filename)
		return AGC_STATUS_GENERR;
		
	file = fopen(filename, "rb");
	assert(file);
	assert(yaml_parser_initialize(&parser));

	yaml_parser_set_input_file(&parser, file);

	while (!done)
	{
		if (!yaml_parser_scan(&parser, &token)) {
			error = 1;
			break;
	 	}

		switch(token.type) 
		{
			case YAML_KEY_TOKEN:
				iskey = 1;
				break;
			case YAML_VALUE_TOKEN:
				iskey = 0;
				break;

			case YAML_SCALAR_TOKEN:
				{
					if (iskey) {
						if (strcmp(token.data.scalar.value, "connections") == 0) {
							blocktype = BLOCK_CONNECTION;
						} else if (strcmp(token.data.scalar.value, "parameters") == 0) {
							blocktype = BLOCK_PARAMETER;
						} else if (strcmp(token.data.scalar.value, "name") == 0) {
							blocktype = BLOCK_UNKOWN;
							keytype = KEY_STR;
							strvalue = &profile_name;
						} else if (strcmp(token.data.scalar.value, "type") == 0) {
							blocktype = BLOCK_UNKOWN;
							keytype = KEY_STR;
							strvalue = &profile_type;
						} else if (strcmp(token.data.scalar.value, "hostname") == 0) {
							keytype = KEY_STR;
							strvalue = &conn->hostname;
						} else if (strcmp(token.data.scalar.value, "virtualhost") == 0) {
							keytype = KEY_STR;
							strvalue = &conn->virtualhost;
						} else if (strcmp(token.data.scalar.value, "username") == 0) {
							keytype = KEY_STR;
							strvalue = &conn->username;
						} else if (strcmp(token.data.scalar.value, "password") == 0) {
							keytype = KEY_STR;
							strvalue = &conn->password;
						} else if (strcmp(token.data.scalar.value, "port") == 0) {
							keytype = KEY_INT;
							intvalue = &conn->port;
						} else if (strcmp(token.data.scalar.value, "heartbeat") == 0) {
							keytype = KEY_INT;
							intvalue = &conn->heartbeat;
						} else if (strcmp(token.data.scalar.value, "exname") == 0) {
							keytype = KEY_STR;
							strvalue = &para->ex_name;
						} else if (strcmp(token.data.scalar.value, "extype") == 0) {
							keytype = KEY_STR;
							strvalue = &para->ex_type;
						} else if (strcmp(token.data.scalar.value, "circuit_breaker_ms") == 0) {
							keytype = KEY_INT;
							intvalue = &para->circuit_breaker_ms;
						} else if (strcmp(token.data.scalar.value, "reconnect_interval_ms") == 0) {
							keytype = KEY_INT;
							intvalue = &para->reconnect_interval_ms;
						} else if (strcmp(token.data.scalar.value, "queue_size") == 0) {
							keytype = KEY_INT;
							intvalue = &para->queue_size;
						} else if (strcmp(token.data.scalar.value, "event_filter") == 0) {
							keytype = KEY_STR;
							strvalue = &para->event_filter;
						} else if (strcmp(token.data.scalar.value, "binding_key") == 0) {
							keytype = KEY_STR;
							strvalue = &para->bind_key;
						} else {
							agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "unknown key %s found.\n", token.data.scalar.value);
						}
						
					} else {
					
						if (keytype == KEY_STR) {
							*strvalue = agc_core_strdup(agcmq_global.pool, token.data.scalar.value);
						} else if (keytype == KEY_INT) {
							*intvalue = agc_atoui(token.data.scalar.value);
						} 
					}
				}
				break;
			case YAML_BLOCK_ENTRY_TOKEN:
				{
					if (blocktype == BLOCK_CONNECTION) {
						if (!conn) {
							conn = agc_memory_alloc(agcmq_global.pool, sizeof(*conn));
							assert(conn);
						} else {
							conn->next = agc_memory_alloc(agcmq_global.pool, sizeof(*conn));
							assert(conn->next);
							conn = conn->next;
						}

						if (!conn_header) {
							conn_header = conn;
						} 
					} else if (blocktype == BLOCK_PARAMETER) {
						para = agc_memory_alloc(agcmq_global.pool, sizeof(*para));
						assert(para);
					}
				}
				break;
			default:
                		break; 
		}

		done = (token.type == YAML_STREAM_END_TOKEN);
		yaml_token_delete(&token);
	 }

	yaml_parser_delete(&parser);
	assert(!fclose(file));

	*connection_info = conn_header;
	*parameter = para;
	*ppname = profile_name;
	*pptype = profile_type;

	return AGC_STATUS_SUCCESS;
}

