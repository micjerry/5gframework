#ifndef AGC_DB_H
#define AGC_DB_H

#include <agc.h>

AGC_BEGIN_EXTERN_C
#include <sqlite3.h>

#define MAX_DB_RETRIES 5

typedef struct sqlite3 agc_db_t;
typedef struct sqlite3_stmt agc_db_stmt_t;

typedef int (*agc_db_callback_func_t) (void *pArg, int argc, char **argv, char **columnNames);
typedef int (*agc_db_err_callback_func_t) (void *pArg, const char *errmsg);

typedef void (*agc_db_destructor_type_t) (void *);
#define AGC_CORE_DB_STATIC      ((agc_db_destructor_type_t)0)
#define AGC_CORE_DB_TRANSIENT   ((agc_db_destructor_type_t)-1)

/**
 * Open the database file "filename".  The "filename" is UTF-8
 * encoded.  A agc_db_t* handle is returned in *Db, even
 * if an error occurs. If the database is opened (or created) successfully,
 * then AGC_DB_OK is returned. Otherwise an error code is returned. The
 * agc_db_errmsg() routine can be used to obtain
 * an English language description of the error.
 *
 * If the database file does not exist, then a new database is created.
 * The encoding for the database is UTF-8.
 *
 * Whether or not an error occurs when it is opened, resources associated
 * with the agc_db_t* handle should be released by passing it to
 * agc_db_close() when it is no longer required.
 */
AGC_DECLARE(int) agc_db_open(const char *filename, agc_db_t **ppdb);

/**
 * A function to close the database.
 *
 * Call this function with a pointer to a structure that was previously
 * returned from agc_db_open() and the corresponding database will by closed.
 *
 * All SQL statements prepared using agc_db_prepare()
 * must be deallocated using agc_db_finalize() before
 * this routine is called. Otherwise, AGC_DB_BUSY is returned and the
 * database connection remains open.
 */
AGC_DECLARE(int) agc_db_close(agc_db_t *db);

AGC_DECLARE(const char *) agc_db_errmsg(agc_db_t *db);

/**
 * The first parameter is a compiled SQL statement. This function returns
 * the column heading for the Nth column of that statement, where N is the
 * second function parameter.  The string returned is UTF-8.
 * The leftmost column of the result set has the index 0
 */
AGC_DECLARE(const char *) agc_db_column_name(agc_db_stmt_t *stmt, int N);

/**
 * Return the number of columns in the result set returned by the compiled
 * SQL statement. This routine returns 0 if pStmt is an SQL statement
 * that does not return data (for example an UPDATE).
 */
AGC_DECLARE(int) agc_db_column_count(agc_db_stmt_t *stmt);


/**
 *    Internal Type    Requested Type     Conversion
 *    -------------    --------------    --------------------------
 *       NULL             TEXT            Result is an empty string
 *       INTEGER         TEXT            ASCII rendering of the integer
 *       FLOAT            TEXT            ASCII rendering of the float
 *       BLOB             TEXT            Add a "\000" terminator if needed
 *
 *  Return the value as UTF-8 text.
 * The leftmost column of the result set has the index 0
 */
AGC_DECLARE(const unsigned char *) agc_db_column_text(agc_db_stmt_t *stmt, int iCol);

/**
 *    Internal Type    Requested Type     Conversion
 *    -------------    --------------    --------------------------
 *       NULL              BLOB            Result is a NULL pointer
 *       INTEGER         BLOB            Same as INTEGER->TEXT
 *       FLOAT            BLOB            CAST to BLOB
 *       TEXT              BLOB            No change
 *
 * The leftmost column of the result set has the index 0
 */
AGC_DECLARE(const void *) agc_db_column_blob(agc_db_stmt_t *stmt, int iCol);

//Return the size of text or blob, The leftmost column of the result set has the index 0
AGC_DECLARE(int) agc_db_column_bytes(agc_db_stmt_t *stmt, int iCol);

/**
 *    Internal Type    Requested Type     Conversion
 *    -------------    --------------    --------------------------
 *       NULL             INTEGER         Result is 0
 *       FLOAT           INTEGER         Convert from float to integer
 *       TEXT             INTEGER         Use atoi()
 *       BLOB            INTEGER         Convert to TEXT then use atoi()
 *
 * The leftmost column of the result set has the index 0
 */
AGC_DECLARE(int) agc_db_column_int(agc_db_stmt_t *stmt, int iCol);

AGC_DECLARE(int64_t) agc_db_column_int64(agc_db_stmt_t *stmt, int iCol);

/**
 * A function to executes one or more statements of SQL.
 *
 * If one or more of the SQL statements are queries, then
 * the callback function specified by the 3rd parameter is
 * invoked once for each row of the query result.  This callback
 * should normally return 0.  If the callback returns a non-zero
 * value then the query is aborted, all subsequent SQL statements
 * are skipped and the agc_db_exec() function returns the AGC_DB_ABORT.
 *
 * The 4th parameter is an arbitrary pointer that is passed
 * to the callback function as its first parameter.
 *
 * The 2nd parameter to the callback function is the number of
 * columns in the query result.  The 3rd parameter to the callback
 * is an array of strings holding the values for each column.
 * The 4th parameter to the callback is an array of strings holding
 * the names of each column.
 *
 * The callback function may be NULL, even for queries.  A NULL
 * callback is not an error.  It just means that no callback
 * will be invoked.
 *
 * If an error occurs while parsing or evaluating the SQL (but
 * not while executing the callback) then an appropriate error
 * message is written into memory obtained from malloc() and
 * *errmsg is made to point to that message.  The calling function
 * is responsible for freeing the memory that holds the error
 * message.   Use agc_db_free() for this.  If errmsg==NULL,
 * then no error message is ever written.
 *
 * The return value is is AGC_DB_OK if there are no errors and
 * some other return code if there is an error.  The particular
 * return value depends on the type of error. 
 *
 * If the query could not be executed because a database file is
 * locked or busy, then this function returns AGC_DB_BUSY.  (This
 * behavior can be modified somewhat using the agc_db_busy_handler()
 * and agc_db_busy_timeout() functions below.)
 */
AGC_DECLARE(int) agc_db_exec(agc_db_t *db, const char *sql, agc_db_callback_func_t callback, void *data, char **errmsg);

/**
 * To execute an SQL query, it must first be compiled into a byte-code
 * program using the following routine.
 *
 * The first parameter "db" is an SQLite database handle. The second
 * parameter "sql" is the statement to be compiled, encoded as
 * UTF-8. If the next parameter, "nBytes", is less
 * than zero, then sql is read up to the first nul terminator.  If
 * "nBytes" is not less than zero, then it is the length of the string sql
 * in bytes (not characters).
 *
 * *pptail is made to point to the first byte past the end of the first
 * SQL statement in sql.  This routine only compiles the first statement
 * in sql, so *pzTail is left pointing to what remains uncompiled.
 *
 * *ppstmt is left pointing to a compiled SQL statement that can be
 * executed using agc_db_step().  Or if there is an error, *ppStmt may be
 * set to NULL.  If the input text contained no SQL (if the input is and
 * empty string or a comment) then *ppStmt is set to NULL.
 *
 * On success, AGC_DB_OK is returned.  Otherwise an error code is returned.
 */
AGC_DECLARE(int) agc_db_prepare(agc_db_t *db, const char *sql, int bytes, agc_db_stmt_t **ppstmt, const char **pptail);

/**
 * This function is called to delete a compiled
 * SQL statement obtained by a previous call to agc_db_prepare().
 * If the statement was executed successfully, or
 * not executed at all, then AGC_DB_OK is returned. If execution of the
 * statement failed then an error code is returned. 
 *
 * This routine can be called at any point during the execution of the
 * virtual machine.  If the virtual machine has not completed execution
 * when this routine is called, that is like encountering an error or
 * an interrupt.  (See agc_db_interrupt().)  Incomplete updates may be
 * rolled back and transactions cancelled,  depending on the circumstances,
 * and the result code returned will be AGC_DB_ABORT.
 */
AGC_DECLARE(int) agc_db_finalize(agc_db_stmt_t *stmt);


/** 
 * After an SQL query has been compiled with a call to either
 * agc_db_prepare(), then this function must be
 * called one or more times to execute the statement.
 *
 * The return value will be either AGC_DB_BUSY, AGC_DB_DONE, 
 * AGC_DB_ROW, AGC_DB_ERROR, or AGC_DB_MISUSE.
 *
 * AGC_DB_BUSY means that the database engine attempted to open
 * a locked database and there is no busy callback registered.
 * Call agc_db_step() again to retry the open.
 *
 * AGC_DB_DONE means that the statement has finished executing
 * successfully.  agc_db_step() should not be called again on this virtual
 * machine.
 *
 * If the SQL statement being executed returns any data, then 
 * AGC_DB_ROW is returned each time a new row of data is ready
 * for processing by the caller. The values may be accessed using
 * the agc_db_column_*() functions described below. agc_db_step()
 * is called again to retrieve the next row of data.
 * 
 * AGC_DB_ERROR means that a run-time error (such as a constraint
 * violation) has occurred.  agc_db_step() should not be called again on
 * the VM. More information may be found by calling agc_db_errmsg().
 *
 * AGC_DB_MISUSE means that the this routine was called inappropriately.
 * Perhaps it was called on a virtual machine that had already been
 * finalized or on one that had previously returned AGC_DB_ERROR or
 * AGC_DB_DONE.  Or it could be the case the the same database connection
 * is being used simulataneously by two or more threads.
 */
AGC_DECLARE(int) agc_db_step(agc_db_stmt_t *stmt);

/**
 * The agc_db_reset() function is called to reset a compiled SQL
 * statement obtained by a previous call to agc_db_prepare()
 * back to it's initial state, ready to be re-executed.
 * Any SQL statement variables that had values bound to them using
 * the agc_db_bind_*() API retain their values.
 */
AGC_DECLARE(int) agc_db_reset(agc_db_stmt_t *stmt);

/**
 * In the SQL strings input to agc_db_prepare(),
 * one or more literals can be replace by parameters "?" or ":AAA" or
 * "$VVV" where AAA is an identifer and VVV is a variable name according
 * to the syntax rules of the TCL programming language.
 * The value of these parameters (also called "host parameter names") can
 * be set using the routines listed below.
 *
 * In every case, the first parameter is a pointer to the sqlite3_stmt
 * structure returned from agc_db_prepare().  The second parameter is the
 * index of the parameter.  The leftmost SQL parameter has an index of 1.  For
 * named parameters (":AAA" or "$VVV") you can use 
 * agc_db_bind_parameter_index() to get the correct index value given
 * the parameters name.  If the same named parameter occurs more than
 * once, it is assigned the same index each time.
 *
 * The agc_db_bind_* routine must be called before agc_db_step() after
 * an agc_db_prepare() or sqlite3_reset().  Unbound parameterss are
 * interpreted as NULL.
 */
AGC_DECLARE(int) agc_db_bind_int(agc_db_stmt_t *stmt, int i, int iValue);

AGC_DECLARE(int) agc_db_bind_int64(agc_db_stmt_t *stmt, int i, int64_t iValue);

AGC_DECLARE(int) agc_db_bind_text(agc_db_stmt_t *stmt, int i, const char *zData, int nData, agc_db_destructor_type_t xDel);

AGC_DECLARE(int) agc_db_bind_double(agc_db_stmt_t *stmt, int i, double dValue);

/**
 * Each entry in a table has a unique integer key.  (The key is
 * the value of the INTEGER PRIMARY KEY column if there is such a column,
 * otherwise the key is generated at random.  The unique key is always
 * available as the ROWID, OID, or _ROWID_ column.)  The following routine
 * returns the integer key of the most recent insert in the database.
 *
 * This function is similar to the mysql_insert_id() function from MySQL.
 */
AGC_DECLARE(int64_t) agc_db_last_insert_rowid(agc_db_t *db);

/**
 * This next routine is really just a wrapper around agc_db_exec().
 * Instead of invoking a user-supplied callback for each row of the
 * result, this routine remembers each row of the result in memory
 * obtained from malloc(), then returns all of the result after the
 * query has finished. 
 *
 * As an example, suppose the query result where this table:
 *
 *        Name        | Age
 *        -----------------------
 *        Alice       | 43
 *        Bob         | 28
 *        Cindy       | 21
 *
 * If the 3rd argument were &azResult then after the function returns
 * azResult will contain the following data:
 *
 *        azResult[0] = "Name";
 *        azResult[1] = "Age";
 *        azResult[2] = "Alice";
 *        azResult[3] = "43";
 *        azResult[4] = "Bob";
 *        azResult[5] = "28";
 *        azResult[6] = "Cindy";
 *        azResult[7] = "21";
 *
 * Notice that there is an extra row of data containing the column
 * headers.  But the *nrow return value is still 3.  *ncolumn is
 * set to 2.  In general, the number of values inserted into azResult
 * will be ((*nrow) + 1)*(*ncolumn).
 *
 * After the calling function has finished using the result, it should 
 * pass the result data pointer to agc_db_free_table() in order to 
 * release the memory that was malloc-ed.  Because of the way the 
 * malloc() happens, the calling function must not try to call 
 * free() directly.  Only agc_db_free_table() is able to release 
 * the memory properly and safely.
 *
 * The return value of this routine is the same as from agc_db_exec().
 */
AGC_DECLARE(int) agc_db_get_table(agc_db_t *db,	/* An open database */
											 const char *sql,	/* SQL to be executed */
											 char ***resultp,	/* Result written to a char *[]  that this points to */
											 int *nrow,	/* Number of result rows written here */
											 int *ncolumn,	/* Number of result columns written here */
											 char **errmsg	/* Error msg written here */
	);

/**
 * Call this routine to free the resultp memory that agc_get_table() allocated.
 */
AGC_DECLARE(void) agc_db_free_table(char **result);


/**
 * Call this routine to free the errmsg memory that agc_db_get_table() allocated.
 */
AGC_DECLARE(void) agc_db_free(char *z);

/**
 * Call this routine to find the number of rows changed by the last statement.
 */
AGC_DECLARE(int) agc_db_changes(agc_db_t *db);

/**
 * Call this routine to load an external extension
 */
AGC_DECLARE(int) agc_db_load_extension(agc_db_t *db, const char *extension);

/** Return values for agc_db_exec() and agc_db_step()*/
#define AGC_DB_OK           0	/* Successful result */
/* beginning-of-error-codes */
#define AGC_DB_ERROR        1	/* SQL error or missing database */
#define AGC_DB_INTERNAL     2	/* NOT USED. Internal logic error in SQLite */
#define AGC_DB_PERM         3	/* Access permission denied */
#define AGC_DB_ABORT        4	/* Callback routine requested an abort */
#define AGC_DB_BUSY         5	/* The database file is locked */
#define AGC_DB_LOCKED       6	/* A table in the database is locked */
#define AGC_DB_NOMEM        7	/* A malloc() failed */
#define AGC_DB_READONLY     8	/* Attempt to write a readonly database */
#define AGC_DB_INTERRUPT    9	/* Operation terminated by agc_db_interrupt() */
#define AGC_DB_IOERR       10	/* Some kind of disk I/O error occurred */
#define AGC_DB_CORRUPT     11	/* The database disk image is malformed */
#define AGC_DB_NOTFOUND    12	/* NOT USED. Table or record not found */
#define AGC_DB_FULL        13	/* Insertion failed because database is full */
#define AGC_DB_CANTOPEN    14	/* Unable to open the database file */
#define AGC_DB_PROTOCOL    15	/* Database lock protocol error */
#define AGC_DB_EMPTY       16	/* Database is empty */
#define AGC_DB_SCHEMA      17	/* The database schema changed */
#define AGC_DB_TOOBIG      18	/* NOT USED. Too much data for one row */
#define AGC_DB_CONSTRAINT  19	/* Abort due to contraint violation */
#define AGC_DB_MISMATCH    20	/* Data type mismatch */
#define AGC_DB_MISUSE      21	/* Library used incorrectly */
#define AGC_DB_NOLFS       22	/* Uses OS features not supported on host */
#define AGC_DB_AUTH        23	/* Authorization denied */
#define AGC_DB_FORMAT      24	/* Auxiliary database format error */
#define AGC_DB_RANGE       25	/* 2nd parameter to agc_db_bind out of range */
#define AGC_DB_NOTADB      26	/* File opened that is not a database file */
#define AGC_DB_ROW         100	/* agc_db_step() has another row ready */
#define AGC_DB_DONE        101	/* agc_db_step() has finished executing */
/* end-of-error-codes */

/**
 * This routine is a variant of the "sprintf()" from the
 * standard C library.  The resulting string is written into memory
 * obtained from malloc() so that there is never a possiblity of buffer
 * overflow.  This routine also implement some additional formatting
 * options that are useful for constructing SQL statements.
 *
 * The strings returned by this routine should be freed by calling
 * agc_db_free().
 *
 * All of the usual printf formatting options apply.  In addition, there
 * is a "%q" option.  %q works like %s in that it substitutes a null-terminated
 * string from the argument list.  But %q also doubles every '\'' character.
 * %q is designed for use inside a string literal.  By doubling each '\''
 * character it escapes that character and allows it to be inserted into
 * the string.
 *
 * For example, so some string variable contains text as follows:
 *
 *      char *zText = "It's a happy day!";
 *
 * We can use this text in an SQL statement as follows:
 *
 *      char *z = agc_db_mprintf("INSERT INTO TABLES('%q')", zText);
 *      agc_db_exec(db, z, callback1, 0, 0);
 *      agc_db_free(z);
 *
 * Because the %q format string is used, the '\'' character in zText
 * is escaped and the SQL generated is as follows:
 *
 *      INSERT INTO table1 VALUES('It''s a happy day!')
 *
 * This is correct.  Had we used %s instead of %q, the generated SQL
 * would have looked like this:
 *
 *      INSERT INTO table1 VALUES('It's a happy day!');
 *
 * This second example is an SQL syntax error.  As a general rule you
 * should always use %q instead of %s when inserting text into a string 
 * literal.
 */

AGC_DECLARE(char*)agc_sql_concat(void);


AGC_DECLARE(agc_db_t *) agc_db_open_file(const char *filename);

AGC_DECLARE(agc_status_t) agc_db_persistant_execute(agc_db_t *db, char *sql, uint32_t retries);

AGC_DECLARE(agc_status_t) agc_db_persistant_execute_trans(agc_db_t *db, char *sql, uint32_t retries);

AGC_END_EXTERN_C

#endif
