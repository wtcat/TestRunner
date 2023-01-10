/*
 * Copyright 2022 wtcat
 * 
 * The simple async-queue implement (rq: run-queue)
 */
#define pr_fmt(fmt) "runqueue: "fmt

#include <errno.h>
#include <assert.h>
#include <string.h>

#include "basework/rq.h"
#include "basework/container/list.h"
#include "basework/malloc.h"
#include "basework/log.h"

#ifdef _WIN32
#pragma comment(lib,"pthreadVC2.lib")
#endif

struct buffer_hdr {
    void (*cb)(void *arg, size_t);
    size_t len;
    char data[];
};

#define BUFFER_HDR_SIZE(sz) ((sz) + sizeof(struct buffer_hdr))

os_critical_global_declare
struct rq_context *_system_rq;

static void exec_adaptor_cb(void *arg) {
    struct buffer_hdr *bh = (struct buffer_hdr *)arg;
    bh->cb(bh->data, bh->len);
    pr_dbg("free buffer: %p\n", bh);
    general_free(bh);
}

void _rq_static_init(struct rq_context *rq) {
    struct rq_node *p = (struct rq_node *)rq->buffer;

    INIT_LIST_HEAD(&rq->frees);
    INIT_LIST_HEAD(&rq->pending);
    os_resched_init(rq->proc);
    for (int i = 0; i < rq->nq; i++) {
        list_add_tail(&p->node, &rq->frees);
        p = (struct rq_node *)((char *)p + sizeof(struct rq_node));
    }
}

int _rq_init(struct rq_context *rq, void *buffer, size_t size) {
    if (!rq || !buffer || ((uintptr_t)buffer & 3))
        return -EINVAL;

    if (size < sizeof(struct rq_node))
        return -EINVAL;

    rq->nq = size / sizeof(struct rq_node);
    rq->buffer = buffer;
    _rq_static_init(rq);
    return 0;
}

static inline void rq_put_locked(struct rq_context *rq, 
    struct rq_node *rn) {
    list_add(&rn->node, &rq->frees);
}

static struct rq_node *rq_get_first_locked(struct list_head *head) {
    struct rq_node *rn;
    if (!list_empty(head)) {
        rn = container_of(head->next, struct rq_node, node);
        list_del(&rn->node);
        return rn;
    }
    return NULL;
}

static void rq_add_node_locked(struct rq_context *rq, struct rq_node *rn, 
    bool prepend) {
    int need_wakeup;
    /*
     * When the pending list is empty that the run-queue is sleep,
     * So we need to wake up it at later.
     */
    need_wakeup = list_empty(&rq->pending);

    if (prepend)
        list_add(&rn->node, &rq->pending);
    else
        list_add_tail(&rn->node, &rq->pending);

    /* We will wake up the schedule queue if necessary */
    if (need_wakeup)
        os_wake_up(rq->proc);
} 

int _rq_submit_with_copy(struct rq_context *rq, void (*exec)(void *, size_t), 
    void *data, size_t size, bool prepend) {
    assert(rq != NULL);
    assert(exec != NULL);
    os_critical_declare
    struct buffer_hdr *bh;
    struct rq_node *rn;

    pr_dbg("allocate %d bytes\n", BUFFER_HDR_SIZE(size));
    bh = general_malloc(BUFFER_HDR_SIZE(size));
    if (unlikely(!bh)) {
        pr_err("No more memory\n");
        os_panic();
    }
    pr_dbg("allocated buffer: %p\n", bh);
    os_critical_lock

    /* Allocate a new rq-node from free list */
    rn = rq_get_first_locked(&rq->frees);
    if (!rn) {
        os_critical_unlock
        general_free(bh);
        return -EBUSY;
    }
    os_critical_unlock

    memcpy(bh->data, data, size);
    bh->cb = exec;
    bh->len = size;
    rn->exec = exec_adaptor_cb;
    rn->arg = bh;

    os_critical_lock    
    rq_add_node_locked(rq, rn, prepend); 
    os_critical_unlock
    return 0;
}

int _rq_submit(struct rq_context *rq, void (*exec)(void *), void *arg, 
    bool prepend) {
    assert(rq != NULL);
    assert(exec != NULL);
    os_critical_declare
    struct rq_node *rn;

    os_critical_lock

    /* Allocate a new rq-node from free list */
    rn = rq_get_first_locked(&rq->frees);
    if (!rn) {
        os_critical_unlock
        return -EBUSY;
    }

    rn->exec = exec;
    rn->arg = arg;
    rq_add_node_locked(rq, rn, prepend); 
    os_critical_unlock

    return 0;
}

void _rq_reset(struct rq_context *rq) {
    os_critical_declare

    os_critical_lock
    /*
     * Move pending list to free list
     */
    list_splice_tail_init(&rq->pending, &rq->frees);
    os_critical_unlock
}

void _rq_schedule(struct rq_context *rq) {
    os_critical_declare
    struct rq_node *rn;
    rq_exec_t fn;

    os_critical_lock

    for ( ; ; ) {
        /* Get the first rq-node from pending list */
        rn = rq_get_first_locked(&rq->pending);
        if (unlikely(!rn)) {
            os_critical_unlock
            /*
             * Now, The queue is empty and we have no more work to do, So we
             * trigger task schedule and switch to other task.
             */
            os_resched(rq->proc);
            continue;
        }
        fn = rn->exec;
        os_critical_unlock

        /* Executing user function */ 
        fn(rn->arg);
        
        os_critical_lock
        
        /* Free rn-node to free list */
        rq_put_locked(rq, rn);
    }
    os_critical_unlock
}

/*
 * The service thread of run-queue
 */
static void *rq_main_thread(void *arg) {
    struct rq_context *rq = arg;
    
    _rq_schedule(rq);

    pthread_exit(NULL);
    return NULL;
}

int _rq_new_thread(struct rq_context *rq, void *stack, size_t size, int posix_prio) {
    struct sched_param param;
    pthread_attr_t attr;

    if (!rq || !rq->buffer || !rq->nq)
        return -EINVAL;

    rq_init(rq);
    param.sched_priority = posix_prio;
    pthread_attr_init(&attr);
#ifndef _WIN32
    if (stack)
        pthread_attr_setstack(&attr, stack, size);
#endif
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);

    return pthread_create(&rq->thread, &attr, rq_main_thread, rq);
}

int _rq_cancel_thread(struct rq_context *rq) {
#ifndef _WIN32
    if (!rq->thread)
        return -EINVAL;
#endif
    pthread_cancel(rq->thread);
    return 0;
}
