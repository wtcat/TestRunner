/*
 * Copyright 2022 wtcat
 *
 * Timer API for OS
 */
#ifndef BASEWORK_OS_TIMER_H_
#define BASEWORK_OS_TIMER_H_

#include <stdbool.h>

#include "basework/os/osapi_config.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void* os_timer_t;

#ifndef CONFIG_OS_TIMER_DISABLE
/*
 * timer_init - Initialize timer object
 * @timer: timer pointer
 * @expired_fn: timeout callback function
 * @arg: callback argument
 * @isr_context: execute with interrupt context
 * return 0 if success
 */
int os_timer_create(os_timer_t *timer, 
    void (*expired_fn)(os_timer_t, void *), void *arg, bool isr_context);

/*
 * timer_mod - modify timer expiration time and re-add to timer list
 * @timer: timer pointer
 * @expires: expiration time (unit: ms)
 * 
 * return 0 if success
 */
int os_timer_mod(os_timer_t timer, long expires);

/*
 * timer_add - add a new timer to timer list
 * @timer: timer pointer
 * @expires: expiration time (unit: ms)
 * 
 * return 0 if success
 */
int os_timer_add(os_timer_t timer, long expires);

/*
 * timer_del - delete a timer from timer list
 * @timer: timer pointer
 * 
 * return 1 if timer has pending
 */
int os_timer_del(os_timer_t timer);

/*
 * timer_del - delete a timer from timer list and free it
 * @timer: timer pointer
 * 
 * return 0 if success
 */
int os_timer_destroy(os_timer_t timer);

#else /* CONFIG_OS_TIMER_DISABLE */

static inline int os_timer_create(os_timer_t *timer, 
    void (*expired_fn)(os_timer_t, void *), void *arg, bool isr_context) {
    return -1;
}

static inline int os_timer_mod(os_timer_t timer, long expires) {
    return -1;
}

static inline int os_timer_add(os_timer_t timer, long expires) {
    return -1;
}

static inline int os_timer_del(os_timer_t timer, long expires) {
    return -1;
}

static inline int os_timer_destroy(os_timer_t timer, long expires) {
    return -1;
}

#endif /* !CONFIG_OS_TIMER_DISABLE */

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_TIMER_H_ */

