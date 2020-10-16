#include "mod_test.h"

#define TEST_SQL_TABLENAME "agc_test"
#define TEST_SQL_FILEDUSER "username"
#define TEST_SQL_FILEDPASS "password"
#define TEST_SQL_FILEDPERMITS "permits"
#define TEST_SQL_VALUEUSER "admin"

void test_insert(agc_stream_handle_t *stream);
void test_query(agc_stream_handle_t *stream);
void test_modify(agc_stream_handle_t *stream);
void test_delete(agc_stream_handle_t *stream);

int sql_callback(void *pArg, int argc, char **argv, char **columnNames);
int sql_errcallback(void *pArg, const char *errmsg);

void test_sql_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	test_insert(stream);
	test_query(stream);
	test_modify(stream);
	test_delete(stream);
}

void test_insert(agc_stream_handle_t *stream)
{
	agc_sql_handle_t *dbh;
	char *err = NULL;
	char *sql = NULL;

	if (agc_sql_get_handle(&dbh) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_insert create handle [fail].\n");
		return;
	}

	sql = agc_mprintf("INSERT INTO %s (%s, %s) VALUES ('%q', '%q');", TEST_SQL_TABLENAME, 
		TEST_SQL_FILEDUSER, 
		TEST_SQL_FILEDPASS,
		TEST_SQL_VALUEUSER,
		"acoLOS902");
	
	if (agc_sql_execute_sql(dbh, sql, &err) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_insert exec sql[fail].\n");
	} else {
		stream->write_function(stream, "test test_insert [success].\n");
	}

	agc_safe_free(err);
	agc_safe_free(sql);
	agc_sql_release_handle(&dbh);
}

void test_query(agc_stream_handle_t *stream)
{
	agc_sql_handle_t *dbh;
	char *err = NULL;
	char *sql = NULL;

	if (agc_sql_get_handle(&dbh) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_query create handle [fail].\n");
		return;
	}

	sql = agc_mprintf("SELECT * FROM %s;", TEST_SQL_TABLENAME);
	if (agc_sql_execute_sql_callback(dbh, sql, sql_callback, NULL, &err) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_query exec sql[fail].\n");
	} else {
		stream->write_function(stream, "test test_query [success].\n");
	}

	agc_safe_free(err);
	agc_safe_free(sql);
	agc_sql_release_handle(&dbh);
}

void test_modify(agc_stream_handle_t *stream)
{
	agc_sql_handle_t *dbh;
	char *err = NULL;
	char *sql = NULL;
	char *query_sql = NULL;

	if (agc_sql_get_handle(&dbh) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_modify create handle [fail].\n");
		return;
	}

	sql = agc_mprintf("UPDATE %s SET %s= '%q' WHERE %s='%q' ;", TEST_SQL_TABLENAME, 
		TEST_SQL_FILEDPASS,
		"lkcIUS182",
		TEST_SQL_FILEDUSER,
		TEST_SQL_VALUEUSER);
	
	if (agc_sql_execute_sql(dbh, sql, &err) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_modify exec sql[fail].\n");
		agc_safe_free(err);
	} 

	stream->write_function(stream, "test test_modify affect %d.\n", agc_sql_affected_rows(dbh));
		
	query_sql = agc_mprintf("SELECT * FROM %s;", TEST_SQL_TABLENAME);
	if (agc_sql_execute_sql_callback_err(dbh, query_sql, sql_callback, sql_errcallback, NULL, &err) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_modify query[fail].\n");
	} else {
		stream->write_function(stream, "test test_modify [success].\n");
	}

	agc_safe_free(err);
	agc_safe_free(sql);
	agc_safe_free(query_sql);
	agc_sql_release_handle(&dbh);	
}

void test_delete(agc_stream_handle_t *stream)
{
	agc_sql_handle_t *dbh;
	char *err = NULL;
	char *sql = NULL;

	if (agc_sql_get_handle(&dbh) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_delete create handle [fail].\n");
		return;
	}

	sql = agc_mprintf("DELETE FROM %s WHERE %s='%q';", TEST_SQL_TABLENAME, TEST_SQL_FILEDUSER, TEST_SQL_VALUEUSER);

	if (agc_sql_persistant_execute(dbh, sql, 2) != AGC_STATUS_SUCCESS) {
		stream->write_function(stream, "test test_delete sql[fail].\n");
	} else {
		stream->write_function(stream, "test test_delete sql[success].\n");
	}

	stream->write_function(stream, "test test_delete affect %d.\n", agc_sql_affected_rows(dbh));
	
	agc_safe_free(err);
	agc_safe_free(sql);
	agc_sql_release_handle(&dbh);
}

int sql_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	int i;

	for (i = 0; i < argc; i++) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "column %s value %s .\n", columnNames[i], argv[i]);
	}

	return AGC_DB_OK;
}

int sql_errcallback(void *pArg, const char *errmsg)
{
	agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL ERR %s .\n", errmsg);
}

