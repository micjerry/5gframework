#include <agc.h>
#include "private/agc_core_pvt.h"

#define HANDLE_NAME_LEN 256
#define DEFAULT_DB_USER "agc"
#define AGC_MAX_SQL_LEN 32768

struct agc_sql_handle {
	char name[HANDLE_NAME_LEN];
	agc_odbc_handle_t *db_handle;
	agc_time_t last_used;
	agc_mutex_t *mutex;
	agc_memory_pool_t *pool;
	int32_t flags;
	unsigned long thread_hash;
	char creator[HANDLE_NAME_LEN];
	char last_user[HANDLE_NAME_LEN];
	uint32_t use_count;
	uint64_t total_used_count;
	struct agc_sql_handle *next;
};

static struct {
	agc_memory_pool_t *memory_pool;
	agc_thread_t *db_thread;
	int db_thread_running;
	agc_mutex_t *dbh_mutex;
	agc_sql_handle_t *handle_pool;
	uint32_t total_handles;
	uint32_t total_used_handles;
} sql_pool;

static agc_sql_handle_t *create_handle();
static void add_handle(agc_sql_handle_t *dbh, const char *db_callsite_str, const char *thread_str);
static void del_handle(agc_sql_handle_t *dbh);

static agc_sql_handle_t *get_freehandle(const char *user_str, const char *thread_str);

static agc_status_t execute_sql_real(agc_sql_handle_t *dbh, const char *sql, char **err);

agc_status_t agc_sql_start(agc_memory_pool_t *pool)
{
	memset(&sql_pool, 0, sizeof(sql_pool));
	sql_pool.memory_pool = pool;

	agc_mutex_init(&sql_pool.dbh_mutex, AGC_MUTEX_NESTED, sql_pool.memory_pool);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Opening DB\n");

	return AGC_STATUS_SUCCESS;
}

void agc_sql_stop(void)
{
	return;
}

AGC_DECLARE(agc_status_t) _agc_sql_get_handle(agc_sql_handle_t ** dbh, const char *file, const char *func, int line)
{
	agc_thread_id_t self = agc_thread_self();
	char thread_str[HANDLE_NAME_LEN] = "";
	char db_callsite_str[HANDLE_NAME_LEN] = "";
	agc_sql_handle_t *new_dbh = NULL;
	int waiting = 0;
	uint32_t yield_len = 100000, total_yield = 0;

	while(runtime.max_db_handles && sql_pool.total_handles >= runtime.max_db_handles && sql_pool.total_used_handles >= sql_pool.total_handles) {
		if (!waiting++) {
			agc_log_printf(AGC_LOG, AGC_LOG_WARNING, "Max handles %u exceeded, blocking....\n", runtime.max_db_handles);
			agc_yield(yield_len);
			total_yield += yield_len;
			if (runtime.db_handle_timeout && total_yield > runtime.db_handle_timeout) {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Error connecting \n");
				*dbh = NULL;
				return AGC_STATUS_FALSE;
			}
		}
	}

	snprintf(thread_str, sizeof(thread_str) - 1, "thread=\"%lu\"",  (unsigned long) (intptr_t) self); 
	snprintf(db_callsite_str, sizeof(db_callsite_str) - 1, "%s:%d", file, line);
	
	if ((new_dbh = get_freehandle(db_callsite_str, thread_str))) {
		agc_log_printf(AGC_LOG, AGC_LOG_DEBUG, "%s Reuse Unused Cached DB handle \n", func);
	} else {
		agc_odbc_handle_t *odbc_dbh = NULL;
		if ((odbc_dbh = agc_odbc_handle_new(runtime.odbc_dsn, NULL, NULL))) {
			if (agc_odbc_handle_connect(odbc_dbh) != AGC_ODBC_SUCCESS) {
				agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Connect DB failed \n");
				agc_odbc_handle_destroy(&odbc_dbh);
				*dbh = NULL;
				return AGC_STATUS_FALSE;
			}
		}

		new_dbh = create_handle();
		if (!new_dbh) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "Create sql handle failed \n");
			agc_odbc_handle_destroy(&odbc_dbh);
			*dbh = NULL;
			return AGC_STATUS_FALSE;
		}

		new_dbh->db_handle = odbc_dbh;
		add_handle(new_dbh, db_callsite_str, thread_str);
	}

	if (new_dbh) {
		new_dbh->last_used = agc_timer_curtime();
	}

	*dbh  = new_dbh;

	return *dbh ? AGC_STATUS_SUCCESS : AGC_STATUS_FALSE;
}

AGC_DECLARE(void) agc_sql_release_handle(agc_sql_handle_t ** dbh)
{
	if (dbh && *dbh) {
		agc_mutex_lock(sql_pool.dbh_mutex);

		(*dbh)->last_used = agc_timer_curtime();

		if ((*dbh)->use_count) {
			if (--(*dbh)->use_count == 0) {
				(*dbh)->thread_hash = 1;
			}
		}
		agc_mutex_unlock((*dbh)->mutex);
		sql_pool.total_used_handles--;
		*dbh = NULL;
		agc_mutex_unlock(sql_pool.dbh_mutex);
	}
}

AGC_DECLARE(char *) agc_sql_execute_sql2str(agc_sql_handle_t *dbh, char *sql, char *str, size_t len, char **err)
{
	agc_status_t status = AGC_STATUS_FALSE;

	memset(str, 0, len);
	
	status = agc_odbc_handle_exec_string(dbh->db_handle, sql, str, len, err);

	return status == AGC_STATUS_SUCCESS ? str : NULL;
}

AGC_DECLARE(agc_status_t) agc_sql_execute_sql(agc_sql_handle_t *dbh, char *sql, char **err)
{
	agc_status_t status = AGC_STATUS_FALSE;
	agc_size_t len;
	char *p, *s, *e;

	len = strlen(sql);
	
	if (len < AGC_MAX_SQL_LEN) {
		return execute_sql_real(dbh, sql, err);
	}

	e = end_of_p(sql);
	s = sql;
	
	while (s && s < e) {
		p = s + AGC_MAX_SQL_LEN;

		if (p > e) {
			p = e;
		}

		while (p > s) {
			if (*p == '\n' && *(p - 1) == ';') {
				*p = '\0';
				*(p - 1) = '\0';
				p++;
				break;
			}

			p--;
		}

		if (p <= s)
			break;


		status = execute_sql_real(dbh, s, err);
		if (status != AGC_STATUS_SUCCESS || (err && *err)) {
			break;
		}

		s = p;

	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_sql_execute_sql_callback(agc_sql_handle_t *dbh, const char *sql,
																	 agc_db_callback_func_t callback, void *pdata, char **err)
{
	agc_status_t status = AGC_STATUS_FALSE;

	if (err) {
		*err = NULL;
	}

	status = agc_odbc_handle_callback_exec(dbh->db_handle, sql, callback, pdata, err);

	return status;
}

AGC_DECLARE(agc_status_t) agc_sql_execute_sql_callback_err(agc_sql_handle_t *dbh, const char *sql,
																	 agc_db_callback_func_t callback,
																	 agc_db_err_callback_func_t err_callback,
																	 void *pdata, char **err)
{
	agc_status_t status = AGC_STATUS_FALSE;

	if (err) {
		*err = NULL;
	}

	status = agc_odbc_handle_callback_exec(dbh->db_handle, sql, callback, pdata, err);
	if (err && *err) {
		(*err_callback)(pdata, (const char*)*err);
	}

	return status;
}

AGC_DECLARE(int) agc_sql_affected_rows(agc_sql_handle_t *dbh)
{
	if (!dbh) {
		return 0;
	}
	return agc_odbc_handle_affected_rows(dbh->db_handle);
}

AGC_DECLARE(agc_status_t) agc_sql_persistant_execute(agc_sql_handle_t *dbh, const char *sql, uint32_t retries)
{
	agc_status_t status = AGC_STATUS_FALSE;
	char *errmsg = NULL;

	if (!retries) {
		retries = 1000;
	}

	while (retries > 0) {
		execute_sql_real(dbh, sql, &errmsg);

		if (errmsg) {
			agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL ERR [%s]\n",  errmsg);
			agc_safe_free(errmsg);
			agc_yield(100000);
			retries--;
		} else {
			status = AGC_STATUS_SUCCESS;
			break;
		}
	}

	return status;
}

static agc_sql_handle_t *create_handle()
{
	agc_sql_handle_t *new_dbh = NULL;
	agc_memory_pool_t *pool = NULL;

	if (agc_memory_create_pool(&pool) != AGC_STATUS_SUCCESS) {
		return NULL;
	}

	new_dbh = agc_memory_alloc(pool, sizeof(*new_dbh));
	new_dbh->pool = pool;
	agc_mutex_init(&new_dbh->mutex, AGC_MUTEX_NESTED, new_dbh->pool);
	
	return new_dbh;
}

static void add_handle(agc_sql_handle_t *dbh, const char *db_callsite_str, const char *thread_str)
{
	agc_ssize_t hlen = -1;

	agc_mutex_lock(sql_pool.dbh_mutex);

	agc_set_string(dbh->creator, db_callsite_str);

	dbh->thread_hash = agc_ci_hashfunc_default(thread_str, &hlen);

	dbh->use_count++;
	dbh->total_used_count++;

	sql_pool.total_used_handles++;
	dbh->next = sql_pool.handle_pool;
	sql_pool.handle_pool = dbh;
	sql_pool.total_handles++;

	agc_mutex_lock(dbh->mutex);
	agc_mutex_unlock(sql_pool.dbh_mutex);
}

static void del_handle(agc_sql_handle_t *dbh)
{
	agc_sql_handle_t *dbh_ptr, *last = NULL;

	agc_mutex_lock(sql_pool.dbh_mutex);

	for (dbh_ptr = sql_pool.handle_pool; dbh_ptr; dbh_ptr = dbh_ptr->next) {
		if (dbh_ptr == dbh) {
			if (last) {
				last->next = dbh_ptr->next;
			} else {
				sql_pool.handle_pool = dbh_ptr->next;
			}
			sql_pool.total_handles--;
			break;
		}
		
		last = dbh_ptr;
	}

	agc_mutex_unlock(sql_pool.dbh_mutex);
}

static agc_sql_handle_t *get_freehandle(const char *user_str, const char *thread_str)
{
	agc_ssize_t hlen = -1;
	unsigned long hash = 0, thread_hash = 0;
	agc_sql_handle_t *dbh_ptr, *r = NULL;

	thread_hash = agc_ci_hashfunc_default(thread_str, &hlen);

	agc_mutex_lock(sql_pool.dbh_mutex);

	for (dbh_ptr = sql_pool.handle_pool; dbh_ptr; dbh_ptr = dbh_ptr->next) {
		if ((dbh_ptr->thread_hash == thread_hash) 
			&& (!dbh_ptr->use_count) 
			&& (agc_mutex_trylock(dbh_ptr->mutex) == AGC_STATUS_SUCCESS)) {
			r = dbh_ptr;
			break;
		}
			
	}

	if (!r) {
		for (dbh_ptr = sql_pool.handle_pool; dbh_ptr; dbh_ptr = dbh_ptr->next) {
			if ((agc_mutex_trylock(dbh_ptr->mutex) == AGC_STATUS_SUCCESS)) {
				r = dbh_ptr;
				break;
			}
			
		}
	}

	if (r) {
		r->use_count++;
		r->total_used_count++;
		sql_pool.total_used_handles++;
		r->thread_hash = thread_hash;
		agc_set_string(r->last_user, user_str);
	}
	
	agc_mutex_unlock(sql_pool.dbh_mutex);

	return r;
}

static agc_status_t execute_sql_real(agc_sql_handle_t *dbh, const char *sql, char **err)
{
	agc_status_t status = AGC_STATUS_FALSE;
	char *errmsg = NULL;

	if (err) {
		*err = NULL;
	}

	status = agc_odbc_handle_exec(dbh->db_handle, sql, NULL, &errmsg);

	if (errmsg) {
		agc_log_printf(AGC_LOG, AGC_LOG_ERROR, "SQL ERR [%s]\n%s\n",  errmsg, sql);
		
		if (err) {
			*err = errmsg;
		} else {
			agc_safe_free(errmsg);
		}
	}

	return status;
}

