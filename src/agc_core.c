#include <agc.h>
#include <errno.h>
#include <yaml.h>
#include "agc_core.h"
#include "private/agc_core_pvt.h"

agc_directories_t AGC_GLOBAL_dirs = { 0 };
struct agc_runtime runtime = { 0 };

void parse_config(const char **err) 
{
    apr_dir_t *dir;
    apr_finfo_t finfo;
    char *headers[16];
    
    if (apr_dir_open(&dir, AGC_GLOBAL_dirs.conf_dir, runtime.memory_pool) != APR_SUCCESS) {
        *err = "FATAL ERROR! Could not open conf dir\n";
        return;
    }
    
    while ((apr_dir_read(&finfo, APR_FINFO_NAME, dir)) == APR_SUCCESS) {
        FILE *file;
        yaml_parser_t parser;
        yaml_token_t token;
        int done = 0;
        int pos = 0;
        int iskey = 0;
        int i = 0;
        char last_key[128] = {0};
        char full_key[512] = {0};
    
        file = fopen(finfo.fname, "rb");
        agc_assert(file);
        agc_assert(yaml_parser_initialize(&parser));
        yaml_parser_set_input_file(&parser, file);
        while (!done)
        {
            if (!yaml_parser_scan(&parser, &token)) {
                *err = "FATAL ERROR! Could not parser config file\n";
                break;
            }
			
			switch(token.type)
			{
				case YAML_STREAM_START_TOKEN:
				case YAML_STREAM_END_TOKEN:
				case YAML_VERSION_DIRECTIVE_TOKEN:
				case YAML_TAG_DIRECTIVE_TOKEN:
				case YAML_DOCUMENT_START_TOKEN:
				case YAML_DOCUMENT_END_TOKEN:
				case YAML_BLOCK_SEQUENCE_START_TOKEN:
				    break;
				case YAML_BLOCK_MAPPING_START_TOKEN:
                    if (strlen(last_key) > 0) {
                        headers[pos] = malloc(strlen(last_key) + 1);
                        strcpy(headers[pos], last_key);
                        pos++;
                    }
				    break;
				case YAML_BLOCK_END_TOKEN:
                    if (pos > 0) {
                        pos--;
                        free(headers[pos]);
                        headers[pos] = NULL;
                    }
				    break;
				case YAML_FLOW_SEQUENCE_START_TOKEN:
				case YAML_FLOW_SEQUENCE_END_TOKEN:
				case YAML_FLOW_MAPPING_START_TOKEN:
				case YAML_FLOW_MAPPING_END_TOKEN:
				case YAML_BLOCK_ENTRY_TOKEN:
				case YAML_FLOW_ENTRY_TOKEN:
				    break;
				case YAML_KEY_TOKEN:
                    iskey = 1;
				    break;
				case YAML_VALUE_TOKEN:
                    iskey = 0;
				    break;
				case YAML_ALIAS_TOKEN: 
				case YAML_ANCHOR_TOKEN:
				case YAML_TAG_TOKEN:
				    break;
				case YAML_SCALAR_TOKEN:
				    {
                        if (iskey) {
                            strcpy(last_key, token.data.scalar.value);
                        }
                        else {
                            memset(full_key, 0, sizeof(full_key));
                            for (i = 0; i < pos; i++) {
                                strcat(full_key, headers[i]);
                                strcat(full_key, ".");
                            }
                        
                            strcat(full_key, last_key);
                            printf("YAML_SCALAR_TOKEN: key=%s, value=%s, style=%d\n", full_key, token.data.scalar.value);
                        }
				    }
				    break;
                default:
                    break;
			}
            done = (token.type == YAML_STREAM_END_TOKEN);
 
            yaml_token_delete(&token);
 
        }
        yaml_parser_delete(&parser);
        fclose(file);
    }
    
    apr_dir_close(dir);
}

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
