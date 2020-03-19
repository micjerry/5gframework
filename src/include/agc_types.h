#ifndef AGC_TYPES_H
#define AGC_TYPES_H

#include <agc.h>

#define __AGC_FUNC__ (const char *)__func__

#define AGC_SEQ_ESC "\033["
#define AGC_SEQ_AND_COLOR ";"
#define AGC_SEQ_END_COLOR "m"

#define AGC_SEQ_DEFAULT_COLOR AGC_SEQ_ESC AGC_SEQ_END_COLOR

/* Foreground colors values */
#define AGC_SEQ_F_BLACK "30"
#define AGC_SEQ_F_RED "31"
#define AGC_SEQ_F_GREEN "32"
#define AGC_SEQ_F_YELLOW "33"
#define AGC_SEQ_F_BLUE "34"
#define AGC_SEQ_F_MAGEN "35"
#define AGC_SEQ_F_CYAN "36"
#define AGC_SEQ_F_WHITE "37"
/* Background colors values */
#define AGC_SEQ_B_BLACK "40"
#define AGC_SEQ_B_RED "41"
#define AGC_SEQ_B_GREEN "42"
#define AGC_SEQ_B_YELLOW "43"
#define AGC_SEQ_B_BLUE "44"
#define AGC_SEQ_B_MAGEN "45"
#define AGC_SEQ_B_CYAN "46"
#define AGC_SEQ_B_WHITE "47"	

#define AGC_SEQ_FBLACK AGC_SEQ_ESC AGC_SEQ_F_BLACK AGC_SEQ_END_COLOR
#define AGC_SEQ_FRED AGC_SEQ_ESC AGC_SEQ_F_RED AGC_SEQ_END_COLOR
#define AGC_SEQ_FGREEN AGC_SEQ_ESC AGC_SEQ_F_GREEN AGC_SEQ_END_COLOR
#define AGC_SEQ_FYELLOW AGC_SEQ_ESC AGC_SEQ_F_YELLOW AGC_SEQ_END_COLOR
#define AGC_SEQ_FBLUE AGC_SEQ_ESC AGC_SEQ_F_BLUE AGC_SEQ_END_COLOR
#define AGC_SEQ_FMAGEN AGC_SEQ_ESC AGC_SEQ_F_MAGEN AGC_SEQ_END_COLOR
#define AGC_SEQ_FCYAN AGC_SEQ_ESC AGC_SEQ_F_CYAN AGC_SEQ_END_COLOR
#define AGC_SEQ_FWHITE AGC_SEQ_ESC AGC_SEQ_F_WHITE AGC_SEQ_END_COLOR
#define AGC_SEQ_BBLACK AGC_SEQ_ESC AGC_SEQ_B_BLACK AGC_SEQ_END_COLOR
#define AGC_SEQ_BRED AGC_SEQ_ESC AGC_SEQ_B_RED AGC_SEQ_END_COLOR
#define AGC_SEQ_BGREEN AGC_SEQ_ESC AGC_SEQ_B_GREEN AGC_SEQ_END_COLOR
#define AGC_SEQ_BYELLOW AGC_SEQ_ESC AGC_SEQ_B_YELLOW AGC_SEQ_END_COLOR
#define AGC_SEQ_BBLUE AGC_SEQ_ESC AGC_SEQ_B_BLUE AGC_SEQ_END_COLOR
#define AGC_SEQ_BMAGEN AGC_SEQ_ESC AGC_SEQ_B_MAGEN AGC_SEQ_END_COLOR
#define AGC_SEQ_BCYAN AGC_SEQ_ESC AGC_SEQ_B_CYAN AGC_SEQ_END_COLOR
#define AGC_SEQ_BWHITE AGC_SEQ_ESC AGC_SEQ_B_WHITE AGC_SEQ_END_COLOR

typedef struct apr_pool_t agc_memory_pool_t;
typedef struct apr_thread_mutex_t agc_mutex_t;

typedef int64_t agc_time_t;
typedef int64_t agc_interval_time_t;

typedef intptr_t agc_int_t;

/** Opaque type used for the atomic operations */
#ifdef apr_atomic_t
    typedef apr_atomic_t agc_atomic_t;
#else
    typedef uint32_t agc_atomic_t;
#endif

typedef struct apr_thread_rwlock_t agc_thread_rwlock_t;
typedef struct apr_thread_cond_t agc_thread_cond_t;

typedef struct agc_time_exp_t {
	/** microseconds past tm_sec */
    int32_t tm_usec;
	/** (0-61) seconds past tm_min */
    int32_t tm_sec;
	/** (0-59) minutes past tm_hour */
    int32_t tm_min;
	/** (0-23) hours past midnight */
    int32_t tm_hour;
	/** (1-31) day of the month */
    int32_t tm_mday;
	/** (0-11) month of the year */
    int32_t tm_mon;
	/** year since 1900 */
    int32_t tm_year;
	/** (0-6) days since sunday */
    int32_t tm_wday;
	/** (0-365) days since jan 1 */
    int32_t tm_yday;
	/** daylight saving time */
    int32_t tm_isdst;
	/** seconds east of UTC */
    int32_t tm_gmtoff;
} agc_time_exp_t;

typedef struct {
    unsigned char data[16];
} agc_uuid_t;

typedef enum {
	AGC_FALSE = 0,
	AGC_TRUE = 1
} agc_bool_t;

typedef struct apr_queue_t agc_queue_t;

typedef struct apr_file_t agc_file_t;
typedef int32_t agc_fileperms_t;
typedef int agc_seek_where_t;
typedef uint16_t agc_port_t;

typedef int agc_os_socket_t;
#define AGC_SOCK_INVALID -1

typedef enum {
	AGC_STATUS_SUCCESS,
	AGC_STATUS_FALSE,
	AGC_STATUS_TIMEOUT,
	AGC_STATUS_RESTART,
	AGC_STATUS_INTR,
	AGC_STATUS_NOTIMPL,
	AGC_STATUS_MEMERR,
	AGC_STATUS_NOOP,
	AGC_STATUS_RESAMPLE,
	AGC_STATUS_GENERR,
	AGC_STATUS_INUSE,
	AGC_STATUS_BREAK,
	AGC_STATUS_SOCKERR,
	AGC_STATUS_MORE_DATA,
	AGC_STATUS_NOTFOUND,
	AGC_STATUS_UNLOAD,
	AGC_STATUS_NOUNLOAD,
	AGC_STATUS_IGNORE,
	AGC_STATUS_TOO_SMALL,
	AGC_STATUS_FOUND,
	AGC_STATUS_CONTINUE,
	AGC_STATUS_TERM,
	AGC_STATUS_NOT_INITALIZED,
	AGC_STATUS_TOO_LATE,
	AGC_STATUS_XBREAK = 35,
	AGC_STATUS_WINBREAK = 730035
} agc_status_t;

typedef enum {
	AGC_EVENT_CUSTOM,
	AGC_EVENT_CLONE,
	AGC_EVENT_XXX
} agc_event_types_t;

typedef enum {
    AGC_PRI_LOW = 1,
    AGC_PRI_NORMAL = 10,
    AGC_PRI_IMPORTANT = 50,
    AGC_PRI_REALTIME = 99,
} agc_thread_priority_t;

typedef enum {
    AGC_LOG_DEBUG = 7,
    AGC_LOG_INFO = 6,
    AGC_LOG_NOTICE = 5,
    AGC_LOG_WARNING = 4,
    AGC_LOG_ERROR = 3,
    AGC_LOG_CRIT = 2,
    AGC_LOG_ALERT = 1,
    AGC_LOG_CONSOLE = 0,
    AGC_LOG_INVALID = 64,
    AGC_LOG_UNINIT = 1000,
} agc_log_level_t;

#define AGC_BLANK_STRING ""
#define AGC_THREAD_STACKSIZE 240 * 1024
#define AGC_DEFAULT_DIR_PERMS AGC_FPROT_UREAD | AGC_FPROT_UWRITE | AGC_FPROT_UEXECUTE | AGC_FPROT_GREAD | AGC_FPROT_GEXECUTE

#define AGC_PATH_SEPARATOR "/"
#define AGC_BLANK_STRING ""

typedef int agc_std_socket_t;
typedef struct sockaddr agc_std_sockaddr_t;
typedef struct agc_connection_s agc_connection_t;
typedef struct agc_routine_s agc_routine_t;
typedef struct agc_listening_s agc_listening_t;
typedef void (*agc_routine_handler_func)(agc_routine_t *routine);
typedef void (*agc_connection_handler_func)(agc_connection_t *c);

#endif
