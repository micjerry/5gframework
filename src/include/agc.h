#ifndef AGC_H
#define AGC_H

#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include <setjmp.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/types.h>
#include <errno.h>

#include "agc_apr.h"
#include "agc_platform.h"
#include "agc_types.h"
#include "agc_utils.h"


#ifdef __cplusplus
#define AGC_BEGIN_EXTERN_C       extern "C" {
#define AGC_END_EXTERN_C         }
#else
#define AGC_BEGIN_EXTERN_C
#define AGC_END_EXTERN_C
#endif

#endif

