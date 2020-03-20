#include <agc.h>

#define LIMIT_CONNECTIONS 1000000

static int genid = 0;
static agc_mutex_t *GENID_MUTEX = NULL;
static agc_memory_pool_t *RUNTIME_POOL = NULL;

static int next_id();

AGC_DECLARE(agc_status_t) agc_conn_init(agc_memory_pool_t *pool)
{
    assert(pool);
    RUNTIME_POOL = pool;
    agc_mutex_init(&GENID_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Connection init success.\n");
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_conn_shutdown(void)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Connection shutdown success.\n");
    
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_listening_t *) agc_conn_create_listening(agc_std_sockaddr_t *addr, 
                                                         int socklen, 
                                                         agc_memory_pool_t *pool,
                                                         agc_connection_handler_func handler);
{
    agc_listening_t *listening;
    struct sockaddr *sa;
    assert(pool);
    
    listening = agc_memory_alloc(pool, sizeof(agc_listening_t));
    if (listening == NULL)
        return NULL;
    
    sa = agc_memory_alloc(pool, socklen);
    
    if (sa == NULL)
        return NULL;
    
    memcpy(sa, addr, socklen);
    listening->sockaddr = sa;
    listening->socklen = socklen;
    listening->handler = handler;
    listening->fd = (agc_std_socket_t)-1;
    listening->pool = pool;
    
    return listening;
}

AGC_DECLARE(agc_connection_t *) agc_create_connection(agc_std_socket_t s, 
                                                      agc_std_sockaddr_t *addr, 
                                                      int socklen, 
                                                      agc_memory_pool_t *pool,
                                                      void *context, 
                                                      agc_routine_handler_func read,
                                                      agc_routine_handler_func write,
                                                      agc_routine_handler_func err)
{
    struct sockaddr *sa;
    agc_connection_t *new_connection;
    agc_routine_t *routine;
    
    assert(pool);
    
    new_connection = agc_memory_alloc(pool, sizeof(agc_connection_t));
    
    if (new_connection == NULL)
        return NULL;
    
    routine = agc_memory_alloc(pool, sizeof(agc_routine_t));
    
    if (routine == NULL)
        return NULL;
    
    routine->data = new_connection;
    routine->read_handle = read;
    routine->write_handle = write;
    routine->err_handle = err;
    
    new_connection->fd = s;
    new_connection->pool = pool;
    new_connection->context = context;
    new_connection->routine = routine;
    new_connection->listening = NULL;
    new_connection->id = next_id();
    
    sa = agc_memory_alloc(pool, socklen);
    memcpy(sa, addr, socklen);
    
    new_connection->sockaddr = sa;
    new_connection->socklen = socklen;
    
    return new_connection;
}

AGC_DECLARE(agc_connection_t *)  agc_get_connection(agc_listening_t *listening)
{
    agc_connection_t *new_connection;
    assert(listening);
    assert(listening->pool);
    
    if (listening->fd == (agc_std_socket_t) -1)
        return NULL;
    
    new_connection = agc_memory_alloc(listening->pool, sizeof(agc_connection_t));
    if (new_connection == NULL)
        return NULL;
    
    new_connection->fd = listening->fd;
    new_connection->listening = listening;
    new_connection->pool = listening->pool;
    new_connection->sockaddr = listening->sockaddr;
    new_connection->socklen = listening->socklen;
    new_connection->routine = NULL;
    new_connection->id = next_id();
    
    return new_connection;
}

AGC_DECLARE(void) agc_free_connection(agc_connection_t *c)
{
    if (!c)
        return;
    
    agc_memory_destroy_pool(c->pool);
}

static int next_id()
{
    int nextid = 0;
    
    agc_mutex_lock(GENID_MUTEX);
    if (genid > LIMIT_CONNECTIONS)
        genid = 0;
    nextid = genid++;
    agc_mutex_unlock(GENID_MUTEX);
    
    return nextid;
}