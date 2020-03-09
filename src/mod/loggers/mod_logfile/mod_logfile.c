#include <agc.h>
#include <yaml.h>

AGC_MODULE_LOAD_FUNCTION(mod_logfile_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_logfile_shutdown);
AGC_MODULE_DEFINITION(mod_logfile, mod_logfile_load, mod_logfile_shutdown, NULL);

#define DEFAULT_LIMIT    0xA00000   /* About 10 MB */
#define WARM_FUZZY_OFFSET 32
#define LOGFILE_MAX_FILES 0x10

#define LOGCFG_FILE "logconf.yml"

static agc_memory_pool_t *module_pool = NULL;

static struct {
    int rotate;
    agc_mutex_t *mutex;
} globals;

struct logfile_profile {
    char *cfgfile;
    agc_size_t log_size;    /* keep the log size in check for rotation */
    agc_size_t roll_size;   /* the size that we want to rotate the file at */
    agc_size_t max_files;       /* number of log files to keep within the rotation */
    agc_file_t *log_afd;
    agc_log_level_t level;
    int suffix;
    char *logfile;
    agc_bool_t log_session;
};

typedef struct logfile_profile logfile_profile_t;

static logfile_profile_t *profile = NULL;

static agc_status_t load_config();

static agc_status_t process_node(const agc_log_node_t *node, agc_log_level_t level);

static agc_status_t mod_logfile_logger(const agc_log_node_t *node, agc_log_level_t level);

static agc_status_t mod_logfile_openlogfile(agc_bool_t check);

static agc_status_t mod_logfile_rotate();

static agc_status_t mod_logfile_raw_write(char *log_data);

static void cleanup_profile();

AGC_MODULE_LOAD_FUNCTION(mod_logfile_load)
{
    module_pool = pool;
    memset(&globals, 0, sizeof(globals));
    
    agc_mutex_init(&globals.mutex, AGC_MUTEX_NESTED, module_pool);
    *module_interface = agc_loadable_module_create_interface(module_pool, modname);
    if (load_config() != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Loading logger config failed\n");
        return AGC_STATUS_GENERR;
    }
    
    agc_log_bind_logger(mod_logfile_logger, AGC_LOG_DEBUG, AGC_FALSE);
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_logfile_shutdown)
{
    agc_log_unbind_logger(mod_logfile_logger);
    cleanup_profile();
    return AGC_STATUS_SUCCESS;
}

static agc_status_t load_config()
{
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
    int done = 0;
    int iskey = 0;
    enum {
        LOGFILE_KEY_ROLLSIZE,
        LOGFILE_KEY_MAXFILES,
        LOGFILE_KEY_LEVEL,
        LOGFILE_KEY_FILE,
        LOGFILE_KEY_UNKOWN
    } keytype = LOGFILE_KEY_UNKOWN;
    
    profile = agc_memory_alloc(module_pool, sizeof(logfile_profile_t));
    
    memset(profile, 0, sizeof(*profile));
    
    profile->cfgfile = agc_core_sprintf(module_pool, "%s%s%s", AGC_GLOBAL_dirs.conf_dir, AGC_PATH_SEPARATOR, LOGCFG_FILE);
    profile->log_session = AGC_TRUE;
    profile->level = AGC_LOG_UNINIT;
          
    file = fopen(profile->cfgfile, "rb");
    assert(file);
    assert(yaml_parser_initialize(&parser));
    
    yaml_parser_set_input_file(&parser, file);
    
    while (!done)
    {
        if (!yaml_parser_scan(&parser, &token)) {
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
                        if (strcmp(token.data.scalar.value, "rollsize") == 0)
                        {
                            keytype = LOGFILE_KEY_ROLLSIZE;
                        } else if (strcmp(token.data.scalar.value, "maxfiles") == 0)
                        {
                            keytype = LOGFILE_KEY_MAXFILES;
                        } else if (strcmp(token.data.scalar.value, "level") == 0)
                        {
                            keytype = LOGFILE_KEY_LEVEL;
                        } else if (strcmp(token.data.scalar.value, "file") == 0)
                        {
                            keytype = LOGFILE_KEY_FILE;
                        } else {
                            keytype = LOGFILE_KEY_UNKOWN;
                        }
                    } else {
                        if (keytype == LOGFILE_KEY_ROLLSIZE) {
                            profile->roll_size = agc_atoui(token.data.scalar.value);
                        } else if (keytype == LOGFILE_KEY_MAXFILES) {
                            profile->max_files = agc_atoui(token.data.scalar.value);
                        } else if (keytype == LOGFILE_KEY_LEVEL) {
                            profile->level = agc_log_str2level(token.data.scalar.value);
                        } else if (keytype == LOGFILE_KEY_FILE) {
                            profile->logfile = agc_core_strdup(module_pool, token.data.scalar.value);
                        }
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
    assert(!fclose(file));
    
    //check value
    
    if (profile->roll_size == 0) {
        profile->roll_size = DEFAULT_LIMIT;
    }
    
    if (profile->max_files == 0) {
        profile->max_files = LOGFILE_MAX_FILES;
    }
    
    if (profile->level == AGC_LOG_UNINIT) {
        profile->level = AGC_LOG_WARNING;
    }
    
    if (zstr(profile->logfile)) {
        char logfile[512];
        agc_snprintf(logfile, sizeof(logfile), "%s%s%s", AGC_GLOBAL_dirs.log_dir, AGC_PATH_SEPARATOR, "agc.log");
        profile->logfile = agc_core_strdup(module_pool, logfile);
    }
    
    if (mod_logfile_openlogfile(AGC_TRUE) != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Create logger file failed\n");
        return AGC_STATUS_GENERR;
    }
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t mod_logfile_logger(const agc_log_node_t *node, agc_log_level_t level)
{
    return process_node(node, level);
}

static agc_status_t process_node(const agc_log_node_t *node, agc_log_level_t level)
{
    if (level <= profile->level) {
        if (profile->log_session && !zstr(node->userdata)) {
            char buf[2048];
            char *dup = strdup(node->data);
            char *lines[100];
            int argc, i;
            
            argc = agc_split(dup, '\n', lines);
            for (i = 0; i < argc; i++) {
                agc_snprintf(buf, sizeof(buf), "%s %s\n", node->userdata, lines[i]);
                mod_logfile_raw_write(buf);    
            }
                
            free(dup);
        } else {
            mod_logfile_raw_write(node->data);
        }
    }
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t mod_logfile_raw_write(char *log_data)
{
    agc_size_t len;
    agc_status_t status = AGC_STATUS_SUCCESS;
    len = strlen(log_data);

    if (len <= 0 || !profile->log_afd) {
        return AGC_STATUS_FALSE;
    }
    
    agc_mutex_lock(globals.mutex);
    
    if (agc_file_write(profile->log_afd, log_data, &len) != AGC_STATUS_SUCCESS) {
        agc_file_close(profile->log_afd);
        if ((status = mod_logfile_openlogfile(AGC_TRUE)) == AGC_STATUS_SUCCESS) {
            len = strlen(log_data);
            agc_file_write(profile->log_afd, log_data, &len);
        }
    }
    
    agc_mutex_unlock(globals.mutex);
    
    if (status == AGC_STATUS_SUCCESS) {
        profile->log_size += len;

        if (profile->roll_size && profile->log_size >= profile->roll_size) {
            mod_logfile_rotate();
        }
    }

    return status;
}

static agc_status_t mod_logfile_openlogfile(agc_bool_t check)
{
    unsigned int flags = 0;
    agc_file_t *afd;
    agc_status_t stat;
    
    flags |= AGC_FOPEN_CREATE;
    flags |= AGC_FOPEN_READ;
    flags |= AGC_FOPEN_WRITE;
    flags |= AGC_FOPEN_APPEND;
    
    stat = agc_file_open(&afd, profile->logfile, flags, AGC_FPROT_OS_DEFAULT, module_pool);
    if (stat != AGC_STATUS_SUCCESS) {
        agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Open logger file failed\n");
        return AGC_STATUS_FALSE;
    }
    
    profile->log_afd = afd;

    profile->log_size = agc_file_get_size(profile->log_afd);
    
    if (check && profile->roll_size && profile->log_size >= profile->roll_size) {
        mod_logfile_rotate();
    }
    
    return AGC_STATUS_SUCCESS;
}

static agc_status_t mod_logfile_rotate()
{
    unsigned int i = 0;
    agc_memory_pool_t *tmp_pool = NULL;
    agc_status_t status = AGC_STATUS_SUCCESS;
    char from_filename[256] = {0};
    char to_filename[256] = {0};
    int creating = 1;
    
    agc_mutex_lock(globals.mutex);
    
    agc_memory_create_pool(&tmp_pool);
    assert(tmp_pool);
    
    //rotate
    for (i = profile->max_files; i > 1; i--) {
        sprintf((char *) to_filename, "%s.%i", profile->logfile, i);
        sprintf((char *) from_filename, "%s.%i", profile->logfile, i - 1);
        if (agc_file_exists(to_filename, tmp_pool) == AGC_STATUS_SUCCESS) {
            if ((status = agc_file_remove(to_filename, tmp_pool)) != AGC_STATUS_SUCCESS)
                break;
        }
        
        if (agc_file_exists(from_filename, tmp_pool) == AGC_STATUS_SUCCESS) {
            if ((status = agc_file_rename(from_filename, to_filename, tmp_pool)) != AGC_STATUS_SUCCESS)
                break;
        }
    }
    
    //creating new file
    while (creating) {
        if (status != AGC_STATUS_SUCCESS)
            break;
        
        sprintf((char *) to_filename, "%s.%i", profile->logfile, i);
        if (agc_file_exists(to_filename, tmp_pool) == AGC_STATUS_SUCCESS) {
            if ((status = agc_file_remove(to_filename, tmp_pool)) != AGC_STATUS_SUCCESS)
                break;
        }
        
        agc_file_close(profile->log_afd);
        
        if ((status = agc_file_rename(profile->logfile, to_filename, tmp_pool)) != AGC_STATUS_SUCCESS) {
            break;
        }
        
        if ((status = mod_logfile_openlogfile(AGC_FALSE)) != AGC_STATUS_SUCCESS) {
            break;
        }
        
        agc_log_printf(AGC_LOG, AGC_LOG_NOTICE, "New log started.\n");
        creating = 0;
    }
    
    if (tmp_pool) {
        agc_memory_destroy_pool(&tmp_pool);
    }
    
    agc_mutex_unlock(globals.mutex);
    
    return status;
}

static void cleanup_profile()
{
    agc_file_close(profile->log_afd); 
}

