#ifndef AGC_PLATFORM_H
#define AGC_PLATFORM_H

AGC_BEGIN_EXTERN_C

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#if defined(__GNUC__) 
#define AGC_DECLARE(type)		__attribute__((visibility("default"))) type
#else
#define AGC_DECLARE(type)		type   
#endif

typedef uintptr_t agc_size_t;
typedef intptr_t agc_ssize_t;

typedef int32_t agc_int32_t;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#if UINTPTR_MAX == 0xffffffffffffffff
#define AGC_64BIT 1
#endif

#ifndef agc_assert
#define agc_assert(expr) assert(expr)
#endif

#if __GNUC__ >= 3
#define PRINTF_FUNCTION(fmtstr,vars) __attribute__((format(printf,fmtstr,vars)))
#else
#define PRINTF_FUNCTION(fmtstr,vars)
#endif

#define __AGC_FUNC__ (const char *)__func__

AGC_END_EXTERN_C

#endif
