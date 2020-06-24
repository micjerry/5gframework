#include <agc.h>

static agc_memory_pool_t *RUNTIME_POOL = NULL;
static agc_api_interface_t *APIS = NULL;
static agc_mutex_t *APIS_MUTEX = NULL;

static agc_status_t parse_and_exec(char *xcmd);

AGC_DECLARE(agc_status_t) agc_api_init(agc_memory_pool_t *pool)
{
	assert(pool);
	RUNTIME_POOL = pool;

	agc_mutex_init(&APIS_MUTEX, AGC_MUTEX_NESTED, RUNTIME_POOL);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Api init success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_api_shutdown(void)
{
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Api shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_api_register(const char *name, const char *desc, const char *syntax, agc_api_func func)
{
	if (!name || !func) 
		return AGC_STATUS_FALSE;
	
	agc_api_interface_t *api = (agc_api_interface_t *)agc_memory_alloc(RUNTIME_POOL, sizeof(agc_api_interface_t));
	assert(api);

	api->name = agc_core_strdup(RUNTIME_POOL, name);

	if (desc)
		api->desc = agc_core_strdup(RUNTIME_POOL, desc);

	if (syntax)
		api->syntax = agc_core_strdup(RUNTIME_POOL, syntax);

	api->function = func;
	
	agc_mutex_lock(APIS_MUTEX);
	api->next = NULL;
    
	if (APIS != NULL) {
		api->next = APIS;
	}
    
	APIS = api;
	agc_mutex_unlock(APIS_MUTEX);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_api_interface_t *) agc_api_find(const char *cmd)
{
	agc_api_interface_t *api = NULL;
	agc_api_interface_t *result = NULL;

	agc_mutex_lock(APIS_MUTEX);

	for (api = APIS; api != NULL; api = api->next) {
		if (!strcasecmp(api->name, cmd)) {
			result = api;
			break;
		}
	}

	agc_mutex_unlock(APIS_MUTEX);

	return result;
}

AGC_DECLARE(agc_status_t) agc_api_execute(const char *cmd, const char *arg, agc_stream_handle_t *stream)
{
	char *command  = (char *)cmd;
	char *argument = (char *)arg;
	agc_api_interface_t *api = NULL;
	agc_status_t status;
    
	assert(stream != NULL);
	assert(stream->data != NULL);
	assert(stream->write_function != NULL);

	if (command && (api = agc_api_find(command)) != NULL) {
		if ((status = api->function(argument, stream)) != AGC_STATUS_SUCCESS) {
			stream->write_function(stream, "COMMAND RETURN ERROR!\n");
		}
	} else {
		status = AGC_STATUS_FALSE;
		stream->write_function(stream, "INVALID COMMAND!\n");
	}
	
	return status;
}

static agc_status_t parse_and_exec(char *xcmd)
{
	char *uuid = NULL;
	char *cmd =NULL;
	char *arg = NULL;
	agc_status_t status;
	agc_stream_handle_t stream = {0};
				

	if (!xcmd)
		return AGC_STATUS_FALSE;

	agc_api_stand_stream(&stream);
	uuid = strdup(xcmd);

	if ((arg = strchr(uuid, '\r')) != 0 || (arg = strchr(uuid, '\n')) != 0) {
		*arg = '\0';
		arg = NULL;
	}
	
	if ((cmd = strchr(uuid, ' ')) != 0) {
		*cmd++ = '\0';
	}

	if ((arg = strchr(cmd, ' ')) != 0) {
		*arg++ = '\0';
	}

	if (!cmd) {
		agc_safe_free(uuid);
		agc_safe_free(stream.data);
		return AGC_STATUS_FALSE;
	}

	stream.uuid = strdup(uuid);

	status = agc_api_execute(cmd, arg, &stream);
	agc_safe_free(stream.data);
	agc_safe_free(uuid);
	agc_safe_free(stream.uuid);

	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_parse_execute(char *xcmd)
{
	char *cmd = NULL;
	char  *argv[100];
	int argc, cmd_numbers;

	if (!xcmd)
		return;

	cmd = strdup(xcmd);

	cmd_numbers = agc_separate_string(cmd, ';', argv, (sizeof(argv) / sizeof(argv[0])));
	
	for (argc=0; argc< cmd_numbers; argc++) {
		if (parse_and_exec(argv[argc])  != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Exec command %s failed.\n", argv[argc]);
		} else {
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Exec command %s success.\n", argv[argc]);
		}
	}

	agc_safe_free(cmd);
	return;
}

AGC_DECLARE(void) agc_api_stand_stream(agc_stream_handle_t *stream)
{
	assert(stream);
    
	memset(stream, 0, sizeof(agc_stream_handle_t));
	stream->data = malloc(AGC_CMD_CHUNK_LEN);
	memset(stream->data, 0, AGC_CMD_CHUNK_LEN);
	stream->end = stream->data;
	stream->data_size = AGC_CMD_CHUNK_LEN;
	stream->write_function = agc_api_stream_write;
	stream->raw_write_function = agc_api_stream_raw_write;
	stream->alloc_len = AGC_CMD_CHUNK_LEN;
	stream->alloc_chunk = AGC_CMD_CHUNK_LEN;
	stream->uuid = NULL;
}

AGC_DECLARE(agc_status_t) agc_api_stream_write(agc_stream_handle_t *handle, const char *fmt, ...)
{
	va_list ap;
	char *buf = handle->data;
	char *end = handle->end;
	int ret = 0;
	char *data = NULL;

	if (handle->data_len >= handle->data_size) {
		return AGC_STATUS_FALSE;
	}
    
	va_start(ap, fmt);
	if (!(data = agc_vmprintf(fmt, ap))) {
		ret = -1;
	}
	va_end(ap);


	if (data) {
		agc_size_t remaining = handle->data_size - handle->data_len;
		agc_size_t need = strlen(data) + 1;
        
		if ((remaining < need) && handle->alloc_len) {
			agc_size_t new_len;
			void *new_data;
            
			new_len = handle->data_size + need + handle->alloc_chunk;
			if ((new_data = realloc(handle->data, new_len))) {
				handle->data_size = handle->alloc_len = new_len;
				handle->data = new_data;
				buf = handle->data;
				remaining = handle->data_size - handle->data_len;
				handle->end = (uint8_t *) (handle->data) + handle->data_len;
				end = handle->end;
			} else {
				agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Alloc memory failed.\n");
				free(data);
				return AGC_STATUS_FALSE;
			}
		}
        
		if (remaining < need) {
			ret = -1;
		} else {
			ret = 0;
			agc_snprintf(end, remaining, "%s", data);
			handle->data_len = strlen(buf);
			handle->end = (uint8_t *) (handle->data) + handle->data_len;
		}
        
		free(data);
	}
    
    return ret ? AGC_STATUS_FALSE : AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_api_stream_raw_write(agc_stream_handle_t *handle, uint8_t *data, agc_size_t datalen)
{
	agc_size_t need = handle->data_len + datalen;

	if (need >= handle->data_size) {
		void *new_data;
		need += handle->alloc_chunk;

		if (!(new_data = realloc(handle->data, need))) {
			return AGC_STATUS_MEMERR;
		}

		handle->data = new_data;
		handle->data_size = need;
	}
    
	memcpy((uint8_t *) (handle->data) + handle->data_len, data, datalen);
	handle->data_len += datalen;
	handle->end = (uint8_t *) (handle->data) + handle->data_len;
	*(uint8_t *) handle->end = '\0';

	return AGC_STATUS_SUCCESS;
}