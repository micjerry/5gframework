#include <agc.h>
#include <yaml.h>

struct agc_loadable_module {
	char *key;
	char *filename;
	int perm;
    agc_loadable_module_interface *module_interface;
    agc_dso_lib_t lib;
	agc_module_load_func load_func;
	agc_module_runtime_func runtime_func;
	agc_module_shutdown_func shutdown_func;
	agc_memory_pool_t *pool;
	agc_status_t status;
	agc_thread_t *thread;
	agc_bool_t shutting_down;
};

struct agc_loadable_module_container {
    char *cfgfile_path;
	agc_hash_t *module_hash;
	agc_mutex_t *mutex;
	agc_memory_pool_t *pool;
};

typedef struct agc_loadable_module agc_loadable_module_t;

static struct agc_loadable_module_container loadable_modules;

static agc_status_t agc_loadable_module_loadfile(char *dir, char *fname, agc_bool_t global, const char **err);
static agc_status_t agc_loadable_module_loadmodule(char *path, char *filename, agc_loadable_module_t **new_module, agc_bool_t global);
static agc_status_t agc_loadable_module_processmodule(char *key, agc_loadable_module_t *new_module);

static void agc_loadable_module_runtime(void);
static void * agc_loadable_module_exec(agc_thread_t *thread, void *obj);

AGC_DECLARE(agc_status_t) agc_loadable_module_init()
{
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
    int module_count = 0;
    int done = 0;
    int error = 0;
    int iskey = 0;
    int block = 0;
    int BLOCK_MODULES = 1;
    int new_module = 0;
    char module_name[64] = {0};
    char module_path[256] = {0};
    char module_global[16] = {0};
    char *datap = NULL;
    const char *err;

    memset(&loadable_modules, 0, sizeof(loadable_modules));
    agc_memory_create_pool(&loadable_modules.pool);
    
    loadable_modules.module_hash = agc_hash_make(loadable_modules.pool);
    
    loadable_modules.cfgfile_path = agc_core_sprintf(loadable_modules.pool, "%s%s%s", AGC_GLOBAL_dirs.conf_dir, AGC_PATH_SEPARATOR, CFG_MODULES_FILE);
    
    file = fopen(loadable_modules.cfgfile_path, "rb");
    assert(file);
    assert(yaml_parser_initialize(&parser));
    
    yaml_parser_set_input_file(&parser, file);
    
    while (!done)
    {
        if (!yaml_parser_scan(&parser, &token)) {
            error = 1;
            break;
        }
        
        switch(token.type)
        {
            case YAML_KEY_TOKEN:
                iskey = 1;
                break;
            case YAML_VALUE_TOKEN:
                iskey = 0;
                break;
            case YAML_SCALAR_TOKEN:
                {
                    if (iskey)
                    {
                        if (strcmp(token.data.scalar.value, "modules") == 0)
                        {
                            block = BLOCK_MODULES;
                            datap = NULL;
                        } else if (strcmp(token.data.scalar.value, "name") == 0)
                        {
                            datap = module_name;
                        } else if (strcmp(token.data.scalar.value, "path") == 0)
                        {
                            datap = module_path;
                        } else if (strcmp(token.data.scalar.value, "global") == 0)
                        {
                            datap = module_global;
                        } else {
                            block = 0;
                            datap = NULL;
                        }
                    } else {
                        if (datap)
                            strcpy(datap, token.data.scalar.value);
                    }
                }
                break;
            case YAML_BLOCK_ENTRY_TOKEN:
                if (block == BLOCK_MODULES) {
                    new_module = 1;
                    memset(module_path, 0, sizeof(module_path));
                    memset(module_name, 0, sizeof(module_name));
                    memset(module_global, 0, sizeof(module_global));
                }
                break;
            case YAML_BLOCK_END_TOKEN:
                if ((block == BLOCK_MODULES) && new_module) {
                    agc_bool_t global = AGC_FALSE;
                    global = agc_true(module_global);
                    if (agc_loadable_module_loadfile(module_path, module_name, global, &err) != AGC_STATUS_SUCCESS) {
                        agc_log_printf(AGC_ID_LOG, AGC_LOG_CRIT, "Failed to load module %s, abort()\n", module_name);
                        abort();
                    }
                    module_count++;
                    new_module = 0;
                }
                break;
            default:
                break;
        }
        
        done = (token.type == YAML_STREAM_END_TOKEN);
        yaml_token_delete(&token);
    }
    
    yaml_parser_delete(&parser);
    assert(!fclose(file));
    
    if (!count) {
		agc_log_printf(AGC_ID_LOG, AGC_LOG_CONSOLE, "No modules loaded\n");
        return AGC_STATUS_GENERR;
	}
    
    agc_loadable_module_runtime();
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_loadable_module_loadfile(char *dir, char *fname, agc_bool_t global, const char **err)
{
    agc_size_t len = 0;
    agc_loadable_module_t *new_module = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    const char *ext = ".so";
    char *locate;
    char *path;
    char *file;
    
    *err = "";
    
    file = agc_core_strdup(loadable_modules.pool, fname);
    
    if (zstr(dir)) {
        locate = AGC_GLOBAL_dirs.mod_dir;
    } else {
        locate = dir;
    }
    
    len = strlen(locate);
    len += strlen(fname);
    len += 8;
    
    path = (char *)agc_memory_alloc(loadable_modules.pool, len);
    agc_snprintf(path, len, "%s%s%s%s", locate, AGC_PATH_SEPARATOR, file, ext);
    
    if (agc_hash_get(loadable_modules.module_hash, file, AGC_HASH_KEY_STRING)) {
        agc_log_printf(AGC_ID_LOG, AGC_LOG_WARNING, "Module %s Already Loaded!\n", file);
        *err = "Module already loaded";
        return AGC_STATUS_SUCCESS;
    }
    
    
    if ((status = agc_loadable_module_loadmodule(path, file, &new_module, global)) == AGC_STATUS_SUCCESS) {
        if ((status = agc_loadable_module_processmodule(file, new_module)) == AGC_STATUS_SUCCESS) {
            agc_log_printf(AGC_ID_LOG, AGC_LOG_INFO, "Module %s load sucess\n", file);
        } else {
            agc_log_printf(AGC_ID_LOG, AGC_LOG_CRIT, "Module %s load failed\n", file);
        }
    } else {
        *err = "module load failed";
    }
    
    return status;
}

static agc_status_t agc_loadable_module_loadmodule(char *path, 
                                                   char *filename, 
                                                   agc_loadable_module_t **new_module, 
                                                   agc_bool_t global)
{
    agc_loadable_module_t *module = NULL;
	agc_dso_lib_t dso = NULL;
    apr_status_t status = AGC_STATUS_SUCCESS;
    agc_loadable_module_function_table_t *module_func_table;
    agc_loadable_module_function_table_t *module_functions;
    agc_loadable_module_interface_t *module_interface = NULL;
    agc_module_load_func load_func_ptr = NULL;
    agc_memory_pool_t *module_pool = NULL;
    char *module_func_table_name = NULL;
    agc_bool_t load_global = global;
    char *err = NULL;
    char *derr = NULL;
    int loading = 1;
    int loading_times = 0;
    
    assert(path != NULL);
    agc_memory_create_pool(&module_pool);
    
    *new_module = NULL;
    module_func_table_name = agc_core_sprintf(module_pool, "%s_module_interface", filename);
    
    while (loading_times < 2) {
        dso = agc_dso_open(path, load_global, &derr);
        
        if (derr || !dso) {
            if (derr)
                agc_log_printf(AGC_ID_LOG, AGC_LOG_CRIT, "Loading module %s\n failed: %s\n", path, derr);
            
            if (dso)
                agc_dso_destroy(&dso);
            
            agc_safe_free(derr);
            continue;
        }
         
        module_func_table =  agc_dso_data_sym(dso, module_func_table_name, &derr);
    
        if (!module_func_table) {
            if (derr)
                agc_log_printf(AGC_ID_LOG, AGC_LOG_CRIT, "Loading module %s no functable found.\n reason: %s\n", path, derr);
            
		    if (dso) 
                agc_dso_destroy(&dso);
            
            agc_safe_free(derr);
            continue;
	    }
        
        break;
    }
    
    while (loading) {
        if (!module_func_table) {
            err = "no functable found";
            break;
        }
    
        if (module_func_table && module_func_table->agc_api_version != AGC_API_VERSION) {
            err = "load an old module";
	        break;
	    }
    
        if (module_func_table) {
		    module_functions = module_func_table;
		    load_func_ptr = module_functions->load;
        }
    
        status = load_func_ptr(&module_interface, module_pool);

	    if (status != AGC_STATUS_SUCCESS && status != AGC_STATUS_NOUNLOAD) {
            err = "load function returned an error";
            break;
	    }

	    if (!module_interface) {
            err = "failed to initialize";
            break;
	    }

	    if ((module = agc_core_alloc(module_pool, sizeof(agc_loadable_module_t))) == 0) {
		    abort();
	    }

	    if (status == AGC_STATUS_NOUNLOAD) {
		    module->perm++;
	    }
        
        loading = 0;
    }
    
    if (err) {
        if (dso)
            agc_dso_destroy(&dso);
        
        if (module_pool)
            agc_memory_destroy_pool(module_pool);
        
        agc_log_printf(AGC_ID_LOG, AGC_LOG_CRIT, "Loading module %s\n failed: %s\n", path, err);
        return AGC_STATUS_GENERR;
    }
    
    module->pool = module_pool;
    module->filename =  agc_core_strdup(module->pool, path);
    module->module_interface = module_interface;
    module->load_func = load_func_ptr;
    
    if (module_functions) {
        module->runtime_func = module_functions->runtime;
        module->shutdown_func = module_functions->shutdown;
    }
    
    module->lib = dso;
    *new_module = module;
    agc_log_printf(AGC_ID_LOG, AGC_LOG_CONSOLE, "Successfully Loaded [%s]\n", module_interface->module_name);
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t agc_loadable_module_processmodule(char *key, agc_loadable_module_t *new_module)
{
    new_module->key = agc_core_strdup(new_module->pool, key);
    agc_mutex_lock(loadable_modules.mutex);
    
    agc_hash_set(loadable_modules.module_hash, key, AGC_HASH_KEY_STRING, new_module);
    
    agc_mutex_unlock(loadable_modules.mutex);
	return AGC_STATUS_SUCCESS;
}

static void * agc_loadable_module_exec(agc_thread_t *thread, void *obj)
{
    agc_status_t status = AGC_STATUS_SUCCESS;
    
    agc_core_thread_obj_t *thd_obj = obj;
    
    assert(thd_obj != NULL);
    
    agc_loadable_module_t *module = thd_obj->objs[0];
    int restarts;
    
    assert(thread != NULL);
	assert(module != NULL);
    
    for (restarts = 0; status != AGC_STATUS_TERM && !module->shutting_down; restarts++) {
		status = module->runtime_func();
	}
    
    agc_log_printf(AGC_ID_LOG, AGC_LOG_NOTICE, "Thread ended for %s\n", module->module_interface->module_name);
    
    if (thd_obj->pool) {
		switch_memory_pool_t *pool = thd_obj->pool;
		agc_log_printf(AGC_ID_LOG, AGC_LOG_DEBUG, "Destroying Pool for %s\n", module->module_interface->module_name);
		agc_memory_destroy_pool(&pool);
	}
	agc_thread_exit(thread, 0);
	return NULL;
}

static void agc_loadable_module_runtime(void)
{
    agc_hash_index_t *hi;
    void *val;
	agc_loadable_module_t *module;
    
    agc_mutex_lock(loadable_modules.mutex);
    for (hi = agc_hash_first(loadable_modules.pool, loadable_modules.module_hash); hi; hi = agc_hash_next(hi)) {
        val = agc_hash_this_val(hi);
        module = (agc_loadable_module_t *) val;
        
        if (module->runtime_func) {
			agc_log_printf(AGC_ID_LOG, AGC_LOG_CONSOLE, "Starting runtime thread for %s\n", module->module_interface->module_name);
			module->thread = agc_core_launch_thread(agc_loadable_module_exec, module, module->pool);
		}
    }
    agc_mutex_unlock(loadable_modules.mutex);
}

