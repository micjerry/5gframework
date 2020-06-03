#ifndef AGC_LOG_H
#define AGC_LOG_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

typedef enum {
	AGC_ID_LOG,
	AGC_ID_SESSION
} agc_log_type_t;

typedef struct {
	char *data;
	char file[80];
	uint32_t line;
	char func[80];
	agc_log_level_t level;
	agc_time_t timestamp;
	char *content;
	char *userdata;
    agc_log_type_t type;
} agc_log_node_t;

#define AGC_LOG_QUEUE_LEN 100000

#define AGC_LOG AGC_ID_LOG, __FILE__, __AGC_FUNC__, __LINE__, NULL
#define AGC_SESSION_LOG(x) AGC_ID_SESSION, __FILE__, __AGC_FUNC__, __LINE__, (const char*)(x)

typedef agc_status_t (*agc_log_function_t) (const agc_log_node_t *node, agc_log_level_t level);

AGC_DECLARE(agc_status_t) agc_log_init(agc_memory_pool_t *pool, agc_bool_t colorize);

AGC_DECLARE(agc_status_t) agc_log_shutdown(void);

AGC_DECLARE(void) agc_log_printf(agc_log_type_t type, const char *file,
									   const char *func, int line,
									   const char *userdata, agc_log_level_t level,
									   const char *fmt, ...) PRINTF_FUNCTION(7, 8);
                                       
AGC_DECLARE(void) agc_log_vprintf(agc_log_type_t type, const char *file,
										const char *func, int line,
										const char *userdata, agc_log_level_t level, const char *fmt, va_list ap);
                                        
AGC_DECLARE(agc_status_t) agc_log_bind_logger(agc_log_function_t function, agc_log_level_t level, agc_bool_t is_console);
AGC_DECLARE(agc_status_t) agc_log_unbind_logger(agc_log_function_t function);

AGC_DECLARE(const char *) agc_log_level2str(agc_log_level_t level);
AGC_DECLARE(agc_log_level_t) agc_log_str2level(const char *str);

AGC_DECLARE(agc_log_node_t *) agc_log_node_dup(const agc_log_node_t *node);
AGC_DECLARE(void) agc_log_node_free(agc_log_node_t **pnode);
     
AGC_END_EXTERN_C	 

#endif
