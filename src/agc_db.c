#include <agc_db.h>

AGC_DECLARE(int) agc_db_open(const char *filename, agc_db_t **ppdb)
{
	return sqlite3_open(filename, ppdb);
}

AGC_DECLARE(int) agc_db_close(agc_db_t *db)
{
	return sqlite3_close(db);
}

AGC_DECLARE(const char *) agc_db_column_name(agc_db_stmt_t *stmt, int N)
{
	return sqlite3_column_name(stmt, N);
}

AGC_DECLARE(int) agc_db_column_count(agc_db_stmt_t *stmt)
{
	return sqlite3_column_count(stmt);
}

AGC_DECLARE(const char *) agc_db_errmsg(agc_db_t *db)
{
	return sqlite3_errmsg(db); 
}

AGC_DECLARE(const unsigned char *) agc_db_column_text(agc_db_stmt_t *stmt, int iCol)
{
	const unsigned char *txt = sqlite3_column_text(stmt, iCol);

	if (!strcasecmp((char *) stmt, "(null)")) {
		memset(stmt, 0, 1);
		txt = NULL;
	}

	return txt;
}

AGC_DECLARE(const void *) agc_db_column_blob(agc_db_stmt_t *stmt, int iCol)
{
	return sqlite3_column_blob(stmt, iCol);
}

AGC_DECLARE(int) agc_db_column_bytes(agc_db_stmt_t *stmt, int iCol)
{
	return sqlite3_column_bytes(stmt, iCol);
}

AGC_DECLARE(int) agc_db_column_int(agc_db_stmt_t *stmt, int iCol)
{
	return sqlite3_column_int(stmt, iCol);
}

AGC_DECLARE(int64_t) agc_db_column_int64(agc_db_stmt_t *stmt, int iCol)
{
	sqlite3_int64  value = sqlite3_column_int64(stmt, iCol);
	return (int64_t)value;
}

AGC_DECLARE(int) agc_db_exec(agc_db_t *db, const char *sql, agc_db_callback_func_t callback, void *data, char **errmsg)
{
	int ret = 0;
	int retries = 10;
	char *err = NULL;

	while (--retries > 0) {
		ret = sqlite3_exec(db, sql, callback, data, &err);
		if (ret == SQLITE_BUSY || ret == SQLITE_LOCKED) {
			if (retries > 1) {
				agc_db_free(err);
				agc_yield(100000);
				continue;
			}
		} else {
			break;
		}
	}

	if (errmsg) {
		*errmsg = err;
	} else if (err) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL ERR [%s]\n", err);
		agc_db_free(err);
		err = NULL;
	}

	return ret;
}

AGC_DECLARE(int) agc_db_prepare(agc_db_t *db, const char *sql, int bytes, agc_db_stmt_t **ppstmt, const char **pptail)
{
	return sqlite3_prepare(db, sql, bytes, ppstmt, pptail);
}

AGC_DECLARE(int) agc_db_finalize(agc_db_stmt_t *stmt)
{
	return sqlite3_finalize(stmt);
}

AGC_DECLARE(int) agc_db_step(agc_db_stmt_t *stmt)
{
	return sqlite3_step(stmt);
}

AGC_DECLARE(int) agc_db_reset(agc_db_stmt_t *stmt)
{
	return sqlite3_reset(stmt);
}

AGC_DECLARE(int) agc_db_bind_int(agc_db_stmt_t *stmt, int i, int iValue)
{
	return sqlite3_bind_int(stmt, i, iValue);
}

AGC_DECLARE(int) agc_db_bind_int64(agc_db_stmt_t *stmt, int i, int64_t iValue)
{
	return sqlite3_bind_int64(stmt, i, iValue);
}

AGC_DECLARE(int) agc_db_bind_text(agc_db_stmt_t *stmt, int i, const char *zData, int nData, agc_db_destructor_type_t xDel)
{
	return sqlite3_bind_text(stmt, i, zData, nData, xDel);
}

AGC_DECLARE(int) agc_db_bind_double(agc_db_stmt_t *stmt, int i, double dValue)
{
	return sqlite3_bind_double(stmt, i, dValue);
}

AGC_DECLARE(int64_t) agc_db_last_insert_rowid(agc_db_t *db)
{
	return sqlite3_last_insert_rowid(db);
}


AGC_DECLARE(int) agc_db_get_table(agc_db_t *db,	
											 const char *sql,	
											 char ***resultp,	
											 int *nrow,	
											 int *ncolumn,
											 char **errmsg
	)
{
	return sqlite3_get_table(db, sql, resultp, nrow, ncolumn, errmsg);	
}

AGC_DECLARE(void) agc_db_free_table(char **result)
{
	sqlite3_free_table(result);
}

AGC_DECLARE(void) agc_db_free(char *z)
{
	sqlite3_free(z);
}

AGC_DECLARE(int) agc_db_changes(agc_db_t *db)
{
	return sqlite3_changes(db);
}

AGC_DECLARE(int) agc_db_load_extension(agc_db_t *db, const char *extension)
{
	int ret = 0;
	char *err = NULL;

	sqlite3_enable_load_extension(db, 1);
	ret = sqlite3_load_extension(db, extension, 0, &err);
	sqlite3_enable_load_extension(db, 0);

	if (err) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "LOAD EXTENSION  ERR [%s]\n", err);
		agc_db_free(err);
		err = NULL;
	}
	return ret;	
}

AGC_DECLARE(agc_db_t *) agc_db_open_file(const char *filename)
{
	agc_db_t *db;
	char path[1024];
	int db_ret;
	int failed = 0;

	memset(path, 0, sizeof(path));

	agc_snprintf(path, sizeof(path), "%s%s%s", AGC_GLOBAL_dirs.db_dir, AGC_PATH_SEPARATOR, filename);

	agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "LOAD DB  PATH [%s]\n", path);
	
	if ((db_ret = agc_db_open(path, &db)) != SQLITE_OK) {
		failed = 1;
	}

	if (!failed && (db_ret = agc_db_exec(db, "PRAGMA synchronous=OFF;", NULL, NULL, NULL) != SQLITE_OK)) {
		failed = 1;
	}

	if (!failed && (db_ret = agc_db_exec(db, "PRAGMA count_changes=OFF;", NULL, NULL, NULL) != SQLITE_OK)) {
		failed = 1;
	}	

	if (!failed && (db_ret = agc_db_exec(db, "PRAGMA cache_size=8000;", NULL, NULL, NULL) != SQLITE_OK)) {
		failed = 1;
	}

	if (!failed && (db_ret = agc_db_exec(db, "PRAGMA temp_store=MEMORY;", NULL, NULL, NULL) != SQLITE_OK)) {
		failed = 1;
	}

	if (db_ret != SQLITE_OK) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL  ERR [%s]\n", agc_db_errmsg(db));
		agc_db_close(db);
		db = NULL;
	}

	return db;
	
}

AGC_DECLARE(agc_status_t) agc_db_persistant_execute(agc_db_t *db, char *sql, uint32_t retries)
{
	char *errmsg;
	agc_status_t status = AGC_STATUS_FALSE;

	if (!retries || retries > MAX_DB_RETRIES) {
		retries = MAX_DB_RETRIES;
	}

	while (retries > 0) {
		agc_db_exec(db, sql, NULL, NULL, &errmsg);
		if (errmsg) {
			agc_db_free(errmsg);
			agc_yield(100000);
			retries--;
			
		} else {
			status = AGC_STATUS_SUCCESS;
			break;
		}
	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_db_persistant_execute_trans(agc_db_t *db, char *sql, uint32_t retries)
{
	char *errmsg;
	agc_status_t status = AGC_STATUS_FALSE;
	unsigned begin_retries = MAX_DB_RETRIES;
	uint8_t again = 0;
	int has_trans = 0;

	if (!retries || retries > MAX_DB_RETRIES) {
		retries = MAX_DB_RETRIES;
	}

	while (begin_retries > 0) {
		agc_db_exec(db, "BEGIN", NULL, NULL, &errmsg);
		if (errmsg) {
			begin_retries--;
			if (strstr(errmsg, "cannot start a transaction within a transaction")) {
				has_trans = 1;
				agc_db_exec(db, "COMMIT", NULL, NULL, NULL);
			} else {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL Retry [%s]\n", errmsg);
			}

			agc_db_free(errmsg);
			errmsg = NULL;

			if (has_trans)
				continue;
			
			agc_yield(100000);
			
		} else {
			break;
		}
	}

	if (begin_retries == 0) {
		agc_db_exec(db, "COMMIT", NULL, NULL, NULL);
		return status;
	}

	while (retries > 0) {
		agc_db_exec(db, sql, NULL, NULL, &errmsg);
		if (errmsg) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL ERR [%s]\n", errmsg);
			agc_db_free(errmsg);
			errmsg = NULL;
			agc_yield(100000);
			retries--;
		} else {
			status = AGC_STATUS_SUCCESS;
			break;
		}
	}

	agc_db_exec(db, "COMMIT", NULL, NULL, NULL);
	return status;
}

