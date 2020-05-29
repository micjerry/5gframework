#ifndef AGC_TIMER_H
#define AGC_TIMER_H

#include <agc.h>

typedef agc_rbtree_key_t  agc_msec_t;
typedef agc_rbtree_key_int_t  agc_msec_int_t;

AGC_DECLARE(agc_status_t) agc_timer_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_timer_shutdown(void);

//deprecated
AGC_DECLARE(void) agc_timer_del_timer(agc_event_t *ev);

AGC_DECLARE(void) agc_timer_add_timer(agc_event_t *ev, agc_msec_t timer);

AGC_DECLARE(agc_time_t) agc_timer_curtime();

#endif