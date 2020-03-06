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
    
    return AGC_STATUS_SUCCESS;
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

AGC_DECLARE(char *) agc_core_vsprintf(agc_memory_pool_t *pool, const char *fmt, va_list ap)
{
    char *result = NULL;
    assert(pool != NULL);
    
    result = apr_pvsprintf(pool, fmt, ap);
	assert(result != NULL);
    return result;
}

AGC_DECLARE(char *) agc_core_sprintf(agc_memory_pool_t *pool, const char *fmt, ...)
{
    va_list ap;
	char *result;
	va_start(ap, fmt);
	result = agc_core_vsprintf(pool, fmt, ap);
	va_end(ap);

	return result;
}

AGC_DECLARE(agc_status_t) agc_core_modload(const char **err)
{
    if (runtime.runlevel > 1) {
		return AGC_STATUS_SUCCESS;
	}
    
    runtime.runlevel++;
    
    if (agc_loadable_module_init() != AGC_STATUS_SUCCESS) {
		*err = "Cannot load modules";
		agc_log_printf(AGC_LOG, AGC_LOG_CONSOLE, "Error: %s\n", *err);
		return AGC_STATUS_GENERR;
	}
    
    return AGC_STATUS_SUCCESS;
    
}

AGC_DECLARE(agc_thread_t *) agc_core_launch_thread(agc_thread_start_t func, void *obj, agc_memory_pool_t *pool)
{
    agc_thread_t *thread = NULL;
    agc_threadattr_t *thd_attr = NULL;
    agc_core_thread_obj_t *thd_obj = NULL;
    
    if (!pool) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "no pool\n");
        return NULL;
    }
    
    agc_threadattr_create(&thd_attr, pool);
    if ((thd_obj = agc_memory_alloc(pool, sizeof(*thd_obj))) == 0) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Could not allocate memory\n");
        return NULL;
    }
    
    thd_obj->pool = pool;
    thd_obj->objs[0] = obj;
    thd_obj->objs[1] = thread;
    
    agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
    agc_threadattr_priority_set(thd_attr, AGC_PRI_REALTIME);
    
    agc_thread_create(&thread, thd_attr, func, thd_obj, pool);
    
    return thread;
}
