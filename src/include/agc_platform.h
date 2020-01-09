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

#if !defined(_STDINT) && !defined(uint32_t)
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned long in_addr_t;
#endif

#if (defined(__GNUC__) 
#define AGC_DECLARE(type)		__attribute__((visibility("default"))) type
#else
#define AGC_DECLARE(type)		type   
#endif

typedef uintptr_t agc_size_t;
typedef intptr_t agc_ssize_t;

#if UINTPTR_MAX == 0xffffffffffffffff
#define AGC_64BIT 1
#endif

#ifndef agc_assert
#define agc_assert(expr) assert(expr)
#endif

AGC_END_EXTERN_C
#endif