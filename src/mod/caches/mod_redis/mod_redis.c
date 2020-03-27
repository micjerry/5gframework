#include <agc.h>

AGC_MODULE_LOAD_FUNCTION(mod_redis_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown);
AGC_MODULE_DEFINITION(mod_redis, mod_redis_load, mod_redis_shutdown, NULL);

AGC_MODULE_LOAD_FUNCTION(mod_redis_load)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Redis shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}