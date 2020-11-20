#include <agc.h>
#include "private/agc_core_pvt.h"

static const char *LEVELS[] = {
	"CONSOLE",
	"ALERT",
	"CRIT",
	"ERR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	NULL
};

struct agc_log_binding {
	agc_log_function_t function;
	agc_log_level_t level;
	int is_console;
	struct agc_log_binding *next;
};

typedef struct agc_log_binding agc_log_binding_t;

static agc_memory_pool_t *LOG_MEMORY_POOL = NULL;
static agc_log_binding_t *LOGGER_BINDINGS = NULL;
static agc_mutex_t *LOGGER_BIND_LOCK = NULL;
static agc_queue_t *LOG_MESSAGE_QUEUE = NULL;

static volatile int8_t LOG_THREAD_RUNNING = 0;
static uint8_t MAX_LOG_LEVEL = 0;

static int mods_loaded = 0;
static int console_mods_loaded = 0;

static agc_bool_t COLORIZE = AGC_FALSE;

static agc_thread_t *log_thread;

static const char *COLORS[] =
	{ AGC_SEQ_DEFAULT_COLOR, AGC_SEQ_FRED, AGC_SEQ_FRED, AGC_SEQ_FRED, AGC_SEQ_FMAGEN, AGC_SEQ_FCYAN, AGC_SEQ_FGREEN, AGC_SEQ_FYELLOW};
    
static agc_log_node_t *agc_log_node_alloc()
{
	agc_log_node_t *node = NULL;
    node = malloc(sizeof(*node));
    agc_assert(node);
	return node;
}

static void *log_thread_func(agc_thread_t *t, void *obj)
{
	if (!obj) {
		obj = NULL;
	}
    
	LOG_THREAD_RUNNING = 1;
    
	while (LOG_THREAD_RUNNING == 1) {
		void *pop = NULL;
		agc_log_node_t *node = NULL;
		agc_log_binding_t *binding;
        
		if (agc_queue_pop(LOG_MESSAGE_QUEUE, &pop) != AGC_STATUS_SUCCESS) {
			break;
		}
        
		if (!pop) {
			LOG_THREAD_RUNNING = -1;
			break;
		}
        
		node = (agc_log_node_t *) pop;
		agc_mutex_lock(LOGGER_BIND_LOCK);
		for (binding = LOGGER_BINDINGS; binding; binding = binding->next) {
			if (binding->level >= node->level) {
				binding->function(node, node->level);
			}
		}
		agc_mutex_unlock(LOGGER_BIND_LOCK);
        
		agc_log_node_free(&node);
	}
    
	LOG_THREAD_RUNNING = 0;
	return NULL;
}

AGC_DECLARE(agc_status_t) agc_log_init(agc_memory_pool_t *pool, agc_bool_t colorize)
{
	agc_threadattr_t *thd_attr;
	agc_assert(pool != NULL);
    
	LOG_MEMORY_POOL = pool;
    
	agc_threadattr_create(&thd_attr, LOG_MEMORY_POOL);
	agc_queue_create(&LOG_MESSAGE_QUEUE, AGC_LOG_QUEUE_LEN, LOG_MEMORY_POOL);
	agc_mutex_init(&LOGGER_BIND_LOCK, AGC_MUTEX_NESTED, LOG_MEMORY_POOL);
	agc_threadattr_stacksize_set(thd_attr, AGC_THREAD_STACKSIZE);
	agc_thread_create(&log_thread, thd_attr, log_thread_func, NULL, LOG_MEMORY_POOL);
	while (!LOG_THREAD_RUNNING) {
		agc_cond_next();
	}
    
	if (colorize) {
	COLORIZE = AGC_TRUE;
	}
    
	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Log init success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_log_shutdown(void)
{
	agc_status_t st;

	agc_queue_push(LOG_MESSAGE_QUEUE, NULL);
	while (LOG_THREAD_RUNNING) {
		agc_cond_next();
	}

	agc_thread_join(&st, log_thread);

	agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Log shutdown success.\n");
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(void) agc_log_printf(agc_log_type_t type, 
                                 const char *file,
								 const char *func, 
                                 int line,
								 const char *userdata, 
                                 agc_log_level_t level,
								 const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	agc_log_vprintf(type, file, func, line, userdata, level, fmt, ap);
	va_end(ap);
}

AGC_DECLARE(void) agc_log_vprintf(agc_log_type_t type, 
                                  const char *file,
								  const char *func, int line,
								  const char *userdata, 
                                  agc_log_level_t level, 
                                  const char *fmt, 
                                  va_list ap)
{
	char *data = NULL;
	char *content = NULL;
	int ret = 0;
	uint32_t len;
	FILE *console;
	const char *funcp = (func ? func : "");
	const char *filep = (file ? agc_cut_path(file) : "");
	const char *extra_fmt = "%s [%s] %s:%d %s";
	char *full_fmt = NULL;

	char date[80] = "";
	agc_time_t now = {0};
	agc_time_exp_t tm;

    
	if (level >  runtime.hard_log_level) {
		return;
	}
    
	agc_assert(level < AGC_LOG_INVALID);

	console = runtime.console;

	//format log datetime
	now = agc_timer_curtime();
	// TODO now = agc_micro_time_now();
	agc_time_exp_lt(&tm, now);
	agc_snprintf(date, sizeof(date), "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d.%0.6d",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_usec);
	len = (uint32_t) (strlen(extra_fmt) + strlen(date) + strlen(filep) + 32 + strlen(fmt));
	full_fmt = malloc(len + 1);
	agc_assert(full_fmt);

	agc_snprintf(full_fmt, len, extra_fmt, date, agc_log_level2str(level), filep, line, fmt);

	ret = agc_vasprintf(&data, full_fmt, ap);
	if (ret == -1) {
		fprintf(stderr, "Memory Error\n");
		agc_safe_free(data);
		agc_safe_free(full_fmt);
		return;
	}
    
	if (console_mods_loaded == 0 || !(LOG_MESSAGE_QUEUE && LOG_THREAD_RUNNING)) {
		if (console) {
			int aok = 1;
			fd_set can_write;
			int fd;
			struct timeval to;

			fd = fileno(console);
			memset(&to, 0, sizeof(to));
			FD_ZERO(&can_write);
			FD_SET(fd, &can_write);
			to.tv_sec = 0;
			to.tv_usec = 100000;
			if (select(fd + 1, NULL, &can_write, NULL, &to) > 0) {
				aok = FD_ISSET(fd, &can_write);
			} else {
				aok = 0;
			}
	        
			if (aok) {
				if (COLORIZE) {
					fprintf(console, "%s%s%s", COLORS[level], data, AGC_SEQ_DEFAULT_COLOR);
				} else {
					fprintf(console, "%s", data);
				}
			}
		}
	}
    
	if ((LOG_MESSAGE_QUEUE && LOG_THREAD_RUNNING) && level <= MAX_LOG_LEVEL) {
		agc_log_node_t *node = agc_log_node_alloc();
		node->data = data;
		data = NULL;
		agc_set_string(node->file, filep);
		agc_set_string(node->func, funcp);
		node->line = line;
		node->level = level;
		node->timestamp = now;
		node->type = type;
		node->userdata = NULL;
		if (agc_queue_trypush(LOG_MESSAGE_QUEUE, node) != AGC_STATUS_SUCCESS) {
			agc_log_node_free(&node);
		}
	}
    
	agc_safe_free(data);
	agc_safe_free(full_fmt);
}
           
AGC_DECLARE(agc_status_t) agc_log_bind_logger(agc_log_function_t function, agc_log_level_t level, agc_bool_t is_console)
{
	agc_log_binding_t *binding = NULL, *ptr = NULL;
	agc_assert(function != NULL);

	if (!(binding = agc_memory_alloc(LOG_MEMORY_POOL, sizeof(*binding)))) {
		return AGC_STATUS_MEMERR;
	}

	if ((uint8_t) level > MAX_LOG_LEVEL) {
		MAX_LOG_LEVEL = level;
	}

	binding->function = function;
	binding->level = level;
	binding->is_console = is_console;

	//printf( "Log bind %d .\n", level);
	if (level > runtime.hard_log_level) {
		//printf("Log bind reset runtime.hard_log_level %d .\n", level);
		runtime.hard_log_level = level;
	}

	agc_mutex_lock(LOGGER_BIND_LOCK);
	for (ptr = LOGGER_BINDINGS; ptr && ptr->next; ptr = ptr->next);
	if (ptr) {
		ptr->next = binding;
	} else {
		LOGGER_BINDINGS = binding;
	}

	if (is_console) {
		console_mods_loaded++;
	}

	mods_loaded++;
	agc_mutex_unlock(LOGGER_BIND_LOCK);
	return AGC_STATUS_SUCCESS;
}

AGC_DECLARE(agc_status_t) agc_log_unbind_logger(agc_log_function_t function)
{
    agc_log_binding_t *bind = NULL, *pre = NULL;
	agc_status_t status = AGC_STATUS_FALSE;
    
    agc_mutex_lock(LOGGER_BIND_LOCK);
    for (bind = LOGGER_BINDINGS; bind; bind = bind->next) {
		if (bind->function == function) {
			if (pre) {
				pre->next = bind->next;
			} else {
				LOGGER_BINDINGS = bind->next;
			}
			status = AGC_STATUS_SUCCESS;
			mods_loaded--;
			if (bind->is_console) {
				console_mods_loaded--;
			}
			break;
		}
		pre = bind;
	}
    agc_mutex_unlock(LOGGER_BIND_LOCK);
}

AGC_DECLARE(const char *) agc_log_level2str(agc_log_level_t level)
{
    if (level > AGC_LOG_DEBUG) {
		level = AGC_LOG_DEBUG;
	}
	return LEVELS[level];
}

AGC_DECLARE(agc_log_level_t) agc_log_str2level(const char *str)
{
    int x = 0;
    agc_log_level_t level = AGC_LOG_INVALID;
    
    if (agc_is_number(str)) {
        x = atoi(str);
        
        if (x > AGC_LOG_INVALID) {
			return AGC_LOG_INVALID - 1;
		} else if (x < 0) {
			return 0;
		} else {
			return x;
		}
    }
    
    for (x = 0;; x++) {
		if (!LEVELS[x]) {
			break;
		}

		if (!strcasecmp(LEVELS[x], str)) {
			level = (agc_log_level_t) x;
			break;
		}
	}

	return level;
}

AGC_DECLARE(agc_log_node_t *) agc_log_node_dup(const agc_log_node_t *node)
{
	agc_log_node_t *newnode = agc_log_node_alloc();

	*newnode = *node;

	if (!zstr(node->data)) {
		newnode->data = strdup(node->data);
		agc_assert(newnode->data);
	}

	if (!zstr(node->userdata)) {
		newnode->userdata = strdup(node->userdata);
		agc_assert(newnode->userdata);
	}

	return newnode;
}

AGC_DECLARE(void) agc_log_node_free(agc_log_node_t **pnode)
{
    agc_log_node_t *node;
    
    if (!pnode) {
		return;
	}
    
    node = *pnode;
    if (node) {
        agc_safe_free(node->userdata);
		agc_safe_free(node->data);
        agc_safe_free(node);
    }
    
    *pnode = NULL;
}

