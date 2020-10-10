#ifndef AGC_ODBC_H
#define AGC_ODBC_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

#define DEFAULT_ODBC_RETRIES 120

struct agc_odbc_handle;
typedef void *agc_odbc_statement_handle_t;

typedef enum {
	AGC_ODBC_STATE_INIT,
	AGC_ODBC_STATE_DOWN,
	AGC_ODBC_STATE_CONNECTED,
	AGC_ODBC_STATE_ERROR
} agc_odbc_state_t;

typedef enum {
	AGC_ODBC_SUCCESS = 0,
	AGC_ODBC_FAIL = -1
} agc_odbc_status_t;

AGC_DECLARE(agc_odbc_handle_t *) agc_odbc_handle_new(const char *dsn, const char *username, const char *password);
AGC_DECLARE(void) agc_odbc_set_num_retries(agc_odbc_handle_t *handle, int num_retries);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_connect(agc_odbc_handle_t *handle);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_disconnect(agc_odbc_handle_t *handle);
AGC_DECLARE(void) agc_odbc_handle_destroy(agc_odbc_handle_t **handlep);
AGC_DECLARE(agc_odbc_state_t) agc_odbc_handle_get_state(agc_odbc_handle_t *handle);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_exec(agc_odbc_handle_t *handle, const char *sql, agc_odbc_statement_handle_t *rstmt,
															 char **err);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_exec_string(agc_odbc_handle_t *handle, const char *sql, char *resbuf, size_t len, char **err);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_SQLSetAutoCommitAttr(agc_odbc_handle_t *handle, agc_bool_t on);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_SQLEndTran(agc_odbc_handle_t *handle, agc_bool_t commit);
AGC_DECLARE(agc_odbc_status_t) agc_odbc_statement_handle_free(agc_odbc_statement_handle_t *stmt);


AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_callback_exec(const char *file, const char *func, int line, agc_odbc_handle_t *handle,
																			   const char *sql, agc_db_callback_func_t callback, void *pdata,
																			   char **err);
AGC_DECLARE(char *) agc_odbc_handle_get_error(agc_odbc_handle_t *handle, agc_odbc_statement_handle_t stmt);

AGC_DECLARE(int) agc_odbc_handle_affected_rows(agc_odbc_handle_t *handle);


AGC_END_EXTERN_C

#endif
