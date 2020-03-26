#ifndef AGC_API_H
#define AGC_API_H

#define AGC_CMD_CHUNK_LEN 1024

#define AGC_STANDARD_API(name) static agc_status_t name (const char *cmd, agc_stream_handle_t *stream)

typedef struct agc_stream_handle agc_stream_handle_t;
typedef agc_status_t (*agc_api_func) (const char *cmd, agc_stream_handle_t *stream);

struct agc_stream_handle {
	agc_stream_handle_read_func read_function;
	agc_stream_handle_write_func write_function;
	agc_stream_handle_raw_write_func raw_write_function;
	void *data;
	void *end;
	agc_size_t data_size;
	agc_size_t data_len;
	agc_size_t alloc_len;
	agc_size_t alloc_chunk;
};

struct agc_api_interface {
    const char *name;
    const char *desc;
    agc_api_func function;
    const char *syntax;
    struct agc_api_interface *next;
};

AGC_DECLARE(agc_status_t) agc_api_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_api_shutdown(void);

AGC_DECLARE(void) agc_api_stand_stream(agc_stream_handle_t *stream);

AGC_DECLARE(agc_status_t) agc_api_stream_write(agc_stream_handle_t *handle, const char *fmt, ...);

AGC_DECLARE(agc_status_t) agc_api_stream_raw_write(agc_stream_handle_t *handle, uint8_t *data, agc_size_t datalen);

#endif