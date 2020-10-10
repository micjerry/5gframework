#include <agc.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#if (ODBCVER < 0x0300)
#define SQL_NO_DATA SQL_SUCCESS
#endif


struct agc_odbc_handle {
	char *dsn;
	char *username;
	char *password;
	SQLHENV env;
	SQLHDBC con;
	agc_odbc_state_t state;
	char odbc_driver[256];
	BOOL is_firebird;
	BOOL is_oracle;
	int affected_rows;
	int num_retries;
};

static agc_odbc_status_t init_odbc_handles(agc_odbc_handle_t *handle, agc_bool_t do_reinit);
static int db_is_up(agc_odbc_handle_t *handle);

AGC_DECLARE(agc_odbc_handle_t *) agc_odbc_handle_new(const char *dsn, const char *username, const char *password)
{
	agc_odbc_handle_t *new_handle;

	if (!(new_handle = malloc(sizeof(*new_handle)))) {
		return NULL;
	}

	memset(new_handle, 0, sizeof(*new_handle));

	if (!(new_handle->dsn = strdup(dsn))) {
		agc_safe_free(new_handle);
		return NULL;
	}

	if (username) {
		if (!(new_handle->username = strdup(username))) {
			agc_safe_free(new_handle->dsn);
			agc_safe_free(new_handle);
			return NULL;
		}
	}

	if (password) {
		if (!(new_handle->password = strdup(password))) {
			agc_safe_free(new_handle->dsn);
			agc_safe_free(new_handle->username);
			agc_safe_free(new_handle);
			return NULL;
		}
	}

	new_handle->env = SQL_NULL_HANDLE;
	new_handle->state = AGC_ODBC_STATE_INIT;
	new_handle->affected_rows = 0;
	new_handle->num_retries = DEFAULT_ODBC_RETRIES;

	return new_handle;
}

AGC_DECLARE(void) agc_odbc_set_num_retries(agc_odbc_handle_t *handle, int num_retries)
{
	if (handle) {
		handle->num_retries = num_retries;
	}
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_connect(agc_odbc_handle_t *handle)
{
	int result;
	SQLINTEGER err;
	int16_t mlen;
	unsigned char msg[200] = "", stat[10] = "";
	SQLSMALLINT valueLength = 0;
	int i = 0;

	init_odbc_handles(handle, AGC_FALSE); 

	if (handle->state == AGC_ODBC_STATE_CONNECTED) {
		agc_odbc_handle_disconnect(handle);
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Re-connecting %s\n", handle->dsn);
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Connecting %s\n", handle->dsn);
	if (!strstr(handle->dsn, "DRIVER")) {
		result = SQLConnect(handle->con, (SQLCHAR *) handle->dsn, SQL_NTS, (SQLCHAR *) handle->username, SQL_NTS, (SQLCHAR *) handle->password, SQL_NTS);
	} else {
		SQLCHAR outstr[1024] = { 0 };
		SQLSMALLINT outstrlen = 0;
		result =
			SQLDriverConnect(handle->con, NULL, (SQLCHAR *) handle->dsn, (SQLSMALLINT) strlen(handle->dsn), outstr, sizeof(outstr), &outstrlen,
							 SQL_DRIVER_NOPROMPT);
	}

	if ((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		char *err_str;

		if ((err_str = agc_odbc_handle_get_error(handle, NULL))) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Connect failed: %s\n", err_str);
			free(err_str);
		} else {
			SQLGetDiagRec(SQL_HANDLE_DBC, handle->con, 1, stat, &err, msg, sizeof(msg), &mlen);
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error SQLConnect=%d errno=%d [%s]\n", result, (int) err, msg);
		}

		init_odbc_handles(handle, AGC_TRUE); /* Reinit ODBC handles */
		return AGC_ODBC_FAIL;
	}

	result = SQLGetInfo(handle->con, SQL_DRIVER_NAME, (SQLCHAR *) handle->odbc_driver, 255, &valueLength);
	if (result == SQL_SUCCESS || result == SQL_SUCCESS_WITH_INFO) {
		for (i = 0; i < valueLength; ++i)
			handle->odbc_driver[i] = (char) toupper(handle->odbc_driver[i]);
	}	

	if (strstr(handle->odbc_driver, "SQORA32.DLL") != 0 || strstr(handle->odbc_driver, "SQORA64.DLL") != 0) {
		handle->is_firebird = FALSE;
		handle->is_oracle = TRUE;
	} else if (strstr(handle->odbc_driver, "FIREBIRD") != 0 || strstr(handle->odbc_driver, "FB32") != 0 || strstr(handle->odbc_driver, "FB64") != 0) {
		handle->is_firebird = TRUE;
		handle->is_oracle = FALSE;
	} else {
		handle->is_firebird = FALSE;
		handle->is_oracle = FALSE;
	}

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Connected to [%s] \n", handle->dsn);
	handle->state = AGC_ODBC_STATE_CONNECTED;
	return AGC_ODBC_SUCCESS;
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_disconnect(agc_odbc_handle_t *handle)
{
	int result;

	if (!handle) {
		return AGC_ODBC_FAIL;
	}

	if (handle->state == AGC_ODBC_STATE_CONNECTED) {
		result = SQLDisconnect(handle->con);
		if (result == AGC_ODBC_SUCCESS) {
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Disconnected %d from [%s]\n", result, handle->dsn);
		} else {
			agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "Error Disconnecting [%s]\n", handle->dsn);
		}
	}

	handle->state = AGC_ODBC_STATE_DOWN;

	return AGC_ODBC_SUCCESS;
}

AGC_DECLARE(void) agc_odbc_handle_destroy(agc_odbc_handle_t **handlep)
{
	agc_odbc_handle_t *handle = NULL;

	if (!handlep) {
		return;
	}
	handle = *handlep;

	if (handle) {
		agc_odbc_handle_disconnect(handle);

		if (handle->env != SQL_NULL_HANDLE) {
			SQLFreeHandle(SQL_HANDLE_DBC, handle->con);
			SQLFreeHandle(SQL_HANDLE_ENV, handle->env);
		}
		agc_safe_free(handle->dsn);
		agc_safe_free(handle->username);
		agc_safe_free(handle->password);
		agc_safe_free(handle);
	}
	
	*handlep = NULL;
}

AGC_DECLARE(agc_odbc_state_t) agc_odbc_handle_get_state(agc_odbc_handle_t *handle)
{
	return handle ? handle->state : AGC_ODBC_STATE_INIT;
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_exec(agc_odbc_handle_t *handle, const char *sql, agc_odbc_statement_handle_t *rstmt,
															 char **err)
{
	SQLHSTMT stmt = NULL;
	int result;
	char *err_str = NULL, *err2 = NULL;
	SQLLEN m = 0;

	if (!handle) {
		return AGC_ODBC_FAIL;
	}

	if (!db_is_up(handle)) {
		goto error;
	}

	if (SQLAllocHandle(SQL_HANDLE_STMT, handle->con, &stmt) != SQL_SUCCESS) {
		err2 = "SQLAllocHandle failed.";
		goto error;
	}

	if (SQLPrepare(stmt, (unsigned char *) sql, SQL_NTS) != SQL_SUCCESS) {
		err2 = "SQLPrepare failed.";
		goto error;
	}

	result = SQLExecute(stmt);

	switch (result) {
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NO_DATA:
		break;
	case SQL_ERROR:
		err2 = "SQLExecute returned SQL_ERROR.";
		goto error;
		break;
	case SQL_NEED_DATA:
		err2 = "SQLExecute returned SQL_NEED_DATA.";
		goto error;
		break;
	default:
		err2 = "SQLExecute returned unknown result code.";
		goto error;
	}

	SQLRowCount(stmt, &m);
	handle->affected_rows = (int) m;

	if (rstmt) {
		*rstmt = stmt;
	} else {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}

	return AGC_ODBC_SUCCESS;

error:

	if (stmt) {
		err_str = agc_odbc_handle_get_error(handle, stmt);
	}

	if (zstr(err_str)) {
		if (err2) {
			err_str = strdup(err2);
		} else {
			err_str = strdup((char *)"SQL ERROR!");
		}
	}

	
	if (err_str) {
		if (!agc_stristr("already exists", err_str) && !agc_stristr("duplicate key name", err_str)) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, agc_str_nil(err_str));

		}
		if (err) {
			*err = err_str;
		} else {
			free(err_str);
		}
	}

	if (rstmt) {
		*rstmt = stmt;
	} else if (stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}	

	return AGC_ODBC_FAIL;
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_exec_string(agc_odbc_handle_t *handle, const char *sql, char *resbuf, size_t len, char **err)
{
	agc_odbc_status_t sstatus = AGC_ODBC_FAIL;
	agc_odbc_statement_handle_t stmt = NULL;
	SQLCHAR name[1024];
	SQLLEN m = 0;

	if (!handle) {
		return sstatus;
	}
	
	handle->affected_rows = 0;

	if (agc_odbc_handle_exec(handle, sql, &stmt, err) == AGC_ODBC_SUCCESS) {
		SQLSMALLINT NameLength, DataType, DecimalDigits, Nullable;
		SQLULEN ColumnSize;
		int result;

		SQLRowCount(stmt, &m);
		handle->affected_rows = (int) m;

		if (m == 0) {
			goto done;
		}

		result = SQLFetch(stmt);

		if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO && result != SQL_NO_DATA) {
			goto done;
		}

		SQLDescribeCol(stmt, 1, name, sizeof(name), &NameLength, &DataType, &ColumnSize, &DecimalDigits, &Nullable);
		SQLGetData(stmt, 1, SQL_C_CHAR, (SQLCHAR *) resbuf, (SQLLEN) len, NULL);

		sstatus = SWITCH_ODBC_SUCCESS;
	}

done:
	agc_odbc_statement_handle_free(&stmt);

	return sstatus;
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_SQLSetAutoCommitAttr(agc_odbc_handle_t *handle, agc_bool_t on)
{
	if (on) {
		return SQLSetConnectAttr(handle->con, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER *) SQL_AUTOCOMMIT_ON, 0 );
	} else {
		return SQLSetConnectAttr(handle->con, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER *) SQL_AUTOCOMMIT_OFF, 0 );
	}
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_SQLEndTran(agc_odbc_handle_t *handle, agc_bool_t commit)
{
	if (commit) {
		return SQLEndTran(SQL_HANDLE_DBC, handle->con, SQL_COMMIT);
	} else {
		return SQLEndTran(SQL_HANDLE_DBC, handle->con, SQL_ROLLBACK);
	}
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_statement_handle_free(agc_odbc_statement_handle_t *stmt)
{
	if (!stmt || !*stmt) {
		return AGC_ODBC_FAIL;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, *stmt);
	*stmt = NULL;
	return AGC_ODBC_SUCCESS;
}

AGC_DECLARE(agc_odbc_status_t) agc_odbc_handle_callback_exec(const char *file, const char *func, int line, agc_odbc_handle_t *handle,
																			   const char *sql, agc_db_callback_func_t callback, void *pdata,
																			   char **err)
{
	SQLHSTMT stmt = NULL;
	SQLSMALLINT c = 0, x = 0;
	SQLLEN m = 0;
	char *x_err = NULL, *err_str = NULL;
	int result;
	int err_cnt = 0;
	int done = 0;

	if (!handle) {
		return AGC_ODBC_FAIL;
	}

	handle->affected_rows = 0;

	agc_assert(callback != NULL);

	if (!db_is_up(handle)) {
		x_err = "DB is not up!";
		goto error;
	}

	if (SQLAllocHandle(SQL_HANDLE_STMT, handle->con, &stmt) != SQL_SUCCESS) {
		x_err = "Unable to SQL allocate handle!";
		goto error;
	}

	if (SQLPrepare(stmt, (unsigned char *) sql, SQL_NTS) != SQL_SUCCESS) {
		x_err = "Unable to prepare SQL statement!";
		goto error;
	}

	result = SQLExecute(stmt);

	if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO && result != SQL_NO_DATA) {
		x_err = "execute error!";
		goto error;
	}

	SQLNumResultCols(stmt, &c);
	SQLRowCount(stmt, &m);
	handle->affected_rows = (int) m;

	while (!done) {
		int name_len = 256;
		char **names;
		char **vals;
		int y = 0;

		result = SQLFetch(stmt);

		if (result != SQL_SUCCESS) {
			if (result != SQL_NO_DATA) {
				err_cnt++;
			}
			break;
		}

		names = calloc(c, sizeof(*names));
		vals = calloc(c, sizeof(*vals));

		agc_assert(names && vals);

		for (x = 1; x <= c; x++) {
			SQLSMALLINT NameLength = 0, DataType = 0, DecimalDigits = 0, Nullable = 0;
			SQLULEN ColumnSize = 0;
			names[y] = malloc(name_len);
			memset(names[y], 0, name_len);

			SQLDescribeCol(stmt, x, (SQLCHAR *) names[y], (SQLSMALLINT) name_len, &NameLength, &DataType, &ColumnSize, &DecimalDigits, &Nullable);

			if (!ColumnSize) {
				ColumnSize = 255;
			}
			ColumnSize++;

			vals[y] = malloc(ColumnSize);
			memset(vals[y], 0, ColumnSize);
			SQLGetData(stmt, x, SQL_C_CHAR, (SQLCHAR *) vals[y], ColumnSize, NULL);
			y++;
		}

		if (callback(pdata, y, vals, names)) {
			done = 1;
		}

		for (x = 0; x < y; x++) {
			free(names[x]);
			free(vals[x]);
		}
		free(names);
		free(vals);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	stmt = NULL; /* Make sure we don't try to free this handle again */
	
	if (!err_cnt) {
		return AGC_ODBC_SUCCESS;
	}

error:

	if (stmt) {
		err_str = agc_odbc_handle_get_error(handle, stmt);
	}

	if (zstr(err_str) && !zstr(x_err)) {
		err_str = strdup(x_err);
	}

	if (err_str) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, agc_str_nil(err_str));
		if (err) {
			*err = err_str;
		} else {
			free(err_str);
		}
	}

	if (stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}

	return AGC_ODBC_FAIL;

}

AGC_DECLARE(char *) agc_odbc_handle_get_error(agc_odbc_handle_t *handle, agc_odbc_statement_handle_t stmt)
{
	char buffer[SQL_MAX_MESSAGE_LENGTH + 1] = "";
	char sqlstate[SQL_SQLSTATE_SIZE + 1] = "";
	SQLINTEGER sqlcode;
	SQLSMALLINT length;
	char *ret = NULL;

	if (SQLError(handle->env, handle->con, stmt, (SQLCHAR *) sqlstate, &sqlcode, (SQLCHAR *) buffer, sizeof(buffer), &length) == SQL_SUCCESS) {
		ret = agc_mprintf("STATE: %s CODE %ld ERROR: %s\n", sqlstate, sqlcode, buffer);
	};

	return ret;	
}

AGC_DECLARE(int) agc_odbc_handle_affected_rows(agc_odbc_handle_t *handle)
{
	if (!handle) {
		return 0;
	}
	
	return handle->affected_rows;
}

static agc_odbc_status_t init_odbc_handles(agc_odbc_handle_t *handle, agc_bool_t do_reinit)
{
	int result;

	if (!handle) {
		return AGC_ODBC_FAIL;
	}

	if (do_reinit == AGC_TRUE && handle->env != SQL_NULL_HANDLE) {
		SQLFreeHandle(SQL_HANDLE_DBC, handle->con);
		SQLFreeHandle(SQL_HANDLE_ENV, handle->env);
		handle->env = SQL_NULL_HANDLE;
	}	

	if (handle->env == SQL_NULL_HANDLE) {
		result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handle->env);
		
		if ((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error AllocHandle\n");
			handle->env = SQL_NULL_HANDLE; /* Reset handle value, just in case */
			return SWITCH_ODBC_FAIL;
		}

		result = SQLSetEnvAttr(handle->env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
		
		if ((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error SetEnv\n");
			SQLFreeHandle(SQL_HANDLE_ENV, handle->env);
			handle->env = SQL_NULL_HANDLE; /* Reset handle value after it's freed */
			return AGC_ODBC_FAIL;
		}

		result = SQLAllocHandle(SQL_HANDLE_DBC, handle->env, &handle->con);

		if ((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error AllocHDB %d \n", result);
			SQLFreeHandle(SQL_HANDLE_ENV, handle->env);
			handle->env = SQL_NULL_HANDLE; /* Reset handle value after it's freed */
			return AGC_ODBC_FAIL;
		}
		SQLSetConnectAttr(handle->con, SQL_LOGIN_TIMEOUT, (SQLPOINTER *) 10, 0);
	}

	return AGC_ODBC_SUCCESS;
}

static int db_is_up(agc_odbc_handle_t *handle)
{
	int ret = 0;
	SQLHSTMT stmt = NULL;
	SQLLEN m = 0;
	int result;
	agc_event_t *event;
	agc_odbc_status_t recon = 0;
	char *err_str = NULL;
	SQLCHAR sql[255] = "";
	int max_tries = DEFAULT_ODBC_RETRIES;
	int code = 0;
	SQLRETURN rc;
	SQLSMALLINT nresultcols;

	if (handle) {
		max_tries = handle->num_retries;
		if (max_tries < 1)
			max_tries = DEFAULT_ODBC_RETRIES;
	}

top:
	if (!handle) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "No DB Handle \n", result);
		goto done;
	}
	
	if (handle->is_oracle) {
		strcpy((char *) sql, "select 1 from dual");
	} else if (handle->is_firebird) {
		strcpy((char *) sql, "select first 1 * from RDB$RELATIONS");
	} else {
		strcpy((char *) sql, "select 1");
	}

	if (SQLAllocHandle(SQL_HANDLE_STMT, handle->con, &stmt) != SQL_SUCCESS) {
		code = __LINE__;
		goto error;
	}

	SQLSetStmtAttr(stmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);

	if (SQLPrepare(stmt, sql, SQL_NTS) != SQL_SUCCESS) {
		code = __LINE__;
		goto error;
	}

	result = SQLExecute(stmt);
	if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
		code = __LINE__;
		goto error;
	}

	SQLRowCount(stmt, &m);
	rc = SQLNumResultCols(stmt, &nresultcols);
	if (rc != SQL_SUCCESS) {
		code = __LINE__;
		goto error;
	}

	ret = (int) nresultcols;
	if (nresultcols <= 0) {
		/* statement is not a select statement */
		code = __LINE__;
		goto error;
	}

	goto done;

error:
	err_str = agc_odbc_handle_get_error(handle, stmt);
	
	if (stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		stmt = NULL;
	}

	recon = agc_odbc_handle_connect(handle);

	max_tries--;

	agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "The sql server is not responding for DSN %s [%s][%d]\n",  agc_str_nil(handle->dsn), agc_str_nil(err_str), code);
	if (recon == AGC_ODBC_SUCCESS) {
		agc_log_printf(AGC_LOG, AGC_LOG_INFO, "The connection has been re-established \n");
	} else {
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "The connection could not be re-established \n");
	}

	if (!max_tries) {
		agc_log_printf(AGC_LOG, AGC_LOG_CRIT, "Giving up!\n");
		goto done;
	}

	agc_safe_free(err_str);
	agc_yield(1000000);
	goto top;

done:
	agc_safe_free(err_str);

	if (stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}

	return ret;
}




