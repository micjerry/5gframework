#ifndef AGC_CORE_H
#define AGC_CORE_H

// global directories
struct agc_directories {
	char *base_dir;
	char *mod_dir;
	char *conf_dir;
	char *log_dir;
	char *run_dir;
	char *certs_dir;
};

typedef struct agc_directories agc_directories_t;

extern agc_directories_t AGC_GLOBAL_dirs;

#define AGC_PATH_SEPARATOR "/"

#define AGC_MAX_PATHLEN 1024

#ifndef AGC_PREFIX_DIR
#define AGC_PREFIX_DIR "."
#endif

AGC_DECLARE(void) agc_cond_next(void);

AGC_DECLARE(agc_status_t) agc_core_init(agc_bool_t console, const char **err);

AGC_DECLARE(void) agc_core_set_globals(void);

AGC_DECLARE(agc_status_t) agc_core_init_and_modload(agc_bool_t console, const char **err);

AGC_DECLARE(char *) agc_core_strdup(agc_memory_pool_t *pool, const char *todup);
                                                  
#endif
