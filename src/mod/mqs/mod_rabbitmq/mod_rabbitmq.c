#include <agc.h>

AGC_MODULE_LOAD_FUNCTION(mod_rabbitmq_load);
AGC_MODULE_SHUTDOWN_FUNCTION(mod_rabbitmq_shutdown);
AGC_MODULE_DEFINITION(mod_rabbitmq, mod_rabbitmq_load, mod_rabbitmq_shutdown, NULL);

AGC_MODULE_LOAD_FUNCTION(mod_rabbitmq_load)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Rabbitmq init success.\n");
    return AGC_STATUS_SUCCESS;
}

AGC_MODULE_SHUTDOWN_FUNCTION(mod_rabbitmq_shutdown)
{
    agc_log_printf(AGC_LOG, AGC_LOG_INFO, "Rabbitmq shutdown success.\n");
    return AGC_STATUS_SUCCESS;
}