#ifndef AGC_MODULE_H
#define AGC_MODULE_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

#define CFG_MODULES_FILE "modules.yml"

struct agc_loadable_module_interface {
    const char *module_name;
    agc_thread_rwlock_t *rwlock;
	int refs;
    agc_memory_pool_t *pool;
};

typedef struct agc_loadable_module_interface agc_loadable_module_interface_t;

#define AGC_API_VERSION 1
#define AGC_MODULE_LOAD_ARGS (agc_loadable_module_interface_t **module_interface, agc_memory_pool_t *pool)
#define AGC_MODULE_RUNTIME_ARGS (void)
#define AGC_MODULE_SHUTDOWN_ARGS (void)

typedef agc_status_t (*agc_module_load_func) AGC_MODULE_LOAD_ARGS;
typedef agc_status_t (*agc_module_runtime_func) AGC_MODULE_RUNTIME_ARGS;
typedef agc_status_t (*agc_module_shutdown_func) AGC_MODULE_SHUTDOWN_ARGS;

#define AGC_MODULE_LOAD_FUNCTION(name) agc_status_t name AGC_MODULE_LOAD_ARGS
#define AGC_MODULE_RUNTIME_FUNCTION(name) agc_status_t name AGC_MODULE_RUNTIME_ARGS
#define AGC_MODULE_SHUTDOWN_FUNCTION(name) agc_status_t name AGC_MODULE_SHUTDOWN_ARGS

typedef uint32_t agc_module_flag_t;

typedef struct agc_loadable_module_function_table {
	int agc_api_version;
	agc_module_load_func load;
	agc_module_shutdown_func shutdown;
	agc_module_runtime_func runtime;
	agc_module_flag_t flags;
} agc_loadable_module_function_table_t;

#define AGC_MODULE_DEFINITION_EX(name, load, shutdown, runtime, flags)					\
static const char modname[] =  #name ;														\
agc_loadable_module_function_table_t name##_module_interface = {	\
	AGC_API_VERSION,																		\
	load,																					\
	shutdown,																				\
	runtime,																				\
	flags																					\
}

#define AGC_MODULE_DEFINITION(name, load, shutdown, runtime)								\
		AGC_MODULE_DEFINITION_EX(name, load, shutdown, runtime, 0)

AGC_DECLARE(agc_status_t) agc_loadable_module_init();

AGC_DECLARE(void) agc_loadable_module_shutdown(void);

AGC_DECLARE(agc_loadable_module_interface_t *) agc_loadable_module_create_interface(agc_memory_pool_t *pool, const char *name);

AGC_END_EXTERN_C

#endif