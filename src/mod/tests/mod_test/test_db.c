#include "mod_test.h"

#define TEST_DB_FILENAME "agc_test.db"

void test_db_exec(agc_stream_handle_t *stream);
void test_db_prepare(agc_stream_handle_t *stream);

int db_callback(void *pArg, int argc, char **argv, char **columnNames);

void test_db_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	agc_db_t *db = NULL;

	if (agc_db_open(TEST_DB_FILENAME, &db) != AGC_DB_OK) {
		stream->write_function(stream, "test open db [fail].\n");
		return;
	}

	
	
}

void test_db_exec(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (agc_db_open(TEST_DB_FILENAME, &db) != AGC_DB_OK) {
		stream->write_function(stream, "test open db [fail].\n");
		return;
	}

	if (agc_db_exec(db, "SELECT * FROM agc_table;", db_callback, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "test agc_db_exec [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		return;
	}

	stream->write_function(stream, "test agc_db_exec [ok].\n");
	agc_db_close(db);
}

void test_db_prepare(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;

	if (agc_db_open(TEST_DB_FILENAME, &db) != AGC_DB_OK) {
		stream->write_function(stream, "test open db [fail].\n");
		return;
	}

	if (agc_db_prepare(db, "insert into agc_table values(?, ?, ?)", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_prepare failed .\n");
		stream->write_function(stream, "test agc_db_prepare [fail].\n");
		agc_db_close(db);
		return;
	}

	
}

int db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	int i;

	for (i = 0; i < argc; i++) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "column %s value %s .\n", columnNames[i], argv[i]);
	}

	return AGC_DB_OK;
}


