#ifndef AGC_SQL_H
#define AGC_SQL_H

#include <agc.h>

AGC_BEGIN_EXTERN_C

agc_status_t agc_sql_start(agc_memory_pool_t *pool);
void agc_sql_stop(void);

AGC_DECLARE(agc_status_t) _agc_sql_get_handle(agc_sql_handle_t ** dbh, const char *file, const char *func, int line);
#define agc_sql_get_handle(_a) _agc_sql_get_handle(_a, __FILE__, __AGC_FUNC__, __LINE__)

AGC_DECLARE(void) agc_sql_release_handle(agc_sql_handle_t ** dbh);

AGC_DECLARE(char *) agc_sql_execute_sql2str(agc_sql_handle_t *dbh, char *sql, char *str, size_t len, char **err);
AGC_DECLARE(agc_status_t) agc_sql_execute_sql(agc_sql_handle_t *dbh, char *sql, char **err);
AGC_DECLARE(agc_status_t) agc_sql_execute_sql_callback(agc_sql_handle_t *dbh, const char *sql,
																	 agc_db_callback_func_t callback, void *pdata, char **err);
AGC_DECLARE(agc_status_t) agc_sql_execute_sql_callback_err(agc_sql_handle_t *dbh, const char *sql,
																	 agc_db_callback_func_t callback,
																	 agc_db_err_callback_func_t err_callback,
																	 void *pdata, char **err);
AGC_DECLARE(int) agc_sql_affected_rows(agc_sql_handle_t *dbh);

AGC_DECLARE(agc_status_t) agc_sql_persistant_execute(agc_sql_handle_t *dbh, const char *sql, uint32_t retries);


AGC_END_EXTERN_C

#endif

