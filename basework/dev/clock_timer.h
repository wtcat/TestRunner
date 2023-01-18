/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_CLOCK_TIMER_H_
#define BASEWORK_CLOCK_TIMER_H_

#include "basework/container/list.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Timeout time
 */
#define CLK_SEC_TIMEOUT   1  
#define CLK_MIN_TIMEOUT  (60 * CLK_SEC_TIMEOUT)
#define CLK_HOUR_TIMEOUT (60 * CLK_MIN_TIMEOUT)
#define CLK_DAY_TIMEOUT  (24 * CLK_HOUR_TIMEOUT) 

 struct clock_callout {
    struct list_head node;
    void (*callout)(void); /* if need restart return true */
#define CALLOUT_RESTART true
#define CALLOUT_STOP false

    unsigned long i_expires;
    unsigned long expires;
};

/*
 * clock_callout_add - Register clock timeout callback 
 * 
 * @cc: callout pointer
 * @callout: timeout callback
 * @seconds: the timeout time with seconds
 * return 0 if success
 */
int clock_callout_add(struct clock_callout *cc, void (*callout)(void), 
    unsigned int seconds);

/*
 * clock_callout_del - Remove clock timeout 
 *
 * @cc: callout pointer
 * return 0 if success
 */
int clock_callout_del(struct clock_callout *cc);

/*
 * clock_timer_init - Clock timer intialize
 *
 * This routine must be called before all clock and time interface function 
 */
int clock_timer_init(void);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_CLOCK_TIMER_H_ */
