#include <agc.h>

#include "private/agc_core_pvt.h"

#include <apr.h>
#include <apr_atomic.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_network_io.h>
#include <apr_errno.h>
#include <apr_thread_proc.h>
#include <apr_portable.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_thread_rwlock.h>
#include <apr_file_io.h>
#include <apr_poll.h>
#include <apr_strings.h>
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_file_info.h>
#include <apr_fnmatch.h>
#include <apr_tables.h>

/* apr_vformatter_buff_t definition*/
#include <apr_lib.h>

/* apr-util headers */
#include <apr_queue.h>
#include <apr_uuid.h>
#include <apr_md5.h>

AGC_DECLARE(int) agc_status_is_timeup(int status)
{
	return APR_STATUS_IS_TIMEUP(status);
}

AGC_DECLARE(apr_thread_id_t) agc_thread_self(void)
{
    return apr_os_thread_current();
}

AGC_DECLARE(int) agc_thread_equal(agc_thread_id_t tid1, agc_thread_id_t tid2)
{
    return apr_os_thread_equal(tid1, tid2);
}

AGC_DECLARE(unsigned int) agc_ci_hashfunc_default(const char *char_key, agc_ssize_t *klen)
{
	unsigned int hash = 0;
	const unsigned char *key = (const unsigned char *) char_key;
	const unsigned char *p;
	apr_ssize_t i;

	if (*klen == APR_HASH_KEY_STRING) {
		for (p = key; *p; p++) {
			hash = hash * 33 + tolower(*p);
		}
		*klen = p - key;
	} else {
		for (p = key, i = *klen; i; i--, p++) {
			hash = hash * 33 + tolower(*p);
		}
	}

	return hash;
}

AGC_DECLARE(unsigned int) agc_hashfunc_default(const char *key, agc_ssize_t *klen)
{
	return apr_hashfunc_default(key, klen);
}

AGC_DECLARE(agc_status_t) agc_strftime(char *s, agc_size_t *retsize, agc_size_t max, const char *format, agc_time_exp_t *tm)
{
	const char *p = format;

	if (!p)
		return AGC_STATUS_FALSE;

	while (*p) {
		if (*p == '%') {
			switch (*(++p)) {
			case 'C':
			case 'D':
			case 'r':
			case 'R':
			case 'T':
			case 'e':
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'c':
			case 'd':
			case 'H':
			case 'I':
			case 'j':
			case 'm':
			case 'M':
			case 'p':
			case 'S':
			case 'U':
			case 'w':
			case 'W':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
			case 'z':
			case 'Z':
			case '%':
				p++;
				continue;
			case '\0':
			default:
				return AGC_STATUS_FALSE;
			}
		}
		p++;
	}

	return apr_strftime(s, retsize, max, format, (apr_time_exp_t *) tm);
}

AGC_DECLARE(agc_status_t) agc_strftime_nocheck(char *s, agc_size_t *retsize, agc_size_t max, const char *format, agc_time_exp_t *tm)
{
	return apr_strftime(s, retsize, max, format, (apr_time_exp_t *) tm);
}

AGC_DECLARE(int) agc_snprintf(char *buf, agc_size_t len, const char *format, ...)
{
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = apr_vsnprintf(buf, len, format, ap);
	va_end(ap);
	return ret;
}

AGC_DECLARE(int) agc_vsnprintf(char *buf, agc_size_t len, const char *format, va_list ap)
{
	return apr_vsnprintf(buf, len, format, ap);
}

AGC_DECLARE(char *) agc_copy_string(char *dst, const char *src, agc_size_t dst_size)
{
	if (!dst)
		return NULL;
	if (!src) {
		*dst = '\0';
		return dst;
	}
	return apr_cpystrn(dst, src, dst_size);
}

/* thread read write lock functions */

AGC_DECLARE(agc_status_t) agc_thread_rwlock_create(agc_thread_rwlock_t ** rwlock, agc_memory_pool_t *pool)
{
	return apr_thread_rwlock_create(rwlock, pool);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_destroy(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_destroy(rwlock);
}

AGC_DECLARE(agc_memory_pool_t *) agc_thread_rwlock_pool_get(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_pool_get(rwlock);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_rdlock(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_rdlock(rwlock);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_tryrdlock(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_tryrdlock(rwlock);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_wrlock(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_wrlock(rwlock);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_trywrlock(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_trywrlock(rwlock);
}

AGC_DECLARE(agc_status_t) agc_thread_rwlock_trywrlock_timeout(agc_thread_rwlock_t *rwlock, int timeout)
{
	int sanity = timeout * 2;

	while (sanity) {
		if (agc_thread_rwlock_trywrlock(rwlock) == AGC_STATUS_SUCCESS) {
			return AGC_STATUS_SUCCESS;
		}
		sanity--;
		agc_yield(500000);
	}

	return AGC_STATUS_FALSE;
}


AGC_DECLARE(agc_status_t) agc_thread_rwlock_unlock(agc_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_unlock(rwlock);
}


/* thread mutex functions */

AGC_DECLARE(agc_status_t) agc_mutex_init(agc_mutex_t ** lock, unsigned int flags, agc_memory_pool_t *pool)
{
	return apr_thread_mutex_create(lock, flags, pool);
}

AGC_DECLARE(agc_status_t) agc_mutex_destroy(agc_mutex_t *lock)
{
	return apr_thread_mutex_destroy(lock);
}

AGC_DECLARE(agc_status_t) agc_mutex_lock(agc_mutex_t *lock)
{
	return apr_thread_mutex_lock(lock);
}

AGC_DECLARE(agc_status_t) agc_mutex_unlock(agc_mutex_t *lock)
{
	return apr_thread_mutex_unlock(lock);
}

AGC_DECLARE(agc_status_t) agc_mutex_trylock(agc_mutex_t *lock)
{
	return apr_thread_mutex_trylock(lock);
}

/* time function stubs */

AGC_DECLARE(agc_time_t) agc_time_now(void)
{
#if defined(HAVE_CLOCK_GETTIME) && defined(AGC_USE_CLOCK_FUNCS)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * APR_USEC_PER_SEC + (ts.tv_nsec / 1000);
#else
	return (agc_time_t) apr_time_now();
#endif
}

AGC_DECLARE(agc_status_t) agc_time_exp_gmt_get(agc_time_t *result, agc_time_exp_t *input)
{
	return apr_time_exp_gmt_get((apr_time_t *) result, (apr_time_exp_t *) input);
}

AGC_DECLARE(agc_status_t) agc_time_exp_get(agc_time_t *result, agc_time_exp_t *input)
{
	return apr_time_exp_get((apr_time_t *) result, (apr_time_exp_t *) input);
}

AGC_DECLARE(agc_status_t) agc_time_exp_lt(agc_time_exp_t *result, agc_time_t input)
{
	return apr_time_exp_lt((apr_time_exp_t *) result, input);
}

AGC_DECLARE(agc_status_t) agc_time_exp_tz(agc_time_exp_t *result, agc_time_t input, agc_int32_t offs)
{
	return apr_time_exp_tz((apr_time_exp_t *) result, input, (apr_int32_t) offs);
}

AGC_DECLARE(agc_status_t) agc_time_exp_gmt(agc_time_exp_t *result, agc_time_t input)
{
	return apr_time_exp_gmt((apr_time_exp_t *) result, input);
}

AGC_DECLARE(agc_status_t) agc_rfc822_date(char *date_str, agc_time_t t)
{
	return apr_rfc822_date(date_str, t);
}

AGC_DECLARE(agc_time_t) agc_time_make(agc_time_t sec, int32_t usec)
{
	return ((agc_time_t) (sec) * APR_USEC_PER_SEC + (agc_time_t) (usec));
}

/* Thread condition locks */

AGC_DECLARE(agc_status_t) agc_thread_cond_create(agc_thread_cond_t ** cond, agc_memory_pool_t *pool)
{
	return apr_thread_cond_create(cond, pool);
}

AGC_DECLARE(agc_status_t) agc_thread_cond_wait(agc_thread_cond_t *cond, agc_mutex_t *mutex)
{
	return apr_thread_cond_wait(cond, mutex);
}

AGC_DECLARE(agc_status_t) agc_thread_cond_timedwait(agc_thread_cond_t *cond, agc_mutex_t *mutex, agc_interval_time_t timeout)
{
	apr_status_t st = apr_thread_cond_timedwait(cond, mutex, timeout);

	if (st == APR_TIMEUP) {
		st = AGC_STATUS_TIMEOUT;
	}

	return st;
}

AGC_DECLARE(agc_status_t) agc_thread_cond_signal(agc_thread_cond_t *cond)
{
	return apr_thread_cond_signal(cond);
}

AGC_DECLARE(agc_status_t) agc_thread_cond_broadcast(agc_thread_cond_t *cond)
{
	return apr_thread_cond_broadcast(cond);
}

AGC_DECLARE(agc_status_t) agc_thread_cond_destroy(agc_thread_cond_t *cond)
{
	return apr_thread_cond_destroy(cond);
}

/* file i/o stubs */

AGC_DECLARE(agc_status_t) agc_file_open(agc_file_t ** newf, const char *fname, int32_t flag, agc_fileperms_t perm,
												 agc_memory_pool_t *pool)
{
	return apr_file_open(newf, fname, flag, perm, pool);
}

AGC_DECLARE(agc_status_t) agc_file_seek(agc_file_t *thefile, agc_seek_where_t where, int64_t *offset)
{
	apr_status_t rv;
	apr_off_t off = (apr_off_t) (*offset);
	rv = apr_file_seek(thefile, where, &off);
	*offset = (int64_t) off;
	return rv;
}

AGC_DECLARE(agc_status_t) agc_file_copy(const char *from_path, const char *to_path, agc_fileperms_t perms, agc_memory_pool_t *pool)
{
	return apr_file_copy(from_path, to_path, perms, pool);
}


AGC_DECLARE(agc_status_t) agc_file_close(agc_file_t *thefile)
{
	return apr_file_close(thefile);
}

AGC_DECLARE(agc_status_t) agc_file_trunc(agc_file_t *thefile, int64_t offset)
{
	return apr_file_trunc(thefile, offset);
}

AGC_DECLARE(agc_status_t) agc_file_lock(agc_file_t *thefile, int type)
{
	return apr_file_lock(thefile, type);
}

AGC_DECLARE(agc_status_t) agc_file_rename(const char *from_path, const char *to_path, agc_memory_pool_t *pool)
{
	return apr_file_rename(from_path, to_path, pool);
}

AGC_DECLARE(agc_status_t) agc_file_remove(const char *path, agc_memory_pool_t *pool)
{
	return apr_file_remove(path, pool);
}

AGC_DECLARE(agc_status_t) agc_file_read(agc_file_t *thefile, void *buf, agc_size_t *nbytes)
{
	return apr_file_read(thefile, buf, nbytes);
}

AGC_DECLARE(agc_status_t) agc_file_write(agc_file_t *thefile, const void *buf, agc_size_t *nbytes)
{
	return apr_file_write(thefile, buf, nbytes);
}

AGC_DECLARE(int) agc_file_printf(agc_file_t *thefile, const char *format, ...)
{
	va_list ap;
	int ret;
	char *data;

	va_start(ap, format);

	if ((ret = agc_vasprintf(&data, format, ap)) != -1) {
		agc_size_t bytes = strlen(data);
		agc_file_write(thefile, data, &bytes);
		free(data);
	}

	va_end(ap);

	return ret;
}

AGC_DECLARE(agc_status_t) agc_file_mktemp(agc_file_t ** thefile, char *templ, int32_t flags, agc_memory_pool_t *pool)
{
	return apr_file_mktemp(thefile, templ, flags, pool);
}

AGC_DECLARE(agc_size_t) agc_file_get_size(agc_file_t *thefile)
{
	struct apr_finfo_t finfo;
	return apr_file_info_get(&finfo, APR_FINFO_SIZE, thefile) == AGC_STATUS_SUCCESS ? (agc_size_t) finfo.size : 0;
}

AGC_DECLARE(agc_status_t) agc_directory_exists(const char *dirname, agc_memory_pool_t *pool)
{
	apr_dir_t *dir_handle;
	agc_memory_pool_t *our_pool = NULL;
	agc_status_t status;

	if (!pool) {
		agc_core_new_memory_pool(&our_pool);
		pool = our_pool;
	}

	if ((status = apr_dir_open(&dir_handle, dirname, pool)) == APR_SUCCESS) {
		apr_dir_close(dir_handle);
	}

	if (our_pool) {
		agc_core_destroy_memory_pool(&our_pool);
	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_file_exists(const char *filename, agc_memory_pool_t *pool)
{
	int32_t wanted = APR_FINFO_TYPE;
	agc_memory_pool_t *our_pool = NULL;
	agc_status_t status = AGC_STATUS_FALSE;
	apr_finfo_t info = { 0 };

	if (zstr(filename)) {
		return status;
	}

	if (!pool) {
		agc_core_new_memory_pool(&our_pool);
	}

	apr_stat(&info, filename, wanted, pool ? pool : our_pool);
	if (info.filetype != APR_NOFILE) {
		status = AGC_STATUS_SUCCESS;
	}

	if (our_pool) {
		agc_core_destroy_memory_pool(&our_pool);
	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_dir_make(const char *path, agc_fileperms_t perm, agc_memory_pool_t *pool)
{
	return apr_dir_make(path, perm, pool);
}

AGC_DECLARE(agc_status_t) agc_dir_make_recursive(const char *path, agc_fileperms_t perm, agc_memory_pool_t *pool)
{
	return apr_dir_make_recursive(path, perm, pool);
}

AGC_DECLARE(agc_status_t) agc_dir_open(agc_dir_t ** new_dir, const char *dirname, agc_memory_pool_t *pool)
{
	agc_status_t status;
	agc_dir_t *dir = malloc(sizeof(*dir));

	if (!dir) {
		*new_dir = NULL;
		return AGC_STATUS_FALSE;
	}

	memset(dir, 0, sizeof(*dir));
	if ((status = apr_dir_open(&(dir->dir_handle), dirname, pool)) == APR_SUCCESS) {
		*new_dir = dir;
	} else {
		free(dir);
		*new_dir = NULL;
	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_dir_close(agc_dir_t *thedir)
{
	agc_status_t status = apr_dir_close(thedir->dir_handle);

	free(thedir);
	return status;
}

AGC_DECLARE(uint32_t) agc_dir_count(agc_dir_t *thedir)
{
	const char *name;
	apr_int32_t finfo_flags = APR_FINFO_DIRENT | APR_FINFO_TYPE | APR_FINFO_NAME;
	uint32_t count = 0;

	apr_dir_rewind(thedir->dir_handle);

	while (apr_dir_read(&(thedir->finfo), finfo_flags, thedir->dir_handle) == AGC_STATUS_SUCCESS) {

		if (thedir->finfo.filetype != APR_REG && thedir->finfo.filetype != APR_LNK) {
			continue;
		}

		if (!(name = thedir->finfo.fname)) {
			name = thedir->finfo.name;
		}

		if (name) {
			count++;
		}
	}

	apr_dir_rewind(thedir->dir_handle);

	return count;
}

AGC_DECLARE(const char *) agc_dir_next_file(agc_dir_t *thedir, char *buf, agc_size_t len)
{
	const char *fname = NULL;
	apr_int32_t finfo_flags = APR_FINFO_DIRENT | APR_FINFO_TYPE | APR_FINFO_NAME;
	const char *name;

	while (apr_dir_read(&(thedir->finfo), finfo_flags, thedir->dir_handle) == AGC_STATUS_SUCCESS) {

		if (thedir->finfo.filetype != APR_REG && thedir->finfo.filetype != APR_LNK) {
			continue;
		}

		if (!(name = thedir->finfo.fname)) {
			name = thedir->finfo.name;
		}

		if (name) {
			agc_copy_string(buf, name, len);
			fname = buf;
			break;
		} else {
			continue;
		}
	}
	return fname;
}

/* thread stubs */
AGC_DECLARE(agc_status_t) agc_threadattr_create(agc_threadattr_t ** new_attr, agc_memory_pool_t *pool)
{
	agc_status_t status;

	if ((status = apr_threadattr_create(new_attr, pool)) == AGC_STATUS_SUCCESS) {

		(*new_attr)->priority = AGC_PRI_LOW;

	}

	return status;
}

AGC_DECLARE(agc_status_t) agc_threadattr_detach_set(agc_threadattr_t *attr, int32_t on)
{
	return apr_threadattr_detach_set(attr, on);
}

AGC_DECLARE(agc_status_t) agc_threadattr_stacksize_set(agc_threadattr_t *attr, agc_size_t stacksize)
{
	return apr_threadattr_stacksize_set(attr, stacksize);
}

AGC_DECLARE(agc_status_t) agc_threadattr_priority_set(agc_threadattr_t *attr, agc_thread_priority_t priority)
{

	attr->priority = priority;

	return AGC_STATUS_SUCCESS;
}

static char TT_KEY[] = "1";

AGC_DECLARE(agc_status_t) agc_thread_create(agc_thread_t ** new_thread, agc_threadattr_t *attr,
													 agc_thread_start_t func, void *data, agc_memory_pool_t *cont)
{
	agc_status_t status;
	agc_core_memory_pool_set_data(cont, "_in_thread", TT_KEY);
	status = apr_thread_create(new_thread, attr, func, data, cont);
	return status;
}

AGC_DECLARE(agc_interval_time_t) agc_interval_time_from_timeval(struct timeval *tvp)
{
	return ((agc_interval_time_t)tvp->tv_sec * 1000000) + tvp->tv_usec / 1000;
}

/* socket stubs */
AGC_DECLARE(agc_status_t) agc_os_sock_get(agc_os_socket_t *thesock, agc_socket_t *sock)
{
	return apr_os_sock_get(thesock, sock);
}

AGC_DECLARE(agc_status_t) agc_os_sock_put(agc_socket_t **sock, agc_os_socket_t *thesock, agc_memory_pool_t *pool)
{
	return apr_os_sock_put(sock, thesock, pool);
}

AGC_DECLARE(agc_status_t) agc_socket_addr_get(agc_sockaddr_t ** sa, agc_bool_t remote, agc_socket_t *sock)
{
	return apr_socket_addr_get(sa, (apr_interface_e) remote, sock);
}

AGC_DECLARE(agc_status_t) agc_socket_create(agc_socket_t ** new_sock, int family, int type, int protocol, agc_memory_pool_t *pool)
{
	return apr_socket_create(new_sock, family, type, protocol, pool);
}

AGC_DECLARE(agc_status_t) agc_socket_shutdown(agc_socket_t *sock, agc_shutdown_how_e how)
{
	return apr_socket_shutdown(sock, (apr_shutdown_how_e) how);
}

AGC_DECLARE(agc_status_t) agc_socket_close(agc_socket_t *sock)
{
	return apr_socket_close(sock);
}

AGC_DECLARE(agc_status_t) agc_socket_bind(agc_socket_t *sock, agc_sockaddr_t *sa)
{
	return apr_socket_bind(sock, sa);
}

AGC_DECLARE(agc_status_t) agc_socket_listen(agc_socket_t *sock, int32_t backlog)
{
	return apr_socket_listen(sock, backlog);
}

AGC_DECLARE(agc_status_t) agc_socket_accept(agc_socket_t ** new_sock, agc_socket_t *sock, agc_memory_pool_t *pool)
{
	return apr_socket_accept(new_sock, sock, pool);
}

AGC_DECLARE(agc_status_t) agc_socket_connect(agc_socket_t *sock, agc_sockaddr_t *sa)
{
	return apr_socket_connect(sock, sa);
}

AGC_DECLARE(agc_status_t) agc_socket_send(agc_socket_t *sock, const char *buf, agc_size_t *len)
{
	int status = AGC_STATUS_SUCCESS;
	agc_size_t req = *len, wrote = 0, need = *len;
	int to_count = 0;

	while ((wrote < req && status == AGC_STATUS_SUCCESS) || (need == 0 && status == AGC_STATUS_BREAK) || status == 730035 || status == 35) {
		need = req - wrote;
		status = apr_socket_send(sock, buf + wrote, &need);
		if (status == AGC_STATUS_BREAK || status == 730035 || status == 35) {
			if (++to_count > 60000) {
				status = AGC_STATUS_FALSE;
				break;
			}
			agc_yield(10000);
		} else {
			to_count = 0;
		}
		wrote += need;
	}

	*len = wrote;
	return (agc_status_t)status;
}

AGC_DECLARE(agc_status_t) agc_socket_send_nonblock(agc_socket_t *sock, const char *buf, agc_size_t *len)
{
	if (!sock || !buf || !len) {
		return AGC_STATUS_GENERR;
	}
	
	return apr_socket_send(sock, buf, len);
}

AGC_DECLARE(agc_status_t) agc_socket_sendto(agc_socket_t *sock, agc_sockaddr_t *where, int32_t flags, const char *buf,
													 agc_size_t *len)
{
	if (!where || !buf || !len || !*len) {
		return AGC_STATUS_GENERR;
	}
	return apr_socket_sendto(sock, where, flags, buf, len);
}

AGC_DECLARE(agc_status_t) agc_socket_recv(agc_socket_t *sock, char *buf, agc_size_t *len)
{
	int r;

	r = apr_socket_recv(sock, buf, len);

	if (r == 35 || r == 730035) {
		r = AGC_STATUS_BREAK;
	}

	return (agc_status_t)r;
}

AGC_DECLARE(agc_status_t) agc_sockaddr_create(agc_sockaddr_t **sa, agc_memory_pool_t *pool)
{
	agc_sockaddr_t *new_sa;
	unsigned short family = APR_INET;

	new_sa = apr_pcalloc(pool, sizeof(apr_sockaddr_t));
	agc_assert(new_sa);
	new_sa->pool = pool;
	memset(new_sa, 0, sizeof(*new_sa));

    new_sa->family = family;
    new_sa->sa.sin.sin_family = family;

    new_sa->salen = sizeof(struct sockaddr_in);
    new_sa->addr_str_len = 16;
    new_sa->ipaddr_ptr = &(new_sa->sa.sin.sin_addr);
    new_sa->ipaddr_len = sizeof(struct in_addr);

	*sa = new_sa;
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_sockaddr_info_get(agc_sockaddr_t ** sa, const char *hostname, int32_t family,
														 agc_port_t port, int32_t flags, agc_memory_pool_t *pool)
{
	return apr_sockaddr_info_get(sa, hostname, family, port, flags, pool);
}

AGC_DECLARE(agc_status_t) agc_socket_opt_set(agc_socket_t *sock, int32_t opt, int32_t on)
{
	if (opt == AGC_SO_TCP_KEEPIDLE) {
		int r = -10;
		
#if defined(TCP_KEEPIDLE)
		r = setsockopt(jsock->client_socket, SOL_TCP, TCP_KEEPIDLE, (void *)&on, sizeof(on));
#endif
		if (r == -10) {
			return AGC_STATUS_NOTIMPL;
		}

	
		return r ? AGC_STATUS_FALSE : AGC_STATUS_SUCCESS;
	}

	if (opt == AGC_SO_TCP_KEEPINTVL) {
		int r = -10;

#if defined(TCP_KEEPINTVL)
		r = setsockopt(jsock->client_socket, SOL_TCP, TCP_KEEPINTVL, (void *)&on, sizeof(on));
#endif

		if (r == -10) {
			return AGC_STATUS_NOTIMPL;
		}

		return r ? AGC_STATUS_FALSE : AGC_STATUS_SUCCESS;
	}

	return apr_socket_opt_set(sock, opt, on);
}

AGC_DECLARE(agc_status_t) agc_socket_timeout_set(agc_socket_t *sock, agc_interval_time_t t)
{
	return apr_socket_timeout_set(sock, t);
}

AGC_DECLARE(agc_status_t) agc_sockaddr_ip_get(char **addr, agc_sockaddr_t *sa)
{
	return apr_sockaddr_ip_get(addr, sa);
}

AGC_DECLARE(int) agc_sockaddr_equal(const agc_sockaddr_t *sa1, const agc_sockaddr_t *sa2)
{
	return apr_sockaddr_equal(sa1, sa2);
}

AGC_DECLARE(agc_status_t) agc_mcast_join(agc_socket_t *sock, agc_sockaddr_t *join, agc_sockaddr_t *iface, agc_sockaddr_t *source)
{
	return apr_mcast_join(sock, join, iface, source);
}

AGC_DECLARE(agc_status_t) agc_mcast_hops(agc_socket_t *sock, uint8_t ttl)
{
	return apr_mcast_hops(sock, ttl);
}

AGC_DECLARE(agc_status_t) agc_mcast_loopback(agc_socket_t *sock, uint8_t opt)
{
	return apr_mcast_loopback(sock, opt);
}

AGC_DECLARE(agc_status_t) agc_mcast_interface(agc_socket_t *sock, agc_sockaddr_t *iface)
{
	return apr_mcast_interface(sock, iface);
}
												 
/* socket functions */

AGC_DECLARE(const char *) agc_get_addr(char *buf, agc_size_t len, agc_sockaddr_t *in)
{
	if (!in) {
		return AGC_BLANK_STRING;
	}

	memset(buf, 0, len);

	if (in->family == AF_INET) {
		get_addr(buf, len, (struct sockaddr *) &in->sa, in->salen);
		return buf;
	}

	get_addr6(buf, len, (struct sockaddr_in6 *) &in->sa, in->salen);
	return buf;
}

AGC_DECLARE(uint16_t) agc_sockaddr_get_port(agc_sockaddr_t *sa)
{
	return sa->port;
}

AGC_DECLARE(int32_t) agc_sockaddr_get_family(agc_sockaddr_t *sa)
{
	return sa->family;
}

AGC_DECLARE(agc_status_t) agc_getnameinfo(char **hostname, agc_sockaddr_t *sa, int32_t flags)
{
	return apr_getnameinfo(hostname, sa, flags);
}

AGC_DECLARE(agc_status_t) agc_socket_atmark(agc_socket_t *sock, int *atmark)
{
	return apr_socket_atmark(sock, atmark);
}

AGC_DECLARE(agc_status_t) agc_socket_recvfrom(agc_sockaddr_t *from, agc_socket_t *sock, int32_t flags, char *buf, size_t *len)
{
	int r = AGC_STATUS_GENERR;

	if (from && sock && (r = apr_socket_recvfrom(from, sock, flags, buf, len)) == APR_SUCCESS) {
		from->port = ntohs(from->sa.sin.sin_port);
		/* from->ipaddr_ptr = &(from->sa.sin.sin_addr);
		 * from->ipaddr_ptr = inet_ntoa(from->sa.sin.sin_addr);
		 */
	}

	if (r == 35 || r == 730035) {
		r = AGC_STATUS_BREAK;
	}

	return (agc_status_t)r;
}

/* poll stubs */

AGC_DECLARE(agc_status_t) agc_pollset_create(agc_pollset_t ** pollset, uint32_t size, agc_memory_pool_t *pool, uint32_t flags)
{
	return apr_pollset_create(pollset, size, pool, flags);
}

AGC_DECLARE(agc_status_t) agc_pollset_add(agc_pollset_t *pollset, const agc_pollfd_t *descriptor)
{
	if (!pollset || !descriptor) {
		return AGC_STATUS_FALSE;
	}

	return apr_pollset_add((apr_pollset_t *) pollset, (const apr_pollfd_t *) descriptor);
}

AGC_DECLARE(agc_status_t) agc_pollset_remove(agc_pollset_t *pollset, const agc_pollfd_t *descriptor)
{
	if (!pollset || !descriptor) {
		return AGC_STATUS_FALSE;
	}	
	
	return apr_pollset_remove((apr_pollset_t *) pollset, (const apr_pollfd_t *) descriptor);
}

AGC_DECLARE(agc_status_t) agc_socket_create_pollfd(agc_pollfd_t **pollfd, agc_socket_t *sock, int16_t flags, void *client_data, agc_memory_pool_t *pool)
{
	if (!pollfd || !sock) {
		return AGC_STATUS_FALSE;
	}
	
	if ((*pollfd = (agc_pollfd_t*)apr_palloc(pool, sizeof(agc_pollfd_t))) == 0) {
		return AGC_STATUS_MEMERR;
	}
	
	memset(*pollfd, 0, sizeof(agc_pollfd_t));

	(*pollfd)->desc_type = (agc_pollset_type_t) APR_POLL_SOCKET;
	(*pollfd)->reqevents = flags;
	(*pollfd)->desc.s = sock;
	(*pollfd)->client_data = client_data;
	
	return AGC_STATUS_SUCCESS;
}


AGC_DECLARE(agc_status_t) agc_pollset_poll(agc_pollset_t *pollset, agc_interval_time_t timeout, int32_t *num, const agc_pollfd_t **descriptors)
{
	apr_status_t st = AGC_STATUS_FALSE;
	
	if (pollset) {
		st = apr_pollset_poll((apr_pollset_t *) pollset, timeout, num, (const apr_pollfd_t **) descriptors);
		
		if (st == APR_TIMEUP) {
			st = agc_status_tIMEOUT;
		}
	}
	
	return st;
}

AGC_DECLARE(agc_status_t) agc_poll(agc_pollfd_t *aprset, int32_t numsock, int32_t *nsds, agc_interval_time_t timeout)
{
	apr_status_t st = AGC_STATUS_FALSE;

	if (aprset) {
		st = apr_poll((apr_pollfd_t *) aprset, numsock, nsds, timeout);

		if (st == APR_TIMEUP) {
			st = agc_status_tIMEOUT;
		}
	}

	return st;
}

AGC_DECLARE(agc_status_t) agc_socket_create_pollset(agc_pollfd_t ** poll, agc_socket_t *sock, int16_t flags, agc_memory_pool_t *pool)
{
	agc_pollset_t *pollset;

	if (agc_pollset_create(&pollset, 1, pool, 0) != AGC_STATUS_SUCCESS) {
		return AGC_STATUS_GENERR;
	}

	if (agc_socket_create_pollfd(poll, sock, flags, sock, pool) != AGC_STATUS_SUCCESS) {
		return AGC_STATUS_GENERR;
	}

	if (agc_pollset_add(pollset, *poll) != AGC_STATUS_SUCCESS) {
		return AGC_STATUS_GENERR;
	}

	return AGC_STATUS_SUCCESS;
}

/* apr-util stubs */

/* UUID Handling (apr-util) */

AGC_DECLARE(void) agc_uuid_format(char *buffer, const agc_uuid_t *uuid)
{
    apr_uuid_format(buffer, (const apr_uuid_t *) uuid);
}

AGC_DECLARE(void) agc_uuid_get(agc_uuid_t *uuid)
{
	agc_mutex_lock(runtime.uuid_mutex);
	apr_uuid_get((apr_uuid_t *) uuid);
	agc_mutex_unlock(runtime.uuid_mutex);
}

AGC_DECLARE(agc_status_t) agc_uuid_parse(agc_uuid_t *uuid, const char *uuid_str)
{
	return apr_uuid_parse((apr_uuid_t *) uuid, uuid_str);
}

AGC_DECLARE(agc_status_t) agc_md5(unsigned char digest[AGC_MD5_DIGESTSIZE], const void *input, agc_size_t inputLen)
{
	return apr_md5(digest, input, inputLen);
}

AGC_DECLARE(agc_status_t) agc_md5_string(char digest_str[AGC_MD5_DIGEST_STRING_SIZE], const void *input, agc_size_t inputLen)
{
	unsigned char digest[AGC_MD5_DIGESTSIZE];
	apr_status_t status = apr_md5(digest, input, inputLen);
	int x;

	digest_str[AGC_MD5_DIGEST_STRING_SIZE - 1] = '\0';

	for (x = 0; x < AGC_MD5_DIGESTSIZE; x++) {
		agc_snprintf(digest_str + (x * 2), 3, "%02x", digest[x]);
	}

	return status;
}

/* FIFO queues (apr-util) */

AGC_DECLARE(agc_status_t) agc_queue_create(agc_queue_t ** queue, unsigned int queue_capacity, agc_memory_pool_t *pool)
{
	return apr_queue_create(queue, queue_capacity, pool);
}

AGC_DECLARE(unsigned int) agc_queue_size(agc_queue_t *queue)
{
	return apr_queue_size(queue);
}

AGC_DECLARE(agc_status_t) agc_queue_pop(agc_queue_t *queue, void **data)
{
	return apr_queue_pop(queue, data);
}

AGC_DECLARE(agc_status_t) agc_queue_pop_timeout(agc_queue_t *queue, void **data, agc_interval_time_t timeout)
{
	return apr_queue_pop_timeout(queue, data, timeout);
}

AGC_DECLARE(agc_status_t) agc_queue_push(agc_queue_t *queue, void *data)
{
	apr_status_t s;

	do {
		s = apr_queue_push(queue, data);
	} while (s == APR_EINTR);

	return s;
}

AGC_DECLARE(agc_status_t) agc_queue_trypop(agc_queue_t *queue, void **data)
{
	return apr_queue_trypop(queue, data);
}

AGC_DECLARE(agc_status_t) agc_queue_interrupt_all(agc_queue_t *queue)
{
	return apr_queue_interrupt_all(queue);
}

AGC_DECLARE(agc_status_t) agc_queue_term(agc_queue_t *queue)
{
	return apr_queue_term(queue);
}

AGC_DECLARE(agc_status_t) agc_queue_trypush(agc_queue_t *queue, void *data)
{
	apr_status_t s;

	do {
		s = apr_queue_trypush(queue, data);
	} while (s == APR_EINTR);

	return s;
}

AGC_DECLARE(int) agc_vasprintf(char **ret, const char *fmt, va_list ap)
{
#ifdef HAVE_VASPRINTF
	return vasprintf(ret, fmt, ap);
#else
	char *buf;
	int len;
	size_t buflen;
	va_list ap2;
	char *tmp = NULL;

#ifdef _MSC_VER
#if _MSC_VER >= 1500
	/* hack for incorrect assumption in msvc header files for code analysis */
	__analysis_assume(tmp);
#endif
	ap2 = ap;
#else
	va_copy(ap2, ap);
#endif

	len = vsnprintf(tmp, 0, fmt, ap2);

	if (len > 0 && (buf = malloc((buflen = (size_t) (len + 1)))) != NULL) {
		len = vsnprintf(buf, buflen, fmt, ap);
		*ret = buf;
	} else {
		*ret = NULL;
		len = -1;
	}

	va_end(ap2);
	return len;
#endif
}

AGC_DECLARE(agc_status_t) agc_match_glob(const char *pattern, agc_array_header_t ** result, agc_memory_pool_t *pool)
{
	return apr_match_glob(pattern, (apr_array_header_t **) result, pool);
}

/**
 * Create an anonymous pipe.
 * @param in The file descriptor to use as input to the pipe.
 * @param out The file descriptor to use as output from the pipe.
 * @param pool The pool to operate on.
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_create(agc_file_t ** in, agc_file_t ** out, agc_memory_pool_t *pool)
{
	return apr_file_pipe_create((apr_file_t **) in, (apr_file_t **) out, pool);
}

/**
 * Get the timeout value for a pipe or manipulate the blocking state.
 * @param thepipe The pipe we are getting a timeout for.
 * @param timeout The current timeout value in microseconds. 
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_timeout_get(agc_file_t *thepipe, agc_interval_time_t *timeout)
{
	return apr_file_pipe_timeout_get((apr_file_t *) thepipe, (apr_interval_time_t *) timeout);
}

/**
 * Set the timeout value for a pipe or manipulate the blocking state.
 * @param thepipe The pipe we are setting a timeout on.
 * @param timeout The timeout value in microseconds.  Values < 0 mean wait 
 *        forever, 0 means do not wait at all.
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_timeout_set(agc_file_t *thepipe, agc_interval_time_t timeout)
{
	return apr_file_pipe_timeout_set((apr_file_t *) thepipe, (apr_interval_time_t) timeout);
}


/**
 * stop the current thread
 * @param thd The thread to stop
 * @param retval The return value to pass back to any thread that cares
 */
AGC_DECLARE(agc_status_t) agc_thread_exit(agc_thread_t *thd, agc_status_t retval)
{
	return apr_thread_exit((apr_thread_t *) thd, retval);
}

/**
 * block until the desired thread stops executing.
 * @param retval The return value from the dead thread.
 * @param thd The thread to join
 */
AGC_DECLARE(agc_status_t) agc_thread_join(agc_status_t *retval, agc_thread_t *thd)
{
	return apr_thread_join((apr_status_t *) retval, (apr_thread_t *) thd);
}


AGC_DECLARE(agc_status_t) agc_atomic_init(agc_memory_pool_t *pool)
{
	return apr_atomic_init((apr_pool_t *) pool);
}

AGC_DECLARE(uint32_t) agc_atomic_read(volatile agc_atomic_t *mem)
{
#ifdef apr_atomic_t
	return apr_atomic_read((apr_atomic_t *)mem);
#else
	return apr_atomic_read32((apr_uint32_t *)mem);
#endif
}

AGC_DECLARE(void) agc_atomic_set(volatile agc_atomic_t *mem, uint32_t val)
{
#ifdef apr_atomic_t
	apr_atomic_set((apr_atomic_t *)mem, val);
#else
	apr_atomic_set32((apr_uint32_t *)mem, val);
#endif
}

AGC_DECLARE(void) agc_atomic_add(volatile agc_atomic_t *mem, uint32_t val)
{
#ifdef apr_atomic_t
	apr_atomic_add((apr_atomic_t *)mem, val);
#else
	apr_atomic_add32((apr_uint32_t *)mem, val);
#endif
}

AGC_DECLARE(void) agc_atomic_inc(volatile agc_atomic_t *mem)
{
#ifdef apr_atomic_t
	apr_atomic_inc((apr_atomic_t *)mem);
#else
	apr_atomic_inc32((apr_uint32_t *)mem);
#endif
}

AGC_DECLARE(int) agc_atomic_dec(volatile agc_atomic_t *mem)
{
#ifdef apr_atomic_t
	return apr_atomic_dec((apr_atomic_t *)mem);
#else
	return apr_atomic_dec32((apr_uint32_t *)mem);
#endif
}

AGC_DECLARE(char *) agc_strerror(agc_status_t statcode, char *buf, agc_size_t bufsize)
{
       return apr_strerror(statcode, buf, bufsize);
}


