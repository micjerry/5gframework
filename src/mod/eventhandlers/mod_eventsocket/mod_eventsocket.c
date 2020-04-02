#include <mod_eventsocket.h>

AGC_MODULE_LOAD_FUNCTION(mod_eventsocket_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_eventsocket_shutdown);
AGC_MODULE_RUNTIME_FUNCTION(mod_eventsocket_runtime);
AGC_MODULE_DEFINITION(mod_eventsocket, mod_eventsocket_load, mod_eventsocket_shutdown, mod_eventsocket_runtime);

#define EVENTSOCKET_CFG_FILE "event_socket.yml"
#define MAX_QUEUE_LEN 100000

static agc_memory_pool_t *module_pool = NULL;

static event_listener_t listener;

static eventsocket_profile_t profile;

static agc_status_t load_configuration();

static void close_socket(agc_socket_t ** sock);

static agc_status_t release_connection(event_connect_t **conn);

static void launch_connection_thread(event_connect_t *conn);

static void *connection_run(agc_thread_t *thread, void *obj);

static void add_conn(event_connect_t *conn);

static void remove_conn(event_connect_t *conn);

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
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "%s init success.\n", modname);
    
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_eventsocket_shutdown)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "%s shutdown success.\n", modname);
    
    profile.done = 1;
    
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_RUNTIME_FUNCTION(mod_eventsocket_runtime)
{
    agc_memory_pool_t *pool = NULL, *connection_pool = NULL;
    agc_status_t status;
    agc_sockaddr_t *sa;
    agc_socket_t *new_socket = NULL;
    event_connect_t *new_connection;
    
    while (!profile.done) {
        if (agc_core_new_memory_pool(&pool) != AGC_STATUS_SUCCESS) {
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
        if (agc_core_new_memory_pool(&connection_pool) != AGC_STATUS_SUCCESS) {
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
        if (agc_socket_addr_get(&new_connection->sa, AGC_TRUE, new_connection->sock) == AGC_STATUS_SUCCESS && listener->sa) {
            agc_get_addr(new_connection->remote_ip, sizeof(new_connection->remote_ip), new_connection->sa);
            if (new_connection->sa && (new_connection->remote_port = agc_sockaddr_get_port(new_connection->sa))) {
                launch_connection_thread(new_connection);
                continue;
            }
        }
        
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Initial connection failed\n");
        close_socket(&new_connection->sock);
        release_connection(new_connection);
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

static agc_status_t release_connection(event_connect_t **conn)
{
    event_connect_t *l_conn = *conn;
    
    if (!conn || !l_conn)
        return AGC_STATUS_FALSE;
    
    agc_memory_destroy_pool(&l_conn->pool);
    *conn = NULL;
    return AGC_STATUS_SUCCESS;
}

static void launch_connection_thread(event_connect_t *conn)
{
    agc_thread_t *thread;
	agc_threadattr_t *thd_attr = NULL;

	agc_threadattr_create(&thd_attr, conn->pool);
	agc_threadattr_detach_set(thd_attr, 1);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
	agc_thread_create(&thread, thd_attr, connection_run, conn, conn->pool);
}

static void *connection_run(agc_thread_t *thread, void *obj)
{
    char buf[1024];
    agc_size_t len;
    event_connect_t *conn = (event_connect_t *)obj;
    agc_event_t *revent = NULL;
    
    assert(conn != NULL);
    
    agc_socket_opt_set(conn->sock, AGC_SO_TCP_NODELAY, TRUE);
	agc_socket_opt_set(conn->sock, AGC_SO_NONBLOCK, TRUE);
    
    conn->is_running = 1;
    add_conn(conn);
    
    while (!profile.done && listener.ready) {
        len = sizeof(buf);
        memset(buf, 0, len);
        
        status = read_packet(conn, &revent, 0);
        
        if (status != AGC_STATUS_SUCCESS) {
			break;
		}

		if (!revent) {
			continue;
		}
        
        
    }
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
    event_connect_t *l_conn, *last_conn;
    
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
