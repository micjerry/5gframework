#include <agc.h>

AGC_MODULE_LOAD_FUNCTION(mod_sqlite_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_sqlite_shutdown);
AGC_MODULE_DEFINITION(mod_sqlite, mod_sqlite_load, mod_sqlite_shutdown, NULL);

AGC_MODULE_LOAD_FUNCTION(mod_sqlite_load)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Sqlite init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_sqlite_shutdown)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Sqlite shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}