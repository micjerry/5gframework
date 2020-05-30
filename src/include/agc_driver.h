#ifndef AGC_DRIVER_H
#define AGC_DRIVER_H

#define AGC_READ_EVENT     (EPOLLIN|EPOLLRDHUP)
#define AGC_WRITE_EVENT     EPOLLOUT

struct agc_routine_s {
    char ready;
    char active;
    void *data;
    agc_event_callback_func read_handle;
    agc_event_callback_func write_handle;
    agc_event_callback_func err_handle;
};

typedef struct {
    agc_status_t  (*add)(agc_connection_t *c, uint32_t event);
    agc_status_t  (*del)(agc_connection_t *c, uint32_t event);
    
    agc_status_t  (*add_conn)(agc_connection_t *c);
    agc_status_t  (*del_conn)(agc_connection_t *c);
    
} agc_routine_actions_t;

AGC_DECLARE(agc_status_t) agc_diver_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_diver_shutdown(void);

AGC_DECLARE(void) agc_diver_register_routine(agc_routine_actions_t *routines);

AGC_DECLARE(agc_status_t) agc_diver_add_connection(agc_connection_t *c);

AGC_DECLARE(agc_status_t) agc_diver_del_connection(agc_connection_t *c);

AGC_DECLARE(agc_status_t)  agc_diver_add_event(agc_connection_t *c, uint32_t event);

AGC_DECLARE(agc_status_t)  agc_diver_del_event(agc_connection_t *c, uint32_t event);

#endif
