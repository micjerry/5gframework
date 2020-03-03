#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_general.h>
#include <apr_portable.h>

#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <unistd.h>

/* getgrnam, getpwnam */
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

struct agc_runtime {
    agc_time_t initialed;
    agc_memory_pool_t *memory_pool;
    char hostname[256];
    int cpu_count;
    uint32_t runlevel;
    agc_mutex_t *uuid_mutex;
    agc_mutex_t *global_mutex;
    agc_log_level_t hard_log_level;
    FILE *console;
};

extern struct agc_runtime runtime;

agc_memory_pool_t *agc_core_memory_init(void);
