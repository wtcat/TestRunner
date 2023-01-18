/*
 * Copyright 2022 wtcat
 */
#ifndef BASE_GENERIC_H_
#define BASE_GENERIC_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if __cplusplus >= 201103L
  #define STATIC_ASSERT( _cond, _msg ) static_assert( _cond, # _msg )
#elif __STDC_VERSION__ >= 201112L
  #define STATIC_ASSERT( _cond, _msg ) _Static_assert( _cond, # _msg )
#else
  #define STATIC_ASSERT( _cond, _msg ) \
    struct _static_assert ## _msg \
      { int _static_assert ## _msg : ( _cond ) ? 1 : -1; }
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define DIV_ROUND_UP(x, y)  (((x) + ((y) - 1)) / (y))

#define roundup(x, y)  ((((x) + ((y) - 1)) / (y)) * (y))
#define powerof2(x)    ((((x) - 1) & (x)) == 0)
/*
 * Note: DISABLE_BRANCH_PROFILING can be used by special lowlevel code
 * to disable branch tracing on a per file basis.
 */
#if defined(__GNUC__) || defined(__clang__)
#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef __section
#define __section(_x)   __attribute__((section(_x)))
#endif

#ifndef __used
#define __used __attribute__((used))
#endif

#ifndef __unused
#define __unused __attribute__((used))
#endif

#ifndef __packed
#define __packed        __attribute__((packed))
#endif

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#ifndef __constructor
#define __constructor(_x) __attribute__((constructor(_x)))
#endif

#ifndef __printf_like
#define __printf_like(f, a)   __attribute__((format (printf, f, a)))
#endif

#else /* */
#ifndef likely
#define likely(x)    (x)
#endif
#ifndef unlikely
#define unlikely(x)  (x)
#endif

#ifndef __section
#define __section(_x)
#endif

#ifndef __used
#define __used
#endif

#ifndef __packed
#define __packed
#endif

#ifndef __aligned
#define __aligned(x)
#endif

#ifndef __printf_like
#define __printf_like(f, a)
#endif
#endif /* __GNUC__ || __clang__ */

#ifndef container_of
#define container_of(_m, _type, _member) \
	( (_type *) ( (uintptr_t) ( _m ) - offsetof( _type, _member ) ) )
#endif
	
#ifndef __cplusplus
#include "basework/minmax.h"
#endif
#endif /* BASE_GENERIC_H_ */
