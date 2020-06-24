#include "mod_test.h"

#define TEST_DB_FILENAME "agc_test.db"

typedef agc_status_t (*test_db_func) (agc_stream_handle_t *stream);

typedef struct test_db_command {
	char *pname;
	test_db_func func;
} test_db_command_t;

static agc_status_t test_db_exec(agc_stream_handle_t *stream);
static agc_status_t test_db_insert(agc_stream_handle_t *stream);
static agc_status_t test_db_query(agc_stream_handle_t *stream);
static agc_status_t test_db_update(agc_stream_handle_t *stream);
static agc_status_t test_db_delete(agc_stream_handle_t *stream);

int db_callback(void *pArg, int argc, char **argv, char **columnNames);

test_db_command_t db_commands[] = {
	{"test_db_exec", test_db_exec},
	{"test_db_insert", test_db_insert},
	{"test_db_query", test_db_query},
	{"test_db_update", test_db_update},
	{"test_db_delete", test_db_delete}
};

#define TEST_DBCMD_SIZE (sizeof(db_commands)/sizeof(db_commands[0]))

void test_db_api(agc_stream_handle_t *stream, int argc, char **argv)
{
	int i = 0;

	for (i = 0; i < TEST_DBCMD_SIZE; i++) {
		test_db_func pfunc = db_commands[i].func;
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "begin execute %s.\n", db_commands[i].pname);
		if (pfunc(stream) != AGC_STATUS_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_INFO, "execute %s failed.\n", db_commands[i].pname);
			break;
		}
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "end execute %s.\n", db_commands[i].pname);
	}	
}

static agc_status_t test_db_exec(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	char *err_msg = NULL;

	if (!(db = agc_db_open_file(TEST_DB_FILENAME))) {
		stream->write_function(stream, "test open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_exec(db, "SELECT * FROM agc_table;", db_callback, NULL, &err_msg) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "agc_db_exec failed %s .\n", err_msg);
		stream->write_function(stream, "test agc_db_exec [fail].\n");
		agc_db_free(err_msg);
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

	stream->write_function(stream, "test agc_db_exec [ok].\n");
	agc_db_close(db);
	return AGC_STATUS_SUCCESS;
}

static agc_status_t test_db_insert(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	char *err_msg = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!(db = agc_db_open_file(TEST_DB_FILENAME)))  {
		stream->write_function(stream, "test open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "insert into agc_table values(?, ?, ?)", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_insert prepare failed .\n");
		stream->write_function(stream, "test_db_insert prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 1, "micjerry", strlen("micjerry"), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_insert bind text failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_insert bind text [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_int(stmt, 2, 7) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_insert bind int failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_insert bind int [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 3, "shihe", strlen("shihe"), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_insert bind text failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_insert bind text [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "insert data failed .\n");
		stream->write_function(stream, "test_db_insert [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test_db_insert [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}

static agc_status_t test_db_query(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	int rc;
	int columns = 0;
	int cindex;
	const char *cname;
	const char *cteacher;
	int error = 0;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!(db = agc_db_open_file(TEST_DB_FILENAME)))  {
		stream->write_function(stream, "test open db [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_prepare(db, "SELECT * FROM agc_class;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_query failed .\n");
		stream->write_function(stream, "test_db_query prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}	

	do {
		rc = agc_db_step(stmt);
		switch(rc) {
			case AGC_DB_DONE:
				break;

			case AGC_DB_ROW:
				columns = agc_db_column_count(stmt);
				cindex = agc_db_column_int(stmt, 0);
				cname = agc_db_column_text(stmt, 1);
				cteacher = agc_db_column_text(stmt, 2);
				agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "new row id %d, name %s, teacher %s .\n", cindex, cname, cteacher);
				break;
			default:
				error = 1;
				break;
		}
	} while (rc == AGC_DB_ROW);

	if (error) {
		stream->write_function(stream, "test_db_query [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test_db_query [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}

static agc_status_t test_db_update(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!(db = agc_db_open_file(TEST_DB_FILENAME))) {
		stream->write_function(stream, "test open db [fail].\n");
		return AGC_STATUS_GENERR;
	}
	
	if (agc_db_prepare(db, "UPDATE agc_class SET teacher = ?  WHERE name = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_update prepare failed .\n");
		stream->write_function(stream, "test_db_update prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}	

	if (agc_db_bind_text(stmt, 1, "Lisuper", strlen("Lisuper"), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_update bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_update bind text [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_bind_text(stmt, 2, "Chinese", strlen("Chinese"), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_update bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_update bind text [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_update failed .\n");
		stream->write_function(stream, "test_db_update [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test_db_update [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);

	return status;
}

static agc_status_t test_db_delete(agc_stream_handle_t *stream)
{
	agc_db_t *db = NULL;
	agc_db_stmt_t *stmt = NULL;
	agc_status_t status = AGC_STATUS_GENERR;

	if (!(db = agc_db_open_file(TEST_DB_FILENAME)))  {
		stream->write_function(stream, "test open db [fail].\n");
		return AGC_STATUS_GENERR;
	}
	
	if (agc_db_prepare(db, "DELETE FROM agc_class WHERE name = ?;", -1, &stmt, NULL) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_delete prepare failed .\n");
		stream->write_function(stream, "test_db_delete prepare [fail].\n");
		agc_db_close(db);
		return AGC_STATUS_GENERR;
	}	

	if (agc_db_bind_text(stmt, 1, "English", strlen("English"), AGC_CORE_DB_TRANSIENT) != AGC_DB_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_delete bind failed .\n");
		agc_db_finalize(stmt);
		agc_db_close(db);
		stream->write_function(stream, "test_db_delete bind text [fail].\n");
		return AGC_STATUS_GENERR;
	}

	if (agc_db_step(stmt) != AGC_DB_DONE) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "test_db_update failed .\n");
		stream->write_function(stream, "test_db_delete [fail].\n");
	} else {
		status = AGC_STATUS_SUCCESS;
		stream->write_function(stream, "test_db_delete [ok].\n");
	}

	agc_db_finalize(stmt);
	agc_db_close(db);	

	return status;
}

int db_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	int i;

	for (i = 0; i < argc; i++) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "column %s value %s .\n", columnNames[i], argv[i]);
	}

	return AGC_DB_OK;
}


