#include <mod_eventsocket.h>
#include <yaml.h>

AGC_MODULE_LOAD_FUNCTION(mod_eventsocket_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_eventsocket_shutdown);
AGC_MODULE_RUNTIME_FUNCTION(mod_eventsocket_runtime);
AGC_MODULE_DEFINITION(mod_eventsocket, mod_eventsocket_load, mod_eventsocket_shutdown, mod_eventsocket_runtime);

#define EVENTSOCKET_CFG_FILE "event_socket.yml"
#define MAX_QUEUE_LEN 100000

static agc_memory_pool_t *module_pool = NULL;

static event_listener_t listener;

eventsocket_profile_t profile;

static agc_event_node_t *subscribe;

static agc_status_t load_configuration();

static void close_socket(agc_socket_t ** sock);

static void release_connection(event_connect_t **conn);

static void launch_connection_thread(event_connect_t *conn);

static void *connection_run(agc_thread_t *thread, void *obj);

static void add_conn(event_connect_t *conn);

static void remove_conn(event_connect_t *conn);

static void handle_event(void *data);

static void send_disconnect(event_connect_t *conn, const char *message);

static void kill_connections(void);

AGC_MODULE_LOAD_FUNCTION(mod_eventsocket_load)
{
    assert(pool);
    module_pool = pool;
    
    *module_interface = agc_loadable_module_create_interface(module_pool, modname);
    if (load_configuration() != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Configuration parse failed\n");
        return AGC_STATUS_GENERR;
    }
    
    memset(&listener, 0, sizeof(event_listener_t));
    agc_mutex_init(&listener.sock_mutex, AGC_MUTEX_NESTED, module_pool);
    agc_mutex_init(&profile.mutex, AGC_MUTEX_NESTED, module_pool);
    
    if (agc_event_bind_removable("eventsocket", EVENT_ID_ALL, handle_event, &subscribe) != AGC_STATUS_SUCCESS) {
         agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "%s subscribe event failed.\n", modname);
         return AGC_STATUS_GENERR;
    }
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "%s init success.\n", modname);
    
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_eventsocket_shutdown)
{
    int wait = 0;
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "%s shutdown success.\n", modname);
    
    profile.done = 1;
    
    agc_event_unbind(&subscribe);
    
    kill_connections();
    close_socket(&listener.sock);
    
    while (profile.threads) {
		agc_yield(100000);
		kill_connections();
		if (++wait >= 200) {
			break;
		}
	}
    
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_RUNTIME_FUNCTION(mod_eventsocket_runtime)
{
	agc_memory_pool_t *pool = NULL;
	agc_memory_pool_t *connection_pool = NULL;
	agc_status_t status;
	agc_sockaddr_t *sa;
	agc_socket_t *new_socket = NULL;
	event_connect_t *new_connection;
    
	while (!profile.done) {
		if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Memory alloc failed\n");
			profile.done = 1;
			break;
		}
    
		if (agc_sockaddr_info_get(&sa, profile.ip, AGC_UNSPEC, profile.port, 0, pool) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Get addr failed\n");
			profile.done = 1;
			break;
		}
    
		if (agc_socket_create(&listener.sock, agc_sockaddr_get_family(sa), SOCK_STREAM, AGC_PROTO_TCP, pool) != AGC_STATUS_SUCCESS) {
			profile.done = 1;
			break;
		}

		if (agc_socket_opt_set(listener.sock, AGC_SO_REUSEADDR, 1) != AGC_STATUS_SUCCESS) {
			profile.done = 1;
			break;
		}

		if (agc_socket_bind(listener.sock, sa) != AGC_STATUS_SUCCESS) {
			profile.done = 1;
			break;
		}

		if (agc_socket_listen(listener.sock, 5) != AGC_STATUS_SUCCESS) {
			profile.done = 1;
			break;
		}

		break;
    }
    
	if (profile.done) {
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Socket Error! Could not listen on %s:%u\n", profile.ip, profile.port);
		if (pool) {
			agc_memory_destroy_pool(&pool);
		}
		return AGC_STATUS_TERM;
	}
    
	listener.ready = 1;
    
	while (!profile.done) {
		if (agc_memory_create_pool(&connection_pool) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Memory alloc failed\n");
			profile.done = 1;
			break;
		}
        
		if (agc_socket_accept(&new_socket, listener.sock, connection_pool) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Accept connection failed\n");
			if (connection_pool)
				agc_memory_destroy_pool(&connection_pool);
			continue;
		}

        
		new_connection = agc_memory_alloc(connection_pool, sizeof(event_connect_t));
		agc_thread_rwlock_create(&new_connection->rwlock, connection_pool);
		agc_queue_create(&new_connection->event_queue, MAX_QUEUE_LEN, connection_pool);
        
		new_connection->sock = new_socket;
		new_connection->pool = connection_pool;
		connection_pool = NULL;
		
		agc_socket_create_pollset(&new_connection->pollfd, new_connection->sock, AGC_POLLIN | AGC_POLLERR, new_connection->pool);
		if (agc_socket_addr_get(&new_connection->sa, AGC_TRUE, new_connection->sock) == AGC_STATUS_SUCCESS && new_connection->sa) {
            		agc_get_addr(new_connection->remote_ip, sizeof(new_connection->remote_ip), new_connection->sa);
			
			if (new_connection->sa && (new_connection->remote_port = agc_sockaddr_get_port(new_connection->sa))) {
				launch_connection_thread(new_connection);
				continue;
			}
		}
        
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Initial connection failed\n");
		close_socket(&new_connection->sock);
		release_connection(&new_connection);
    }
    
    close_socket(&listener.sock);
    if (pool) {
        agc_memory_destroy_pool(&pool);
    }
    
    if (connection_pool) {
        agc_memory_destroy_pool(&connection_pool);
    }
    
    return AGC_STATUS_TERM;
}

static agc_status_t load_configuration()
{
    char *filename;
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
    int done = 0;
    int error = 0;
    int iskey = 0;
    enum {
        EVENTSOCKET_KEY_IP,
        EVENTSOCKET_KEY_PORT,
        EVENTSOCKET_KEY_UNKOWN
    } keytype = EVENTSOCKET_KEY_UNKOWN;
    
    memset(&profile, 0, sizeof(eventsocket_profile_t));
    filename = agc_mprintf("%s%s%s", AGC_GLOBAL_dirs.conf_dir, AGC_PATH_SEPARATOR, EVENTSOCKET_CFG_FILE);
    
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
                    if (iskey)
                    {
                        if (strcmp(token.data.scalar.value, "ip") == 0)
                        {
                            keytype = EVENTSOCKET_KEY_IP;
                        } else if (strcmp(token.data.scalar.value, "port") == 0)
                        {
                            keytype = EVENTSOCKET_KEY_PORT;
                        }
                    } else {
                        if (keytype == EVENTSOCKET_KEY_IP) 
                        {
                            strcpy(profile.ip, token.data.scalar.value);
                        } else if (keytype == EVENTSOCKET_KEY_PORT)
                        {
                            profile.port = agc_atoui(token.data.scalar.value);
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
    
    agc_safe_free(filename);
    
    return error ? AGC_STATUS_GENERR: AGC_STATUS_SUCCESS;
}

static void close_socket(agc_socket_t ** sock)
{
	agc_mutex_lock(listener.sock_mutex);
	
	if (*sock) {
		agc_socket_shutdown(*sock, AGC_SHUTDOWN_READWRITE);
		agc_socket_close(*sock);
		*sock = NULL;
	}
	
	agc_mutex_unlock(listener.sock_mutex);
}

static void release_connection(event_connect_t **conn)
{
	event_connect_t *l_conn = *conn;

	if (!conn || !l_conn)
		return;

	if (l_conn->pool)
		agc_memory_destroy_pool(&l_conn->pool);

	if (l_conn->thread_pool)
		agc_memory_destroy_pool(&l_conn->thread_pool);
	
	*conn = NULL;
}

static void launch_connection_thread(event_connect_t *conn)
{
	agc_threadattr_t *thd_attr = NULL;

	//thread and socket can not share one pool
	if (agc_memory_create_pool(&conn->thread_pool)  != AGC_STATUS_SUCCESS) {
		return;
	}
	agc_threadattr_create(&thd_attr, conn->thread_pool);
	agc_threadattr_detach_set(thd_attr, 1);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
	agc_thread_create(&conn->conn_thread, thd_attr, connection_run, conn, conn->thread_pool);
}

static void *connection_run(agc_thread_t *thread, void *obj)
{
	char buf[1024];
	char reply[512] = "";
	agc_size_t len;
	event_connect_t *conn = (event_connect_t *)obj;
	agc_event_t *revent = NULL;
	int rc;
	sigset_t signal_mask;
    
	assert(conn != NULL);

	//disable SIGPIPE, otherwise write to a closed socket will crash
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGPIPE);
	rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	if (rc != 0) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Block SIGPIPE failed\n");
	}
    
	agc_mutex_lock(profile.mutex);
	profile.threads++;
	agc_mutex_unlock(profile.mutex);
    
	agc_socket_opt_set(conn->sock, AGC_SO_TCP_NODELAY, TRUE);
	agc_socket_opt_set(conn->sock, AGC_SO_NONBLOCK, TRUE);
    
	conn->is_running = 1;
	add_conn(conn);
    
	while (!profile.done && listener.ready && conn->is_running) {
		len = sizeof(buf);
		memset(buf, 0, len);

		if (read_packet(conn, &revent) != AGC_STATUS_SUCCESS) {
			break;
		}

		if (!revent) {
			continue;
		}
        
		if (parse_command(conn, &revent, reply, sizeof(reply)) != AGC_STATUS_SUCCESS ) {
			conn->is_running = 0;
			break;
		}
	        
		if (revent) {
			agc_event_destroy(&revent);
		}
		
		//TODO
        
		if (*reply != '\0') {
			if (*reply == '~') {
				agc_snprintf(buf, sizeof(buf), "Content-Type: command/reply\n%s", reply + 1);
			} else {
				agc_snprintf(buf, sizeof(buf), "Content-Type: command/reply\nReply-Text: %s\n\n", reply);
			}
			len = strlen(buf);
			agc_socket_send(conn->sock, buf, &len);
		} 
	}
    
	if (revent) {
		agc_event_destroy(&revent);
	}
    
	send_disconnect(conn, "Disconnected, goodbye.\n");
	remove_conn(conn);
	close_socket(&conn->sock);
	release_connection(&conn);
    
	agc_mutex_lock(profile.mutex);
	profile.threads--;
	agc_mutex_unlock(profile.mutex);
    
	return NULL;
}

static void add_conn(event_connect_t *conn)
{
	agc_mutex_lock(listener.sock_mutex);
	conn->next = listener.connections;
	listener.connections = conn;
	agc_mutex_unlock(listener.sock_mutex);
}

static void remove_conn(event_connect_t *conn)
{
	event_connect_t *l_conn = NULL;
	event_connect_t *last_conn = NULL;
    
	agc_mutex_lock(listener.sock_mutex);
	for (l_conn = listener.connections; l_conn; l_conn = l_conn->next) {
		if (l_conn == conn) {
			if (last_conn) {
				last_conn->next = l_conn->next;
			} else {
				listener.connections = l_conn->next;
			}
			
			break;
		}
	    
		last_conn = l_conn;
	}

	agc_mutex_unlock(listener.sock_mutex);
}

static void handle_event(void *data)
{
	agc_event_t *event = (agc_event_t *)data;
	agc_event_t *clone = NULL;
	event_connect_t *c, *cp, *last = NULL;
    
	assert(event != NULL);
    
	if (!listener.ready) {
		return;
	}
    
	agc_mutex_lock(listener.sock_mutex);
	cp = listener.connections;
    
	while (cp) {
		c = cp;
		cp = cp->next;
        
		if (c->has_event && c->event_list[event->event_id]) {
			if (agc_event_dup(&clone, event) == AGC_STATUS_SUCCESS) {
				if (agc_queue_trypush(c->event_queue, clone) != AGC_STATUS_SUCCESS) {
					agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Send event failed.\n");
					agc_event_destroy(&clone);
				}
			}
		}
	}
    
	agc_mutex_unlock(listener.sock_mutex);
}

static void send_disconnect(event_connect_t *conn, const char *message)
{
	char disco_buf[512] = "";
	agc_size_t len = 0, mlen = 0;

	if (zstr(message)) {
		message = "Disconnected.\n";
	}

	mlen = strlen(message);
	
	agc_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: text/disconnect-notice\nContent-Length: %d\n\n", (int)mlen);

	if (!conn->sock) return;

	len = strlen(disco_buf);
	if (agc_socket_send(conn->sock, disco_buf, &len) == AGC_STATUS_SUCCESS) {
		if (len > 0) {
			len = mlen;
			agc_socket_send(conn->sock, message, &len);
		}
	}
}

static void kill_connections(void)
{
	event_connect_t *conn = NULL;

	agc_mutex_lock(listener.sock_mutex);
	
	for (conn = listener.connections; conn; conn = conn->next) {
		send_disconnect(conn, "The system is being shut down.\n");
		conn->is_running = 0;
		if (conn->sock) {
			agc_socket_shutdown(conn->sock, AGC_SHUTDOWN_READWRITE);
			agc_socket_close(conn->sock);
		}
	}
	
	agc_mutex_unlock(listener.sock_mutex);
}
