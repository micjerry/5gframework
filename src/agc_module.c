#include <agc.h>
#include <yaml.h>

struct agc_loadable_module {
	char *key;
	char *filename;
	int perm;
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
	switch_mutex_t *mutex;
	switch_memory_pool_t *pool;
};

typedef struct agc_loadable_module agc_loadable_module_t;

static struct agc_loadable_module_container loadable_modules;

static agc_status_t agc_loadable_module_loadfile(char *dir, char *fname, agc_bool_t global, const char **err);
static agc_status_t agc_loadable_module_loadmodule(char *path, char *filename, agc_loadable_module_t **new_module, agc_bool_t global);

AGC_DECLARE(agc_status_t) agc_loadable_module_init(agc_bool_t autoload)
{
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
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
                    agc_loadable_module_loadfile(module_path, module_name, global, &err);
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
    
    assert(path != NULL);
    agc_memory_create_pool(&module_pool);
    
    *new_module = NULL;
    module_func_table_name = agc_core_sprintf(module_pool, "%s_module_interface", filename);
    
    dso = agc_dso_open(NULL, load_global, &derr);
    
    if (!derr && dso) {
        module_func_table =  agc_dso_data_sym(dso, module_func_table_name, &derr);
    }
    
    agc_safe_free(derr);
    
    if (!module_func_table) {
		if (dso) agc_dso_destroy(&dso);
		dso = agc_dso_open(path, load_global, &derr);
	}
    
    while (loading) {
        if (derr) {
			err = derr;
			break;
		}
        
        if (!module_func_table) {
			module_func_table = agc_dso_data_sym(dso, module_func_table_name, &derr);
		}
        
        if (derr) {
			err = derr;
			break;
		}
        
        if (module_func_table && module_func_table->agc_api_version != AGC_API_VERSION) {
			err = "Trying to load an old module.";
			break;
		}
        
        if (module_func_table) {
			module_functions = module_func_table;
			load_func_ptr = module_functions->load;
		}
        
        if (load_func_ptr == NULL) {
			err = "Not a valid module.";
			break;
		}
        
        status = load_func_ptr(&module_interface, pool);

	    if (status != AGC_STATUS_SUCCESS && status != AGC_STATUS_NOUNLOAD) {
			err = "Module load returned an error";
			module_interface = NULL;
			break;
		}

		if (!module_interface) {
			err = "Module failed to initialize. Is this a valid module?";
			break;
		}

		if ((module = agc_core_alloc(pool, sizeof(agc_loadable_module_t))) == 0) {
			err = "Could not allocate memory\n";
			abort();
		}

		if (status == AGC_STATUS_NOUNLOAD) {
			module->perm++;
		}

		loading = 0;
    }
    
    //TODO
}