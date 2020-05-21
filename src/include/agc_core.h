#ifndef AGC_CORE_H
#define AGC_CORE_H

#define AGC_MAX_CORE_THREAD_OBJS 128

// global directories
struct agc_directories {
	char *base_dir;
	char *mod_dir;
	char *conf_dir;
	char *log_dir;
	char *run_dir;
	char *certs_dir;
	char *db_dir;
};

typedef struct agc_directories agc_directories_t;

struct agc_core_thread_obj {
	/*! status of the thread */
	int running;
	/*! mutex */
	agc_mutex_t *mutex;
	/*! array of void pointers to pass mutiple data objects */
	void *objs[AGC_MAX_CORE_THREAD_OBJS];
	/*! a pointer to a memory pool if the thread has it's own pool */
	agc_memory_pool_t *pool;
};

typedef struct agc_core_thread_obj agc_core_thread_obj_t;

extern agc_directories_t AGC_GLOBAL_dirs;

#define AGC_PATH_SEPARATOR "/"

#define AGC_MAX_PATHLEN 1024

#ifndef AGC_PREFIX_DIR
#define AGC_PREFIX_DIR "."
#endif

AGC_DECLARE(void) agc_cond_next(void);

AGC_DECLARE(agc_status_t) agc_core_init(agc_bool_t console, const char **err);

AGC_DECLARE(agc_status_t) agc_core_destroy();

AGC_DECLARE(void) agc_core_runtime_loop(void);

AGC_DECLARE(agc_bool_t) agc_core_is_running(void);

AGC_DECLARE(void) agc_core_set_globals(void);

AGC_DECLARE(char *) agc_core_strdup(agc_memory_pool_t *pool, const char *todup);

AGC_DECLARE(char *) agc_core_vsprintf(agc_memory_pool_t *pool, const char *fmt, va_list ap);

AGC_DECLARE(char *) agc_core_sprintf(agc_memory_pool_t *pool, const char *fmt, ...);

AGC_DECLARE(uint32_t) agc_core_cpu_count(void);

AGC_DECLARE(void) agc_os_yield(void);

AGC_DECLARE(agc_thread_t *) agc_core_launch_thread(agc_thread_start_t func, void *obj, agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_core_modload(const char **err);

AGC_DECLARE(const char *) agc_core_get_hostname(void);

AGC_DECLARE(void) agc_core_set_signal_handlers(void);
                                             
#endif
