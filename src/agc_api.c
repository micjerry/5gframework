#include <agc.h>

static agc_memory_pool_t *RUNTIME_POOL = NULL;

AGC_DECLARE(agc_status_t) agc_api_init(agc_memory_pool_t *pool)
{
    assert(pool);
    RUNTIME_POOL = pool;
    
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Api init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_api_shutdown(void)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Api shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_api_stand_stream(agc_stream_handle_t *stream)
{
    assert(stream);
    
    memset(stream, 0, sizeof(agc_stream_handle_t))
    stream->data = malloc(AGC_CMD_CHUNK_LEN);
    memset(stream->data, 0, AGC_CMD_CHUNK_LEN);
    stream->end = stream->data;
    stream->data_size = AGC_CMD_CHUNK_LEN;
    stream->write_function = agc_api_stream_write;
    stream->raw_write_function = agc_api_stream_raw_write;
    stream->alloc_len = AGC_CMD_CHUNK_LEN;
    stream->alloc_chunk = AGC_CMD_CHUNK_LEN;
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