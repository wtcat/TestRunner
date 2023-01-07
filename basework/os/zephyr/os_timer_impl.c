/*
 * Copyright 2022 wtcat
 *
 * Timer implement for zephyr
 */
// #define CONFIG_LOGLEVEL LOGLEVEL_DEBUG
#define pr_fmt(fmt) "os_timer: "fmt

#include <errno.h>
#include <assert.h>

#include <zephyr.h>
#include <drivers/hrtimer.h>

#include "basework/os/osapi_config.h"
#include "basework/os/osapi_timer.h"
#include "basework/log.h"
#include "basework/rq.h"

#define HRTIMER_MS(n) (s32_t)((n) * 1000)

struct os_timer {
    struct hrtimer timer;
    void (*task)(os_timer_t, void *);
};

static struct os_timer timer_objs[CONFIG_OS_MAX_TIMERS] __unused;

static void timer_task_wrapper(void *arg) {
    struct os_timer *timer = arg;
    pr_dbg("timer task call: %p timer->task(%p) arg(%p)\n", timer, timer->task, arg);
    assert(timer->task != NULL);
    timer->task(timer, timer->timer.expiry_fn_arg);
}

static void __unused timeout_adaptor(os_timer_t timer, void *arg) {
    pr_dbg("timer submit: %p arg(%p)\n", timer, arg);
    rq_submit(timer_task_wrapper, timer);
}

int os_timer_create(os_timer_t *timer, 
    void (*timeout_cb)(os_timer_t, void *), 
    void *arg, 
    bool isr_context) {
    struct os_timer *p;

    if (!timer || !timeout_cb)
        return -EINVAL;

    if (!os_obj_ready(OBJ_TIMER_CLASS)) {
        int err = os_obj_initialize(OBJ_TIMER_CLASS, timer_objs, 
            sizeof(timer_objs), sizeof(timer_objs[0]));
        if (err) {
            pr_err("Initialize timer object failed (%d)\n", err);
            os_panic();
            return err;
        }
    }

    p = os_obj_allocate(OBJ_TIMER_CLASS);
    if (!p) {
        pr_err("Allocate timer object failed\n");
        return -ENOMEM;
    }

    *p = (struct os_timer){ 0 };
    if (!isr_context) {
        p->task = timeout_cb;
        timeout_cb = timeout_adaptor;
    }

    pr_dbg("timer create: %p timer->task(%p) arg(%p)\n", p, p->task, arg);
    hrtimer_init(&p->timer, (hrtimer_expiry_t)timeout_cb, arg);
    *timer = p;
    return 0;
}

int os_timer_mod(os_timer_t timer, long expires) {
    assert(timer != NULL);
    struct os_timer *p = timer;
    hrtimer_start(&p->timer, HRTIMER_MS(expires), 0);
    return 0;
}

int os_timer_add(os_timer_t timer, long expires) {
    assert(timer != NULL);
    struct os_timer *p = timer;
    hrtimer_start(&p->timer, HRTIMER_MS(expires), 0);
    return 0;
}

int os_timer_del(os_timer_t timer) {
    assert(timer != NULL);
    struct os_timer *p = timer;
    hrtimer_stop(&p->timer);
    return 0;
}

int os_timer_destroy(os_timer_t timer) {
    assert(timer != NULL);
    struct os_timer *p = timer;
    hrtimer_stop(&p->timer);
    os_obj_free(OBJ_TIMER_CLASS, p);
    return 0;
}
