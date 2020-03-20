#ifndef AGC_CONNECTION_H
#define AGC_CONNECTION_H

struct agc_connection_s {
    //service context
    void *context;
    
    agc_routine_t *routine;
    
    agc_std_socket_t fd;
    
    agc_listening_t *listening;
    
    agc_memory_pool_t *pool;
    
    agc_std_sockaddr_t *sockaddr;
    
    int socklen;
    
    int thread_index;
    
    //the unique id of connection
    int id;
};

struct agc_listening_s {
    agc_std_socket_t fd;
    
    agc_std_sockaddr_t *sockaddr;
    
    int socklen;
    
    agc_connection_handler_func handler;
    
    agc_memory_pool_t *pool;
};

AGC_DECLARE(agc_status_t) agc_conn_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_conn_shutdown(void);

AGC_DECLARE(agc_listening_t *) agc_conn_create_listening(agc_std_sockaddr_t *addr, 
                                                         int socklen, 
                                                         agc_memory_pool_t *pool,
                                                         agc_connection_handler_func handler);

AGC_DECLARE(agc_connection_t *) agc_create_connection(agc_std_socket_t s, 
                                                      agc_std_sockaddr_t *addr, 
                                                      int socklen, 
                                                      agc_memory_pool_t *pool,
                                                      void *context, 
                                                      agc_routine_handler_func read,
                                                      agc_routine_handler_func write,
                                                      agc_routine_handler_func err);

AGC_DECLARE(agc_connection_t *)  agc_get_connection(agc_listening_t *listening);

AGC_DECLARE(void) agc_free_connection(agc_connection_t *c);

#endif