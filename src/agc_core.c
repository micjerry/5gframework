#include <agc.h>
#include <errno.h>
#include <yaml.h>
#include "agc_core.h"
#include "private/agc_core_pvt.h"

agc_directories_t AGC_GLOBAL_dirs = { 0 };
struct agc_runtime runtime = { 0 };

AGC_DECLARE(agc_status_t) agc_core_init(agc_bool_t console, const char **err)
{
    agc_uuid_t uuid;
    
    memset(&runtime, 0, sizeof(runtime));
    gethostname(runtime.hostname, sizeof(runtime.hostname));
    runtime.cpu_count = sysconf (_SC_NPROCESSORS_ONLN);
    runtime.hard_log_level = AGC_LOG_DEBUG;
    
    if (apr_initialize() != AGC_STATUS_SUCCESS) {
		*err = "FATAL ERROR! Could not initialize APR\n";
		return AGC_STATUS_MEMERR;
	}
    
    if (!(runtime.memory_pool = agc_core_memory_init())) {
		*err = "FATAL ERROR! Could not allocate memory pool\n";
		return AGC_STATUS_MEMERR;
	}
    
    agc_assert(runtime.memory_pool != NULL);
    
    agc_dir_make_recursive(AGC_GLOBAL_dirs.base_dir, AGC_DEFAULT_DIR_PERMS, runtime.memory_pool);
	agc_dir_make_recursive(AGC_GLOBAL_dirs.mod_dir, AGC_DEFAULT_DIR_PERMS, runtime.memory_pool);
	agc_dir_make_recursive(AGC_GLOBAL_dirs.conf_dir, AGC_DEFAULT_DIR_PERMS, runtime.memory_pool);
	agc_dir_make_recursive(AGC_GLOBAL_dirs.log_dir, AGC_DEFAULT_DIR_PERMS, runtime.memory_pool);
	agc_dir_make_recursive(AGC_GLOBAL_dirs.run_dir, AGC_DEFAULT_DIR_PERMS, runtime.memory_pool);
    
    agc_mutex_init(&runtime.uuid_mutex, AGC_MUTEX_NESTED, runtime.memory_pool);
    agc_mutex_init(&runtime.global_mutex, AGC_MUTEX_NESTED, runtime.memory_pool);
    
    if (console) {
        runtime.console = stdout;
    }
    
    //parse_yaml_config
    parse_config(err);
    
}

AGC_DECLARE(void) agc_core_set_globals(void)
{
    char base_dir[AGC_MAX_PATHLEN] = AGC_PREFIX_DIR;
    
    if (!AGC_GLOBAL_dirs.mod_dir && (AGC_GLOBAL_dirs.mod_dir = (char *) malloc(AGC_MAX_PATHLEN))) {
        if (AGC_GLOBAL_dirs.base_dir) {
            agc_snprintf(AGC_GLOBAL_dirs.mod_dir, AGC_MAX_PATHLEN, "%s%smod", AGC_GLOBAL_dirs.base_dir, AGC_PATH_SEPARATOR);
        } else {
#ifdef AGC_MOD_DIR
            agc_snprintf(AGC_GLOBAL_dirs.mod_dir, AGC_MAX_PATHLEN, "%s", AGC_MOD_DIR);
#else
            agc_snprintf(AGC_GLOBAL_dirs.mod_dir, AGC_MAX_PATHLEN, "%s%smod", base_dir, AGC_PATH_SEPARATOR);
#endif
        }
    }
    
    if (!AGC_GLOBAL_dirs.conf_dir && (AGC_GLOBAL_dirs.conf_dir = (char *) malloc(AGC_MAX_PATHLEN))) {
        if (AGC_GLOBAL_dirs.base_dir) {
            agc_snprintf(AGC_GLOBAL_dirs.conf_dir, AGC_MAX_PATHLEN, "%s%sconf", AGC_GLOBAL_dirs.base_dir, AGC_PATH_SEPARATOR);
        } else {
#ifdef AGC_CONF_DIR
            agc_snprintf(AGC_GLOBAL_dirs.conf_dir, AGC_MAX_PATHLEN, "%s", AGC_CONF_DIR);
#else
            agc_snprintf(AGC_GLOBAL_dirs.conf_dir, AGC_MAX_PATHLEN, "%s%sconf", base_dir, AGC_PATH_SEPARATOR);
#endif
        }
    }  
    
    if (!AGC_GLOBAL_dirs.log_dir && (AGC_GLOBAL_dirs.log_dir = (char *) malloc(AGC_MAX_PATHLEN))) {
        if (AGC_GLOBAL_dirs.base_dir) {
            agc_snprintf(AGC_GLOBAL_dirs.log_dir, AGC_MAX_PATHLEN, "%s%slog", AGC_GLOBAL_dirs.base_dir, AGC_PATH_SEPARATOR);
        } else {
#ifdef AGC_LOG_DIR
            agc_snprintf(AGC_GLOBAL_dirs.log_dir, AGC_MAX_PATHLEN, "%s", AGC_LOG_DIR);
#else
            agc_snprintf(AGC_GLOBAL_dirs.log_dir, AGC_MAX_PATHLEN, "%s%slog", base_dir, AGC_PATH_SEPARATOR);
#endif            
        }
    }

    if (!AGC_GLOBAL_dirs.run_dir && (AGC_GLOBAL_dirs.run_dir = (char *) malloc(AGC_MAX_PATHLEN))) {
        if (AGC_GLOBAL_dirs.base_dir) {
            agc_snprintf(AGC_GLOBAL_dirs.run_dir, AGC_MAX_PATHLEN, "%s%srun", AGC_GLOBAL_dirs.base_dir, AGC_PATH_SEPARATOR);
        } else {
#ifdef AGC_RUN_DIR
            agc_snprintf(AGC_GLOBAL_dirs.run_dir, AGC_MAX_PATHLEN, "%s", AGC_RUN_DIR);
#else
            agc_snprintf(AGC_GLOBAL_dirs.run_dir, AGC_MAX_PATHLEN, "%s%srun", base_dir, AGC_PATH_SEPARATOR);
#endif 
        }
    } 

    if (!AGC_GLOBAL_dirs.certs_dir && (AGC_GLOBAL_dirs.certs_dir = (char *) malloc(AGC_MAX_PATHLEN))) {
        if (AGC_GLOBAL_dirs.base_dir) {
            agc_snprintf(AGC_GLOBAL_dirs.certs_dir, AGC_MAX_PATHLEN, "%s%scerts", AGC_GLOBAL_dirs.base_dir, AGC_PATH_SEPARATOR);
        } else {
#ifdef AGC_CERTS_DIR
            agc_snprintf(AGC_GLOBAL_dirs.certs_dir, AGC_MAX_PATHLEN, "%s", AGC_CERTS_DIR);
#else
            agc_snprintf(AGC_GLOBAL_dirs.certs_dir, AGC_MAX_PATHLEN, "%s%scerts", base_dir, AGC_PATH_SEPARATOR);
#endif
        }
    }      
    
}

AGC_DECLARE(agc_status_t) agc_core_init_and_modload(agc_bool_t console, const char **err)
{
    
}

AGC_DECLARE(char *) agc_core_strdup(agc_memory_pool_t *pool, const char *todup)
{
    char *duped = NULL;
	agc_size_t len;
	assert(pool != NULL);
    
    if (!todup) {
		return NULL;
	}
    
    if (zstr(todup)) {
		return AGC_BLANK_STRING;
	}
    
    len = strlen(todup) + 1;
    duped = apr_pstrmemdup(pool, todup, len);
    assert(duped != NULL);
    return duped;
}
