/*
 * Copyright 2022 wtcat
 */
#include "basework/os/osapi.h"
#include "basework/container/list.h"
#include "basework/dev/clock_timer.h"
#include "basework/dev/clock.h"

#define CLOCK_PERIOD 1000 /* unit: ms */

// typedef long vtime_t;

os_critical_global_declare
static LIST_HEAD(callout_list);
static os_timer_t clock_timer;
static volatile vtime_t clock_counter;

/*
 * Merge two list to one
 */
static inline bool list_merge(struct list_head *list,
	struct list_head *head) {
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
        INIT_LIST_HEAD(list);
        return true;
    }
    return false;
}

/*
 * Clock timeout callback
 */
static void clock_timeout_cb(os_timer_t timer, void *arg) {
    (void) arg;
    os_critical_declare
    struct list_head head = LIST_HEAD_INIT(head);
    struct list_head *p, *n;
    vtime_t tick;
    bool merged;

    tick = clock_counter;
    tick++;
    clock_counter = tick;

    /*
     * When walk around list, We must be reduce critical time to 
     * improve performance and so use a temp-list to do this.
     */
    os_critical_lock
    merged = list_merge(&callout_list, &head);
    os_critical_unlock

    list_for_each_safe(p, n, &head) {
        struct  clock_callout *co = container_of(p, struct clock_callout, node);
        if (co->expires <= (unsigned long)tick) {
            co->callout();
            co->expires = tick + co->i_expires;
        }
    }
    if (merged) {
        os_critical_lock
        list_splice(&head, &callout_list);
        os_critical_unlock
    }
	
    /* Repeat timer */
    os_timer_mod(timer, CLOCK_PERIOD);
}

static int clock_timer_get_time(struct utime *tv) {
    /*
     * We must make sure the counter can't not be modified 
     * when read it.
     */
    tv->tv_nsec = 0;
    do {
        tv->tv_sec = clock_counter;
    } while (tv->tv_sec != clock_counter);

    return 0;
}

static int clock_timer_set_time(const struct utime *tv) {
    os_critical_declare

    os_critical_lock
    clock_counter = tv->tv_sec;
    os_critical_unlock
    return 0;
}

static const struct clock_timer clock_timer_ops = {
    .get_time = clock_timer_get_time,
    .set_time = clock_timer_set_time
};

static bool clock_callout_find(struct clock_callout *cc, bool del) {
    os_critical_declare
    struct list_head *p, *n;

    os_critical_lock
    list_for_each_safe(p, n, &callout_list) {
        struct  clock_callout *co = container_of(p, struct clock_callout, node);
        if (cc == co) {
            if (del)
                list_del(&co->node);
            os_critical_unlock
            return true;
        }
    }
    os_critical_unlock
    return false;
}

int clock_callout_add(struct clock_callout *cc, void (*callout)(void), 
    unsigned int seconds) {
    os_critical_declare

    if (cc == NULL || callout == NULL)
        return -EINVAL;

    if (!seconds)
        return -EINVAL;

    if (clock_callout_find(cc, false))
        return -EEXIST;

    cc->callout = callout;
    cc->i_expires = seconds;

    os_critical_lock
    cc->expires = clock_counter + seconds;
    list_add_tail(&cc->node, &callout_list);
    os_critical_unlock

    return 0;
}

int clock_callout_del(struct clock_callout *cc) {
    if (cc == NULL)
        return -EINVAL;

    if (clock_callout_find(cc, true)) 
        return 0;
    
    return -EFAULT;
}

/*
 * Create clock timer and register it
 */
int clock_timer_init(void) {
    int err;

    err = os_timer_create(&clock_timer, clock_timeout_cb, 
        NULL, false);
    if (err)
        return err;

    err = clock_register(&clock_timer_ops);
    if (err) {
        os_timer_destroy(clock_timer);
        return err;
    }
    
    (void) os_timer_add(clock_timer, CLOCK_PERIOD);
    return 0;
}
