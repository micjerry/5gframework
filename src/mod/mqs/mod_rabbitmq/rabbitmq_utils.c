#include "mod_rabbitmq.h"
#include <yaml.h>

#define  AGCMQ_CFG_PATH "mq_profiles"
#define AGCMQ_MAX_FRAME 131072

static agc_status_t agcmq_load_profile(const char *filename, 
										char **ppname,
										char **pptype,
										agcmq_connection_info_t **connection_info, 
										agcmq_conn_parameter_t **parameter,
										agc_memory_pool_t *pool);

agc_status_t agcmq_load_config()
{
	agc_memory_pool_t *pool;
	agc_memory_pool_t *loop_pool;
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
	
	if (agc_memory_create_pool(&loop_pool) != AGC_STATUS_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Alloc dir memory failed.\n");
		return;
	}

	dir_path = agc_mprintf("%s%s%s", AGC_GLOBAL_dirs.conf_dir, AGC_PATH_SEPARATOR, AGCMQ_CFG_PATH);
	
	if ((status = agc_dir_open(&dir, dir_path, loop_pool)) != AGC_STATUS_SUCCESS) {
		agc_memory_destroy_pool(&loop_pool);
		agc_safe_free(dir_path);
		return status;
	}

	while((fname = agc_dir_next_file(dir, buffer, sizeof(buffer)))) {
		if ((fname_ext = strrchr(fname, '.'))) {
			if (!strcmp(fname_ext, ".yml")) {
				char *full_filename = NULL;
				
				if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Alloc memory failed.\n");
					break;
				}

				full_filename = agc_mprintf("%s%s%s", dir_path, AGC_PATH_SEPARATOR, fname);
				if (!full_filename) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Alloc memory failed.\n");
					agc_memory_destroy_pool(&pool);
					break;
				}
				
				if ((agcmq_load_profile(full_filename, &pname, &ptype, &conn, &para, pool) != AGC_STATUS_SUCCESS)
					|| !pname || !ptype || !conn || !para) {
					agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Parse file %s failed.\n", fname);
					agc_memory_destroy_pool(&pool);
				} else {
					if (!strcmp(ptype, "producer")) {
						agcmq_producer_create(pname, conn, para, pool);
					} else if (!strcmp(ptype, "consumer")) {
						agcmq_consumer_create(pname, conn, para, pool);
					} else {
						agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Unknown profile %s.\n", fname);
						agc_memory_destroy_pool(&pool);
					}
				}

				agc_safe_free(full_filename);
			}
		}
	}

	agc_memory_destroy_pool(&loop_pool);

	agc_safe_free(dir_path);
}

agc_status_t agcmq_connection_open(agcmq_connection_info_t *conn_infos, agcmq_connection_t *conn, char *profile_name)
{
	amqp_socket_t *socket = NULL;
	agcmq_connection_info_t *conn_info;
	int amqp_status = -1;
	amqp_rpc_reply_t reply;
	amqp_connection_state_t oldConnection;

	if (!conn_infos || !conn)
		return AGC_STATUS_GENERR;

	oldConnection = conn->state;
	if (oldConnection) {
		amqp_destroy_connection(oldConnection);
	}
	
	conn->state= amqp_new_connection();
	socket = amqp_tcp_socket_new(conn->state);

	if (!socket) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create amqp socket failed.\n");
		return AGC_STATUS_GENERR;
	}

	conn_info = conn_infos;
	while (conn_info) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Profile[%s] trying to connect amqp %s:%d.\n", profile_name, conn_info->hostname, conn_info->port);

		if ((amqp_status = amqp_socket_open(socket, conn_info->hostname, conn_info->port))) {
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Profile[%s] could not connect amqp %s:%d status(%d) %s.\n", profile_name, conn_info->hostname, conn_info->port, amqp_status, amqp_error_string2(amqp_status));
			conn_info = conn_info->next;
		} else {
			break;
		}
	}

	if (!conn_info) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Profile[%s] could not connect to any amqp.\n", profile_name);
		return AGC_STATUS_GENERR;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Profile[%s] connect amqp %s:%d success.\n", profile_name, conn_info->hostname, conn_info->port);

	reply = amqp_login(conn->state, conn_info->virtualhost, 0, AGCMQ_MAX_FRAME, 0, AMQP_SASL_METHOD_PLAIN,
                               conn_info->username, conn_info->password);

	if (agcmq_parse_amqp_reply(reply, "Logining in")) {
		agcmq_connection_close(conn);
		return AGC_STATUS_GENERR;
	}
	
	amqp_channel_open(conn->state, 1);
	
	if (agcmq_parse_amqp_reply(reply, "Openning channel")) {
		return AGC_STATUS_GENERR;
	}

	conn->active = AGC_TRUE;

	return AGC_STATUS_SUCCESS;
}

void agcmq_connection_close(agcmq_connection_t *pconn)
{
	int status = 0;
	
	if (pconn != NULL) {
		agcmq_parse_amqp_reply(amqp_channel_close(pconn->state, 1, AMQP_REPLY_SUCCESS), "Closing channel");
		agcmq_parse_amqp_reply(amqp_connection_close(pconn->state, AMQP_REPLY_SUCCESS), "Closing connection");

		if ((status = amqp_destroy_connection(pconn->state))) {
			const char *errstr = amqp_error_string2(-status);
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Error destroying amqp connection: %s\n", errstr);
		}

		pconn->active = AGC_FALSE;
	}
}

int agcmq_parse_amqp_reply(amqp_rpc_reply_t x, char const *context)
{
	switch (x.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		return 0;

	case AMQP_RESPONSE_NONE:
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: missing RPC reply type!.\n", context);
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: %s.\n", context, amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) {
		case AMQP_CONNECTION_CLOSE_METHOD: {
			amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: server connection error %d, message: %.*s\n",  
						context, m->reply_code, (int) m->reply_text.len, (char *) m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD: {
			amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: server channel error %d, message: %.*s\n", 
						context, m->reply_code, (int) m->reply_text.len, (char *) m->reply_text.bytes);
			break;
		}
		default:
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: unknown server error, method id 0x%08X\n", 
						context, x.reply.id);
			break;
		}
		break;

	default:
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "%s: unknown reply_type: %d \n", context, x.reply_type);
		break;
	}

	return -1;
}

static agc_status_t agcmq_load_profile(const char *filename, 
										char **ppname,
										char **pptype,
										agcmq_connection_info_t **connection_info, 
										agcmq_conn_parameter_t **parameter,
										agc_memory_pool_t *pool)
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

	para = agc_memory_alloc(pool, sizeof(*para));
	assert(para);
	para->delivery_timestamp = 1;
	para->delivery_mode = 0;
		
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
							keytype = KEY_UNKOWN;
						} else if (strcmp(token.data.scalar.value, "parameters") == 0) {
							blocktype = BLOCK_PARAMETER;
							keytype = KEY_UNKOWN;
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
						} else if (strcmp(token.data.scalar.value, "delivery_mode") == 0) {
							keytype = KEY_INT;
							intvalue = &para->delivery_mode;
						} else if (strcmp(token.data.scalar.value, "delivery_timestamp") == 0) {
							keytype = KEY_INT;
							intvalue = &para->delivery_timestamp;
						} else {
							keytype = KEY_UNKOWN;
							agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "unknown key %s found.\n", token.data.scalar.value);
						}

						//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "key %s found.\n", token.data.scalar.value);
						
					} else {
						//agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "value %s found.\n", token.data.scalar.value);
						if (keytype == KEY_STR) {
							*strvalue = agc_core_strdup(pool, token.data.scalar.value);
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
							conn = agc_memory_alloc(pool, sizeof(*conn));
							assert(conn);
						} else {
							conn->next = agc_memory_alloc(pool, sizeof(*conn));
							assert(conn->next);
							conn = conn->next;
						}

						if (!conn_header) {
							conn_header = conn;
						} 
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

	if (para->reconnect_interval_ms == 0) {
		para->reconnect_interval_ms = 1000;
	}

	for (conn = conn_header; conn; conn = conn->next) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "connect %s:%d  %s found.\n", conn->hostname, conn->port, conn->virtualhost);
	}
	
	return AGC_STATUS_SUCCESS;
}



