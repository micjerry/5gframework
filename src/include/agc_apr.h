#ifndef AGC_APR_H
#define AGC_APR_H

AGC_BEGIN_EXTERN_C

#include <pthread.h>
typedef pthread_t agc_thread_id_t;

AGC_DECLARE(int) agc_status_is_timeup(int status);

AGC_DECLARE(agc_thread_id_t) agc_thread_self(void);

AGC_DECLARE(int) agc_thread_equal(agc_thread_id_t tid1, agc_thread_id_t tid2);

AGC_DECLARE(void) agc_pool_clear(agc_memory_pool_t *pool);

AGC_DECLARE(int) agc_snprintf(char *buf, agc_size_t len, const char *format, ...);

AGC_DECLARE(int) agc_vasprintf(char **ret, const char *fmt, va_list ap);

AGC_DECLARE(int) agc_vsnprintf(char *buf, agc_size_t len, const char *format, va_list ap);

AGC_DECLARE(char *) agc_copy_string(char *dst, const char *src, agc_size_t dst_size);

AGC_DECLARE(unsigned int) agc_hashfunc_default(const char *key, agc_ssize_t *klen);



AGC_DECLARE(agc_time_t) agc_time_make(agc_time_t sec, int32_t usec);

/**
 * @return the current time
 */
AGC_DECLARE(agc_time_t) agc_time_now(void);

/**
 * Convert time value from human readable format to a numeric apr_time_t that
 * always represents GMT
 * @param result the resulting imploded time
 * @param input the input exploded time
 */
AGC_DECLARE(agc_status_t) agc_time_exp_gmt_get(agc_time_t *result, agc_time_exp_t *input);

/**
 * formats the exploded time according to the format specified
 * @param s string to write to
 * @param retsize The length of the returned string
 * @param max The maximum length of the string
 * @param format The format for the time string
 * @param tm The time to convert
 */
AGC_DECLARE(agc_status_t) agc_strftime(char *s, agc_size_t *retsize, agc_size_t max, const char *format, agc_time_exp_t *tm);

/**
 * formats the exploded time according to the format specified (does not validate format string)
 * @param s string to write to
 * @param retsize The length of the returned string
 * @param max The maximum length of the string
 * @param format The format for the time string
 * @param tm The time to convert
 */
AGC_DECLARE(agc_status_t) agc_strftime_nocheck(char *s, agc_size_t *retsize, agc_size_t max, const char *format, agc_time_exp_t *tm);

/**
 * agc_rfc822_date formats dates in the RFC822
 * format in an efficient manner.  It is a fixed length
 * format which requires the indicated amount of storage,
 * including the trailing NUL terminator.
 * @param date_str String to write to.
 * @param t the time to convert 
 */
AGC_DECLARE(agc_status_t) agc_rfc822_date(char *date_str, agc_time_t t);

/**
 * convert a time to its human readable components in GMT timezone
 * @param result the exploded time
 * @param input the time to explode
 */
AGC_DECLARE(agc_status_t) agc_time_exp_gmt(agc_time_exp_t *result, agc_time_t input);

/**
 * Convert time value from human readable format to a numeric apr_time_t 
 * e.g. elapsed usec since epoch
 * @param result the resulting imploded time
 * @param input the input exploded time
 */
AGC_DECLARE(agc_status_t) agc_time_exp_get(agc_time_t *result, agc_time_exp_t *input);

/**
 * convert a time to its human readable components in local timezone
 * @param result the exploded time
 * @param input the time to explode
 */
AGC_DECLARE(agc_status_t) agc_time_exp_lt(agc_time_exp_t *result, agc_time_t input);

/**
 * convert a time to its human readable components in a specific timezone with offset
 * @param result the exploded time
 * @param input the time to explode
 */
AGC_DECLARE(agc_status_t) agc_time_exp_tz(agc_time_exp_t *result, agc_time_t input, agc_int32_t offs);

/**
 * Sleep for the specified number of micro-seconds.
 * @param t desired amount of time to sleep.
 * @warning May sleep for longer than the specified time. 
 */
AGC_DECLARE(void) agc_sleep(agc_interval_time_t t);
AGC_DECLARE(void) agc_micro_sleep(agc_interval_time_t t);


#define AGC_MUTEX_DEFAULT	0x0	/**< platform-optimal lock behavior */
#define AGC_MUTEX_NESTED	0x1	/**< enable nested (recursive) locks */
#define	AGC_MUTEX_UNNESTED	0x2	/**< disable nested locks */

/**
 * Create and initialize a mutex that can be used to synchronize threads.
 * @param lock the memory address where the newly created mutex will be
 *        stored.
 * @param flags Or'ed value of:
 * <PRE>
 *           AGC_MUTEX_DEFAULT   platform-optimal lock behavior.
 *           AGC_MUTEX_NESTED    enable nested (recursive) locks.
 *           AGC_MUTEX_UNNESTED  disable nested locks (non-recursive).
 * </PRE>
 * @param pool the pool from which to allocate the mutex.
 * @warning Be cautious in using AGC_THREAD_MUTEX_DEFAULT.  While this is the
 * most optimial mutex based on a given platform's performance charateristics,
 * it will behave as either a nested or an unnested lock.
 *
*/
AGC_DECLARE(agc_status_t) agc_mutex_init(agc_mutex_t ** lock, unsigned int flags, agc_memory_pool_t *pool);

/**
 * Destroy the mutex and free the memory associated with the lock.
 * @param lock the mutex to destroy.
 */
AGC_DECLARE(agc_status_t) agc_mutex_destroy(agc_mutex_t *lock);

/**
 * Acquire the lock for the given mutex. If the mutex is already locked,
 * the current thread will be put to sleep until the lock becomes available.
 * @param lock the mutex on which to acquire the lock.
 */
AGC_DECLARE(agc_status_t) agc_mutex_lock(agc_mutex_t *lock);

/**
 * Release the lock for the given mutex.
 * @param lock the mutex from which to release the lock.
 */
AGC_DECLARE(agc_status_t) agc_mutex_unlock(agc_mutex_t *lock);

/**
 * Attempt to acquire the lock for the given mutex. If the mutex has already
 * been acquired, the call returns immediately with APR_EBUSY. Note: it
 * is important that the APR_STATUS_IS_EBUSY(s) macro be used to determine
 * if the return value was APR_EBUSY, for portability reasons.
 * @param lock the mutex on which to attempt the lock acquiring.
 */
AGC_DECLARE(agc_status_t) agc_mutex_trylock(agc_mutex_t *lock);

/**
 * Some architectures require atomic operations internal structures to be
 * initialized before use.
 * @param pool The memory pool to use when initializing the structures.
 */
AGC_DECLARE(agc_status_t) agc_atomic_init(agc_memory_pool_t *pool);

/**
 * Uses an atomic operation to read the uint32 value at the location specified
 * by mem.
 * @param mem The location of memory which stores the value to read.
 */
AGC_DECLARE(uint32_t) agc_atomic_read(volatile agc_atomic_t *mem);

/**
 * Uses an atomic operation to set a uint32 value at a specified location of
 * memory.
 * @param mem The location of memory to set.
 * @param val The uint32 value to set at the memory location.
 */
AGC_DECLARE(void) agc_atomic_set(volatile agc_atomic_t *mem, uint32_t val);

/**
 * Uses an atomic operation to add the uint32 value to the value at the
 * specified location of memory.
 * @param mem The location of the value to add to.
 * @param val The uint32 value to add to the value at the memory location.
 */
AGC_DECLARE(void) agc_atomic_add(volatile agc_atomic_t *mem, uint32_t val);

/**
 * Uses an atomic operation to increment the value at the specified memroy
 * location.
 * @param mem The location of the value to increment.
 */
AGC_DECLARE(void) agc_atomic_inc(volatile agc_atomic_t *mem);

/**
 * Uses an atomic operation to decrement the value at the specified memroy
 * location.
 * @param mem The location of the value to decrement.
 */
AGC_DECLARE(int)  agc_atomic_dec(volatile agc_atomic_t *mem);


AGC_DECLARE(agc_status_t) agc_thread_rwlock_create(agc_thread_rwlock_t ** rwlock, agc_memory_pool_t *pool);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_destroy(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_memory_pool_t *) agc_thread_rwlock_pool_get(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_rdlock(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_tryrdlock(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_wrlock(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_trywrlock(agc_thread_rwlock_t *rwlock);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_trywrlock_timeout(agc_thread_rwlock_t *rwlock, int timeout);
AGC_DECLARE(agc_status_t) agc_thread_rwlock_unlock(agc_thread_rwlock_t *rwlock);



/**
 * Create and initialize a condition variable that can be used to signal
 * and schedule threads in a single process.
 * @param cond the memory address where the newly created condition variable
 *        will be stored.
 * @param pool the pool from which to allocate the mutex.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_create(agc_thread_cond_t ** cond, agc_memory_pool_t *pool);

/**
 * Put the active calling thread to sleep until signaled to wake up. Each
 * condition variable must be associated with a mutex, and that mutex must
 * be locked before  calling this function, or the behavior will be
 * undefined. As the calling thread is put to sleep, the given mutex
 * will be simultaneously released; and as this thread wakes up the lock
 * is again simultaneously acquired.
 * @param cond the condition variable on which to block.
 * @param mutex the mutex that must be locked upon entering this function,
 *        is released while the thread is asleep, and is again acquired before
 *        returning from this function.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_wait(agc_thread_cond_t *cond, agc_mutex_t *mutex);

/**
 * Put the active calling thread to sleep until signaled to wake up or
 * the timeout is reached. Each condition variable must be associated
 * with a mutex, and that mutex must be locked before calling this
 * function, or the behavior will be undefined. As the calling thread
 * is put to sleep, the given mutex will be simultaneously released;
 * and as this thread wakes up the lock is again simultaneously acquired.
 * @param cond the condition variable on which to block.
 * @param mutex the mutex that must be locked upon entering this function,
 *        is released while the thread is asleep, and is again acquired before
 *        returning from this function.
 * @param timeout The amount of time in microseconds to wait. This is 
 *        a maximum, not a minimum. If the condition is signaled, we 
 *        will wake up before this time, otherwise the error APR_TIMEUP
 *        is returned.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_timedwait(agc_thread_cond_t *cond, agc_mutex_t *mutex, agc_interval_time_t timeout);

/**
 * Signals a single thread, if one exists, that is blocking on the given
 * condition variable. That thread is then scheduled to wake up and acquire
 * the associated mutex. Although it is not required, if predictable scheduling
 * is desired, that mutex must be locked while calling this function.
 * @param cond the condition variable on which to produce the signal.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_signal(agc_thread_cond_t *cond);

/**
 * Signals all threads blocking on the given condition variable.
 * Each thread that was signaled is then scheduled to wake up and acquire
 * the associated mutex. This will happen in a serialized manner.
 * @param cond the condition variable on which to produce the broadcast.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_broadcast(agc_thread_cond_t *cond);

/**
 * Destroy the condition variable and free the associated memory.
 * @param cond the condition variable to destroy.
 */
AGC_DECLARE(agc_status_t) agc_thread_cond_destroy(agc_thread_cond_t *cond);

/** UUIDs are formatted as: 00112233-4455-6677-8899-AABBCCDDEEFF */
#define AGC_UUID_FORMATTED_LENGTH 256

#define AGC_MD5_DIGESTSIZE 16
#define AGC_MD5_DIGEST_STRING_SIZE 33

/**
 * Format a UUID into a string, following the standard format
 * @param buffer The buffer to place the formatted UUID string into. It must
 *               be at least APR_UUID_FORMATTED_LENGTH + 1 bytes long to hold
 *               the formatted UUID and a null terminator
 * @param uuid The UUID to format
 */
AGC_DECLARE(void) agc_uuid_format(char *buffer, const agc_uuid_t *uuid);

/**
 * Generate and return a (new) UUID
 * @param uuid The resulting UUID
 */
AGC_DECLARE(void) agc_uuid_get(agc_uuid_t *uuid);

/**
 * Parse a standard-format string into a UUID
 * @param uuid The resulting UUID
 * @param uuid_str The formatted UUID
 */
AGC_DECLARE(agc_status_t) agc_uuid_parse(agc_uuid_t *uuid, const char *uuid_str);


/**
 * MD5 in one step
 * @param digest The final MD5 digest
 * @param input The message block to use
 * @param inputLen The length of the message block
 */
AGC_DECLARE(agc_status_t) agc_md5(unsigned char digest[AGC_MD5_DIGESTSIZE], const void *input, agc_size_t inputLen);
AGC_DECLARE(agc_status_t) agc_md5_string(char digest_str[AGC_MD5_DIGEST_STRING_SIZE], const void *input, agc_size_t inputLen);

/** 
 * create a FIFO queue
 * @param queue The new queue
 * @param queue_capacity maximum size of the queue
 * @param pool a pool to allocate queue from
 */
AGC_DECLARE(agc_status_t) agc_queue_create(agc_queue_t ** queue, unsigned int queue_capacity, agc_memory_pool_t *pool);

/**
 * pop/get an object from the queue, blocking if the queue is already empty
 *
 * @param queue the queue
 * @param data the data
 * @returns APR_EINTR the blocking was interrupted (try again)
 * @returns APR_EOF if the queue has been terminated
 * @returns APR_SUCCESS on a successfull pop
 */
AGC_DECLARE(agc_status_t) agc_queue_pop(agc_queue_t *queue, void **data);

/**
 * pop/get an object from the queue, blocking if the queue is already empty
 *
 * @param queue the queue
 * @param data the data
 * @param timeout The amount of time in microseconds to wait. This is
 *        a maximum, not a minimum. If the condition is signaled, we
 *        will wake up before this time, otherwise the error APR_TIMEUP
 *        is returned.
 * @returns APR_TIMEUP the request timed out
 * @returns APR_EINTR the blocking was interrupted (try again)
 * @returns APR_EOF if the queue has been terminated
 * @returns APR_SUCCESS on a successfull pop
 */
AGC_DECLARE(agc_status_t) agc_queue_pop_timeout(agc_queue_t *queue, void **data, agc_interval_time_t timeout);

/**
 * push/add a object to the queue, blocking if the queue is already full
 *
 * @param queue the queue
 * @param data the data
 * @returns APR_EINTR the blocking was interrupted (try again)
 * @returns APR_EOF the queue has been terminated
 * @returns APR_SUCCESS on a successfull push
 */
AGC_DECLARE(agc_status_t) agc_queue_push(agc_queue_t *queue, void *data);

/**
 * returns the size of the queue.
 *
 * @warning this is not threadsafe, and is intended for reporting/monitoring
 * of the queue.
 * @param queue the queue
 * @returns the size of the queue
 */
AGC_DECLARE(unsigned int) agc_queue_size(agc_queue_t *queue);

/**
 * pop/get an object to the queue, returning immediatly if the queue is empty
 *
 * @param queue the queue
 * @param data the data
 * @returns APR_EINTR the blocking operation was interrupted (try again)
 * @returns APR_EAGAIN the queue is empty
 * @returns APR_EOF the queue has been terminated
 * @returns APR_SUCCESS on a successfull push
 */
AGC_DECLARE(agc_status_t) agc_queue_trypop(agc_queue_t *queue, void **data);

AGC_DECLARE(agc_status_t) agc_queue_interrupt_all(agc_queue_t *queue);

AGC_DECLARE(agc_status_t) agc_queue_term(agc_queue_t *queue);

/**
 * push/add a object to the queue, returning immediatly if the queue is full
 *
 * @param queue the queue
 * @param data the data
 * @returns APR_EINTR the blocking operation was interrupted (try again)
 * @returns APR_EAGAIN the queue is full
 * @returns APR_EOF the queue has been terminated
 * @returns APR_SUCCESS on a successfull push
 */
AGC_DECLARE(agc_status_t) agc_queue_trypush(agc_queue_t *queue, void *data);


/* flags for apr_file_seek */
/** Set the file position */
#define AGC_SEEK_SET SEEK_SET
/** Current */
#define AGC_SEEK_CUR SEEK_CUR
/** Go to end of file */
#define AGC_SEEK_END SEEK_END
/** @} */

#define AGC_FPROT_USETID 0x8000			/**< Set user id */
#define AGC_FPROT_UREAD 0x0400			/**< Read by user */
#define AGC_FPROT_UWRITE 0x0200			/**< Write by user */
#define AGC_FPROT_UEXECUTE 0x0100		/**< Execute by user */

#define AGC_FPROT_GSETID 0x4000			/**< Set group id */
#define AGC_FPROT_GREAD 0x0040			/**< Read by group */
#define AGC_FPROT_GWRITE 0x0020			/**< Write by group */
#define AGC_FPROT_GEXECUTE 0x0010		/**< Execute by group */

#define AGC_FPROT_WSTICKY 0x2000
#define AGC_FPROT_WREAD 0x0004			/**< Read by others */
#define AGC_FPROT_WWRITE 0x0002			/**< Write by others */
#define AGC_FPROT_WEXECUTE 0x0001		/**< Execute by others */

#define AGC_FPROT_OS_DEFAULT 0x0FFF		/**< use OS's default permissions */

/* additional permission flags for apr_file_copy  and apr_file_append */
#define AGC_FPROT_FILE_SOURCE_PERMS 0x1000	/**< Copy source file's permissions */

#define AGC_FLOCK_SHARED        1	   /**< Shared lock. More than one process
                                           or thread can hold a shared lock
                                           at any given time. Essentially,
                                           this is a "read lock", preventing
                                           writers from establishing an
                                           exclusive lock. */
#define AGC_FLOCK_EXCLUSIVE     2	   /**< Exclusive lock. Only one process
                                           may hold an exclusive lock at any
                                           given time. This is analogous to
                                           a "write lock". */

#define AGC_FLOCK_TYPEMASK      0x000F  /**< mask to extract lock type */
#define AGC_FLOCK_NONBLOCK      0x0010  /**< do not block while acquiring the file lock */

#define AGC_FOPEN_READ				0x00001		/**< Open the file for reading */
#define AGC_FOPEN_WRITE				0x00002		/**< Open the file for writing */
#define AGC_FOPEN_CREATE				0x00004		/**< Create the file if not there */
#define AGC_FOPEN_APPEND				0x00008		/**< Append to the end of the file */
#define AGC_FOPEN_TRUNCATE			0x00010		/**< Open the file and truncate to 0 length */
#define AGC_FOPEN_BINARY				0x00020		/**< Open the file in binary mode */
#define AGC_FOPEN_EXCL				0x00040		/**< Open should fail if APR_CREATE and file exists. */
#define AGC_FOPEN_BUFFERED			0x00080		/**< Open the file for buffered I/O */
#define AGC_FOPEN_DELONCLOSE			0x00100		/**< Delete the file after close */
#define AGC_FOPEN_XTHREAD			0x00200		/**< Platform dependent tag to open the file for use across multiple threads */
#define AGC_FOPEN_SHARELOCK			0x00400		/**< Platform dependent support for higher level locked read/write access to support writes across process/machines */
#define AGC_FOPEN_NOCLEANUP			0x00800		/**< Do not register a cleanup when the file is opened */
#define AGC_FOPEN_SENDFILE_ENABLED	0x01000		/**< Advisory flag that this file should support apr_socket_sendfile operation */
#define AGC_FOPEN_LARGEFILE			0x04000		/**< Platform dependent flag to enable large file support */

/**
 * Open the specified file.
 * @param newf The opened file descriptor.
 * @param fname The full path to the file (using / on all systems)
 * @param flag Or'ed value of:
 * <PRE>
 *         AGC_FOPEN_READ				open for reading
 *         AGC_FOPEN_WRITE				open for writing
 *         AGC_FOPEN_CREATE				create the file if not there
 *         AGC_FOPEN_APPEND				file ptr is set to end prior to all writes
 *         AGC_FOPEN_TRUNCATE			set length to zero if file exists
 *         AGC_FOPEN_BINARY				not a text file (This flag is ignored on 
 *											UNIX because it has no meaning)
 *         AGC_FOPEN_BUFFERED			buffer the data.  Default is non-buffered
 *         AGC_FOPEN_EXCL				return error if APR_CREATE and file exists
 *         AGC_FOPEN_DELONCLOSE			delete the file after closing.
 *         AGC_FOPEN_XTHREAD				Platform dependent tag to open the file
 *											for use across multiple threads
 *         AGC_FOPEN_SHARELOCK			Platform dependent support for higher
 *											level locked read/write access to support
 *											writes across process/machines
 *         AGC_FOPEN_NOCLEANUP			Do not register a cleanup with the pool 
 *											passed in on the <EM>pool</EM> argument (see below).
 *											The apr_os_file_t handle in apr_file_t will not
 *											be closed when the pool is destroyed.
 *         AGC_FOPEN_SENDFILE_ENABLED	Open with appropriate platform semantics
 *											for sendfile operations.  Advisory only,
 *											apr_socket_sendfile does not check this flag.
 * </PRE>
 * @param perm Access permissions for file.
 * @param pool The pool to use.
 * @remark If perm is AGC_FPROT_OS_DEFAULT and the file is being created,
 * appropriate default permissions will be used.
 */
AGC_DECLARE(agc_status_t) agc_file_open(agc_file_t ** newf, const char *fname, int32_t flag, agc_fileperms_t perm,
												 agc_memory_pool_t *pool);
                                                 
AGC_DECLARE(agc_status_t) agc_file_seek(agc_file_t *thefile, agc_seek_where_t where, int64_t *offset);

AGC_DECLARE(agc_status_t) agc_file_copy(const char *from_path, const char *to_path, agc_fileperms_t perms, agc_memory_pool_t *pool);

/**
 * Close the specified file.
 * @param thefile The file descriptor to close.
 */
AGC_DECLARE(agc_status_t) agc_file_close(agc_file_t *thefile);

AGC_DECLARE(agc_status_t) agc_file_trunc(agc_file_t *thefile, int64_t offset);

AGC_DECLARE(agc_status_t) agc_file_lock(agc_file_t *thefile, int type);

/**
 * Delete the specified file.
 * @param path The full path to the file (using / on all systems)
 * @param pool The pool to use.
 * @remark If the file is open, it won't be removed until all
 * instances are closed.
 */
AGC_DECLARE(agc_status_t) agc_file_remove(const char *path, agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_file_rename(const char *from_path, const char *to_path, agc_memory_pool_t *pool);

/**
 * Read data from the specified file.
 * @param thefile The file descriptor to read from.
 * @param buf The buffer to store the data to.
 * @param nbytes On entry, the number of bytes to read; on exit, the number
 * of bytes read.
 *
 * @remark apr_file_read will read up to the specified number of
 * bytes, but never more.  If there isn't enough data to fill that
 * number of bytes, all of the available data is read.  The third
 * argument is modified to reflect the number of bytes read.  If a
 * char was put back into the stream via ungetc, it will be the first
 * character returned.
 *
 * @remark It is not possible for both bytes to be read and an APR_EOF
 * or other error to be returned.  APR_EINTR is never returned.
 */
AGC_DECLARE(agc_status_t) agc_file_read(agc_file_t *thefile, void *buf, agc_size_t *nbytes);

/**
 * Write data to the specified file.
 * @param thefile The file descriptor to write to.
 * @param buf The buffer which contains the data.
 * @param nbytes On entry, the number of bytes to write; on exit, the number 
 *               of bytes written.
 *
 * @remark apr_file_write will write up to the specified number of
 * bytes, but never more.  If the OS cannot write that many bytes, it
 * will write as many as it can.  The third argument is modified to
 * reflect the * number of bytes written.
 *
 * @remark It is possible for both bytes to be written and an error to
 * be returned.  APR_EINTR is never returned.
 */
AGC_DECLARE(agc_status_t) agc_file_write(agc_file_t *thefile, const void *buf, agc_size_t *nbytes);
AGC_DECLARE(int) agc_file_printf(agc_file_t *thefile, const char *format, ...);

AGC_DECLARE(agc_status_t) agc_file_mktemp(agc_file_t ** thefile, char *templ, int32_t flags, agc_memory_pool_t *pool);

AGC_DECLARE(agc_size_t) agc_file_get_size(agc_file_t *thefile);

AGC_DECLARE(agc_status_t) agc_file_exists(const char *filename, agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_directory_exists(const char *dirname, agc_memory_pool_t *pool);

/**
* Create a new directory on the file system.
* @param path the path for the directory to be created. (use / on all systems)
* @param perm Permissions for the new direcoty.
* @param pool the pool to use.
*/
AGC_DECLARE(agc_status_t) agc_dir_make(const char *path, agc_fileperms_t perm, agc_memory_pool_t *pool);

/** Creates a new directory on the file system, but behaves like
* 'mkdir -p'. Creates intermediate directories as required. No error
* will be reported if PATH already exists.
* @param path the path for the directory to be created. (use / on all systems)
* @param perm Permissions for the new direcoty.
* @param pool the pool to use.
*/
AGC_DECLARE(agc_status_t) agc_dir_make_recursive(const char *path, agc_fileperms_t perm, agc_memory_pool_t *pool);

struct agc_dir {
	apr_dir_t *dir_handle;
	apr_finfo_t finfo;
};

typedef struct agc_dir agc_dir_t;

struct agc_array_header_t {
	/** The pool the array is allocated out of */
    agc_memory_pool_t *pool;
	/** The amount of memory allocated for each element of the array */
    int elt_size;
	/** The number of active elements in the array */
    int nelts;
	/** The number of elements allocated in the array */
    int nalloc;
	/** The elements in the array */
    char *elts;
};

typedef struct agc_array_header_t agc_array_header_t;

AGC_DECLARE(agc_status_t) agc_dir_open(agc_dir_t ** new_dir, const char *dirname, agc_memory_pool_t *pool);
AGC_DECLARE(agc_status_t) agc_dir_close(agc_dir_t *thedir);
AGC_DECLARE(const char *) agc_dir_next_file(agc_dir_t *thedir, char *buf, agc_size_t len);
AGC_DECLARE(uint32_t) agc_dir_count(agc_dir_t *thedir);

/** Opaque Thread structure. */
typedef struct apr_thread_t agc_thread_t;

#ifndef WIN32
struct apr_threadattr_t {
	apr_pool_t *pool;
	pthread_attr_t attr;
	int priority;
};
#else
struct apr_threadattr_t {
    apr_pool_t *pool;
    apr_int32_t detach;
    apr_size_t stacksize;
	int priority;
};
#endif

/** Opaque Thread attributes structure. */
typedef struct apr_threadattr_t agc_threadattr_t;

/**
 * The prototype for any APR thread worker functions.
 * typedef void *(AGC_THREAD_FUNC *agc_thread_start_t)(agc_thread_t*, void*);
 */
typedef void *(AGC_THREAD_FUNC * agc_thread_start_t) (agc_thread_t *, void *);

//APR_DECLARE(apr_status_t) apr_threadattr_stacksize_set(apr_threadattr_t *attr, agc_size_t stacksize)
AGC_DECLARE(agc_status_t) agc_threadattr_stacksize_set(agc_threadattr_t *attr, agc_size_t stacksize);

AGC_DECLARE(agc_status_t) agc_threadattr_priority_set(agc_threadattr_t *attr, agc_thread_priority_t priority);


/**
 * Create and initialize a new threadattr variable
 * @param new_attr The newly created threadattr.
 * @param pool The pool to use
 */
AGC_DECLARE(agc_status_t) agc_threadattr_create(agc_threadattr_t ** new_attr, agc_memory_pool_t *pool);

/**
 * Set if newly created threads should be created in detached state.
 * @param attr The threadattr to affect 
 * @param on Non-zero if detached threads should be created.
 */
AGC_DECLARE(agc_status_t) agc_threadattr_detach_set(agc_threadattr_t *attr, int32_t on);

/**
 * Create a new thread of execution
 * @param new_thread The newly created thread handle.
 * @param attr The threadattr to use to determine how to create the thread
 * @param func The function to start the new thread in
 * @param data Any data to be passed to the starting function
 * @param cont The pool to use
 */
AGC_DECLARE(agc_status_t) agc_thread_create(agc_thread_t ** new_thread, agc_threadattr_t *attr,
													 agc_thread_start_t func, void *data, agc_memory_pool_t *cont);
                                                     
#define AGC_SO_LINGER 1
#define AGC_SO_KEEPALIVE 2
#define AGC_SO_DEBUG 4
#define AGC_SO_NONBLOCK 8
#define AGC_SO_REUSEADDR 16
#define AGC_SO_SNDBUF 64
#define AGC_SO_RCVBUF 128
#define AGC_SO_DISCONNECTED 256
#define AGC_SO_TCP_NODELAY 512
#define AGC_SO_TCP_KEEPIDLE 520
#define AGC_SO_TCP_KEEPINTVL 530                                                     

 /**
 * @def AGC_INET
 * Not all platforms have these defined, so we'll define them here
 * The default values come from FreeBSD 4.1.1
 */
#define AGC_INET     AF_INET
#ifdef AF_INET6
#define AGC_INET6    AF_INET6
#else
#define AGC_INET6 0
#endif

/** @def AGC_UNSPEC
 * Let the system decide which address family to use
 */
#ifdef AF_UNSPEC
#define AGC_UNSPEC   AF_UNSPEC
#else
#define AGC_UNSPEC   0
#endif

/** A structure to represent sockets */
typedef struct apr_socket_t agc_socket_t;

typedef struct apr_sockaddr_t agc_sockaddr_t;

typedef enum {
		 AGC_SHUTDOWN_READ,	   /**< no longer allow read request */
		 AGC_SHUTDOWN_WRITE,	   /**< no longer allow write requests */
		 AGC_SHUTDOWN_READWRITE /**< no longer allow read or write requests */
} agc_shutdown_how_e;

#define AGC_PROTO_TCP       6   /**< TCP  */
#define AGC_PROTO_UDP      17   /**< UDP  */
#define AGC_PROTO_SCTP    132   /**< SCTP */

/**
 * Create a socket.
 * @param new_sock The new socket that has been set up.
 * @param family The address family of the socket (e.g., AGC_INET).
 * @param type The type of the socket (e.g., SOCK_STREAM).
 * @param protocol The protocol of the socket (e.g., AGC_PROTO_TCP).
 * @param pool The pool to use
 */
AGC_DECLARE(agc_status_t) agc_socket_create(agc_socket_t ** new_sock, int family, int type, int protocol, agc_memory_pool_t *pool);

/**
 * Shutdown either reading, writing, or both sides of a socket.
 * @param sock The socket to close 
 * @param how How to shutdown the socket.  One of:
 * <PRE>
 *            AGC_SHUTDOWN_READ         no longer allow read requests
 *            AGC_SHUTDOWN_WRITE        no longer allow write requests
 *            AGC_SHUTDOWN_READWRITE    no longer allow read or write requests 
 * </PRE>
 * @see agc_shutdown_how_e
 * @remark This does not actually close the socket descriptor, it just
 *      controls which calls are still valid on the socket.
 */
AGC_DECLARE(agc_status_t) agc_socket_shutdown(agc_socket_t *sock, agc_shutdown_how_e how);

/**
 * Close a socket.
 * @param sock The socket to close 
 */
AGC_DECLARE(agc_status_t) agc_socket_close(agc_socket_t *sock);

/**
 * Bind the socket to its associated port
 * @param sock The socket to bind 
 * @param sa The socket address to bind to
 * @remark This may be where we will find out if there is any other process
 *      using the selected port.
 */
AGC_DECLARE(agc_status_t) agc_socket_bind(agc_socket_t *sock, agc_sockaddr_t *sa);

/**
 * Listen to a bound socket for connections.
 * @param sock The socket to listen on 
 * @param backlog The number of outstanding connections allowed in the sockets
 *                listen queue.  If this value is less than zero, the listen
 *                queue size is set to zero.  
 */
AGC_DECLARE(agc_status_t) agc_socket_listen(agc_socket_t *sock, int32_t backlog);

/**
 * Accept a new connection request
 * @param new_sock A copy of the socket that is connected to the socket that
 *                 made the connection request.  This is the socket which should
 *                 be used for all future communication.
 * @param sock The socket we are listening on.
 * @param pool The pool for the new socket.
 */
AGC_DECLARE(agc_status_t) agc_socket_accept(agc_socket_t ** new_sock, agc_socket_t *sock, agc_memory_pool_t *pool);

/**
 * Issue a connection request to a socket either on the same machine 
 * or a different one.
 * @param sock The socket we wish to use for our side of the connection 
 * @param sa The address of the machine we wish to connect to.
 */
AGC_DECLARE(agc_status_t) agc_socket_connect(agc_socket_t *sock, agc_sockaddr_t *sa);

AGC_DECLARE(uint16_t) agc_sockaddr_get_port(agc_sockaddr_t *sa);
AGC_DECLARE(const char *) agc_get_addr(char *buf, agc_size_t len, agc_sockaddr_t *in);
AGC_DECLARE(agc_status_t) agc_getnameinfo(char **hostname, agc_sockaddr_t *sa, int32_t flags);
AGC_DECLARE(int32_t) agc_sockaddr_get_family(agc_sockaddr_t *sa);
AGC_DECLARE(agc_status_t) agc_sockaddr_ip_get(char **addr, agc_sockaddr_t *sa);
AGC_DECLARE(int) agc_sockaddr_equal(const agc_sockaddr_t *sa1, const agc_sockaddr_t *sa2);

/**
 * Create apr_sockaddr_t from hostname, address family, and port.
 * @param sa The new apr_sockaddr_t.
 * @param hostname The hostname or numeric address string to resolve/parse, or
 *               NULL to build an address that corresponds to 0.0.0.0 or ::
 * @param family The address family to use, or AGC_UNSPEC if the system should 
 *               decide.
 * @param port The port number.
 * @param flags Special processing flags:
 * <PRE>
 *       APR_IPV4_ADDR_OK          first query for IPv4 addresses; only look
 *                                 for IPv6 addresses if the first query failed;
 *                                 only valid if family is APR_UNSPEC and hostname
 *                                 isn't NULL; mutually exclusive with
 *                                 APR_IPV6_ADDR_OK
 *       APR_IPV6_ADDR_OK          first query for IPv6 addresses; only look
 *                                 for IPv4 addresses if the first query failed;
 *                                 only valid if family is APR_UNSPEC and hostname
 *                                 isn't NULL and APR_HAVE_IPV6; mutually exclusive
 *                                 with APR_IPV4_ADDR_OK
 * </PRE>
 * @param pool The pool for the apr_sockaddr_t and associated storage.
 */
AGC_DECLARE(agc_status_t) agc_sockaddr_info_get(agc_sockaddr_t ** sa, const char *hostname,
														 int32_t family, agc_port_t port, int32_t flags, agc_memory_pool_t *pool);

AGC_DECLARE(agc_status_t) agc_sockaddr_create(agc_sockaddr_t **sa, agc_memory_pool_t *pool);

/**
 * Send data over a network.
 * @param sock The socket to send the data over.
 * @param buf The buffer which contains the data to be sent. 
 * @param len On entry, the number of bytes to send; on exit, the number
 *            of bytes sent.
 * @remark
 * <PRE>
 * This functions acts like a blocking write by default.  To change 
 * this behavior, use apr_socket_timeout_set() or the APR_SO_NONBLOCK
 * socket option.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
AGC_DECLARE(agc_status_t) agc_socket_send(agc_socket_t *sock, const char *buf, agc_size_t *len);

/**
 * @param sock The socket to send from
 * @param where The apr_sockaddr_t describing where to send the data
 * @param flags The flags to use
 * @param buf  The data to send
 * @param len  The length of the data to send
 */
AGC_DECLARE(agc_status_t) agc_socket_sendto(agc_socket_t *sock, agc_sockaddr_t *where, int32_t flags, const char *buf,
													 agc_size_t *len);
													
AGC_DECLARE(agc_status_t) agc_socket_send_nonblock(agc_socket_t *sock, const char *buf, agc_size_t *len);


/**
 * @param from The apr_sockaddr_t to fill in the recipient info
 * @param sock The socket to use
 * @param flags The flags to use
 * @param buf  The buffer to use
 * @param len  The length of the available buffer
 *
 */
AGC_DECLARE(agc_status_t) agc_socket_recvfrom(agc_sockaddr_t *from, agc_socket_t *sock, int32_t flags, char *buf, size_t *len);

AGC_DECLARE(agc_status_t) agc_socket_atmark(agc_socket_t *sock, int *atmark);

/**
 * Read data from a network.
 * @param sock The socket to read the data from.
 * @param buf The buffer to store the data in. 
 * @param len On entry, the number of bytes to receive; on exit, the number
 *            of bytes received.
 * @remark
 * <PRE>
 * This functions acts like a blocking read by default.  To change 
 * this behavior, use apr_socket_timeout_set() or the APR_SO_NONBLOCK
 * socket option.
 * The number of bytes actually received is stored in argument 3.
 *
 * It is possible for both bytes to be received and an APR_EOF or
 * other error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
AGC_DECLARE(agc_status_t) agc_socket_recv(agc_socket_t *sock, char *buf, agc_size_t *len);

/**
 * Setup socket options for the specified socket
 * @param sock The socket to set up.
 * @param opt The option we would like to configure.  One of:
 * <PRE>
 *            AGC_SO_DEBUG      --  turn on debugging information 
 *            AGC_SO_KEEPALIVE  --  keep connections active
 *            AGC_SO_LINGER     --  lingers on close if data is present
 *            AGC_SO_NONBLOCK   --  Turns blocking on/off for socket
 *                                  When this option is enabled, use
 *                                  the APR_STATUS_IS_EAGAIN() macro to
 *                                  see if a send or receive function
 *                                  could not transfer data without
 *                                  blocking.
 *            AGC_SO_REUSEADDR  --  The rules used in validating addresses
 *                                  supplied to bind should allow reuse
 *                                  of local addresses.
 *            AGC_SO_SNDBUF     --  Set the SendBufferSize
 *            AGC_SO_RCVBUF     --  Set the ReceiveBufferSize
 * </PRE>
 * @param on Value for the option.
 */
AGC_DECLARE(agc_status_t) agc_socket_opt_set(agc_socket_t *sock, int32_t opt, int32_t on);

/**
 * Setup socket timeout for the specified socket
 * @param sock The socket to set up.
 * @param t Value for the timeout.
 * <PRE>
 *   t > 0  -- read and write calls return APR_TIMEUP if specified time
 *             elapsess with no data read or written
 *   t == 0 -- read and write calls never block
 *   t < 0  -- read and write calls block
 * </PRE>
 */
AGC_DECLARE(agc_status_t) agc_socket_timeout_set(agc_socket_t *sock, agc_interval_time_t t);

/**
 * Join a Multicast Group
 * @param sock The socket to join a multicast group
 * @param join The address of the multicast group to join
 * @param iface Address of the interface to use.  If NULL is passed, the 
 *              default multicast interface will be used. (OS Dependent)
 * @param source Source Address to accept transmissions from (non-NULL 
 *               implies Source-Specific Multicast)
 */
AGC_DECLARE(agc_status_t) agc_mcast_join(agc_socket_t *sock, agc_sockaddr_t *join, agc_sockaddr_t *iface, agc_sockaddr_t *source);

/**
 * Set the Multicast Time to Live (ttl) for a multicast transmission.
 * @param sock The socket to set the multicast ttl
 * @param ttl Time to live to Assign. 0-255, default=1
 * @remark If the TTL is 0, packets will only be seen by sockets on the local machine,
 *     and only when multicast loopback is enabled.
 */
AGC_DECLARE(agc_status_t) agc_mcast_hops(agc_socket_t *sock, uint8_t ttl);

AGC_DECLARE(agc_status_t) agc_mcast_loopback(agc_socket_t *sock, uint8_t opt);
AGC_DECLARE(agc_status_t) agc_mcast_interface(agc_socket_t *sock, agc_sockaddr_t *iface);

typedef enum {
	AGC_NO_DESC,		   /**< nothing here */
	AGC_POLL_SOCKET,	   /**< descriptor refers to a socket */
	AGC_POLL_FILE,		   /**< descriptor refers to a file */
	AGC_POLL_LASTDESC	   /**< descriptor is the last one in the list */
} agc_pollset_type_t;

typedef union {
	agc_file_t *f;		   /**< file */
	agc_socket_t *s;	   /**< socket */
} agc_descriptor_t;

struct agc_pollfd {
	agc_memory_pool_t *p;		  /**< associated pool */
	agc_pollset_type_t desc_type;
									   /**< descriptor type */
	int16_t reqevents;	/**< requested events */
	int16_t rtnevents;	/**< returned events */
	agc_descriptor_t desc;	 /**< @see apr_descriptor */
	void *client_data;		/**< allows app to associate context */
};

/** Poll descriptor set. */
typedef struct agc_pollfd agc_pollfd_t;

/** Opaque structure used for pollset API */
typedef struct apr_pollset_t agc_pollset_t;

/**
 * Poll options
 */
#define AGC_POLLIN 0x001			/**< Can read without blocking */
#define AGC_POLLPRI 0x002			/**< Priority data available */
#define AGC_POLLOUT 0x004			/**< Can write without blocking */
#define AGC_POLLERR 0x010			/**< Pending error */
#define AGC_POLLHUP 0x020			/**< Hangup occurred */
#define AGC_POLLNVAL 0x040		/**< Descriptior invalid */

/**
 * Setup a pollset object
 * @param pollset  The pointer in which to return the newly created object 
 * @param size The maximum number of descriptors that this pollset can hold
 * @param pool The pool from which to allocate the pollset
 * @param flags Optional flags to modify the operation of the pollset.
 *
 * @remark If flags equals APR_POLLSET_THREADSAFE, then a pollset is
 * created on which it is safe to make concurrent calls to
 * apr_pollset_add(), apr_pollset_remove() and apr_pollset_poll() from
 * separate threads.  This feature is only supported on some
 * platforms; the apr_pollset_create() call will fail with
 * APR_ENOTIMPL on platforms where it is not supported.
 */
AGC_DECLARE(agc_status_t) agc_pollset_create(agc_pollset_t ** pollset, uint32_t size, agc_memory_pool_t *pool, uint32_t flags);

/**
 * Add a socket or file descriptor to a pollset
 * @param pollset The pollset to which to add the descriptor
 * @param descriptor The descriptor to add
 * @remark If you set client_data in the descriptor, that value
 *         will be returned in the client_data field whenever this
 *         descriptor is signalled in apr_pollset_poll().
 * @remark If the pollset has been created with APR_POLLSET_THREADSAFE
 *         and thread T1 is blocked in a call to apr_pollset_poll() for
 *         this same pollset that is being modified via apr_pollset_add()
 *         in thread T2, the currently executing apr_pollset_poll() call in
 *         T1 will either: (1) automatically include the newly added descriptor
 *         in the set of descriptors it is watching or (2) return immediately
 *         with APR_EINTR.  Option (1) is recommended, but option (2) is
 *         allowed for implementations where option (1) is impossible
 *         or impractical.
 */
AGC_DECLARE(agc_status_t) agc_pollset_add(agc_pollset_t *pollset, const agc_pollfd_t *descriptor);

/**
 * Remove a descriptor from a pollset
 * @param pollset The pollset from which to remove the descriptor
 * @param descriptor The descriptor to remove
 * @remark If the pollset has been created with APR_POLLSET_THREADSAFE
 *         and thread T1 is blocked in a call to apr_pollset_poll() for
 *         this same pollset that is being modified via apr_pollset_remove()
 *         in thread T2, the currently executing apr_pollset_poll() call in
 *         T1 will either: (1) automatically exclude the newly added descriptor
 *         in the set of descriptors it is watching or (2) return immediately
 *         with APR_EINTR.  Option (1) is recommended, but option (2) is
 *         allowed for implementations where option (1) is impossible
 *         or impractical.
 */
AGC_DECLARE(agc_status_t) agc_pollset_remove(agc_pollset_t *pollset, const agc_pollfd_t *descriptor);

/**
 * Poll the sockets in the poll structure
 * @param aprset The poll structure we will be using. 
 * @param numsock The number of sockets we are polling
 * @param nsds The number of sockets signalled.
 * @param timeout The amount of time in microseconds to wait.  This is 
 *                a maximum, not a minimum.  If a socket is signalled, we 
 *                will wake up before this time.  A negative number means 
 *                wait until a socket is signalled.
 * @remark The number of sockets signalled is returned in the third argument. 
 *         This is a blocking call, and it will not return until either a 
 *         socket has been signalled, or the timeout has expired. 
 */
AGC_DECLARE(agc_status_t) agc_poll(agc_pollfd_t *aprset, int32_t numsock, int32_t *nsds, agc_interval_time_t timeout);

/**
 * Block for activity on the descriptor(s) in a pollset
 * @param pollset The pollset to use
 * @param timeout Timeout in microseconds
 * @param num Number of signalled descriptors (output parameter)
 * @param descriptors Array of signalled descriptors (output parameter)
 */
AGC_DECLARE(agc_status_t) agc_pollset_poll(agc_pollset_t *pollset, agc_interval_time_t timeout, int32_t *num, const agc_pollfd_t **descriptors);

/*!
  \brief Create a set of file descriptors to poll from a socket
  \param poll the polfd to create
  \param sock the socket to add
  \param flags the flags to modify the behaviour
  \param pool the memory pool to use
  \return AGC_STATUS_SUCCESS when successful
*/
AGC_DECLARE(agc_status_t) agc_socket_create_pollset(agc_pollfd_t ** poll, agc_socket_t *sock, int16_t flags, agc_memory_pool_t *pool);

AGC_DECLARE(agc_interval_time_t) agc_interval_time_from_timeval(struct timeval *tvp);

/*!
  \brief Create a pollfd out of a socket
  \param pollfd the pollfd to create
  \param sock the socket to add
  \param flags the flags to modify the behaviour
  \param client_data custom user data
  \param pool the memory pool to use
  \return AGC_STATUS_SUCCESS when successful
*/
AGC_DECLARE(agc_status_t) agc_socket_create_pollfd(agc_pollfd_t **pollfd, agc_socket_t *sock, int16_t flags, void *client_data, agc_memory_pool_t *pool);
AGC_DECLARE(agc_status_t) agc_match_glob(const char *pattern, agc_array_header_t ** result, agc_memory_pool_t *pool);
AGC_DECLARE(agc_status_t) agc_os_sock_get(agc_os_socket_t *thesock, agc_socket_t *sock);
AGC_DECLARE(agc_status_t) agc_os_sock_put(agc_socket_t **sock, agc_os_socket_t *thesock, agc_memory_pool_t *pool);
AGC_DECLARE(agc_status_t) agc_socket_addr_get(agc_sockaddr_t ** sa, agc_bool_t remote, agc_socket_t *sock);

/**
 * Create an anonymous pipe.
 * @param in The file descriptor to use as input to the pipe.
 * @param out The file descriptor to use as output from the pipe.
 * @param pool The pool to operate on.
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_create(agc_file_t ** in, agc_file_t ** out, agc_memory_pool_t *pool);

/**
 * Get the timeout value for a pipe or manipulate the blocking state.
 * @param thepipe The pipe we are getting a timeout for.
 * @param timeout The current timeout value in microseconds. 
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_timeout_get(agc_file_t *thepipe, agc_interval_time_t *timeout);

/**
 * Set the timeout value for a pipe or manipulate the blocking state.
 * @param thepipe The pipe we are setting a timeout on.
 * @param timeout The timeout value in microseconds.  Values < 0 mean wait 
 *        forever, 0 means do not wait at all.
 */
AGC_DECLARE(agc_status_t) agc_file_pipe_timeout_set(agc_file_t *thepipe, agc_interval_time_t timeout);

/**
 * stop the current thread
 * @param thd The thread to stop
 * @param retval The return value to pass back to any thread that cares
 */
AGC_DECLARE(agc_status_t) agc_thread_exit(agc_thread_t *thd, agc_status_t retval);

/**
 * block until the desired thread stops executing.
 * @param retval The return value from the dead thread.
 * @param thd The thread to join
 */
AGC_DECLARE(agc_status_t) agc_thread_join(agc_status_t *retval, agc_thread_t *thd);

/**
 * Return a human readable string describing the specified error.
 * @param statcode The error code the get a string for.
 * @param buf A buffer to hold the error string.
 * @bufsize Size of the buffer to hold the string.
 */

AGC_DECLARE(char *) agc_strerror(agc_status_t statcode, char *buf, agc_size_t bufsize);


AGC_END_EXTERN_C
#endif