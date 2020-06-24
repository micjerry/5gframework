#ifndef AGC_API_H
#define AGC_API_H

AGC_BEGIN_EXTERN_C

#define AGC_CMD_CHUNK_LEN 1024

#define AGC_STANDARD_API(name) static agc_status_t name (const char *cmd, agc_stream_handle_t *stream)

typedef struct agc_stream_handle_s agc_stream_handle_t;
typedef struct agc_api_interface_s agc_api_interface_t;

typedef agc_status_t (*agc_api_func) (const char *cmd, agc_stream_handle_t *stream);
typedef uint8_t * (*agc_stream_handle_read_func) (agc_stream_handle_t *handle, int *len);
typedef agc_status_t (*agc_stream_handle_write_func) (agc_stream_handle_t *handle, const char *fmt, ...);
typedef agc_status_t (*agc_stream_handle_raw_write_func) (agc_stream_handle_t *handle, uint8_t *data, agc_size_t datalen);

struct agc_stream_handle_s {
	agc_stream_handle_read_func read_function;
	agc_stream_handle_write_func write_function;
	agc_stream_handle_raw_write_func raw_write_function;
	void *data;
	void *end;
	agc_size_t data_size;
	agc_size_t data_len;
	agc_size_t alloc_len;
	agc_size_t alloc_chunk;
	char *uuid;
};

struct agc_api_interface_s {
    const char *name;
    const char *desc;
    agc_api_func function;
    const char *syntax;
    agc_api_interface_t *next;
};

AGC_DECLARE(agc_status_t) agc_api_init(agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_api_shutdown(void);

AGC_DECLARE(agc_status_t) agc_api_register(const char *name, const char *desc, const char *syntax, agc_api_func func);

AGC_DECLARE(agc_api_interface_t *) agc_api_find(const char *cmd);

AGC_DECLARE(agc_status_t) agc_api_execute(const char *cmd, const char *arg, agc_stream_handle_t *stream);

AGC_DECLARE(void) agc_parse_execute(char *xcmd);

AGC_DECLARE(void) agc_api_stand_stream(agc_stream_handle_t *stream);

AGC_DECLARE(agc_status_t) agc_api_stream_write(agc_stream_handle_t *handle, const char *fmt, ...);

AGC_DECLARE(agc_status_t) agc_api_stream_raw_write(agc_stream_handle_t *handle, uint8_t *data, agc_size_t datalen);

AGC_END_EXTERN_C

#endif