#include <agc.h>
#include <stdio.h>
#include "private/agc_core_pvt.h"

AGC_DECLARE(void) agc_cond_next(void)
{
    apr_sleep(1000);
}
