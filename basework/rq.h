/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_RQ_H_
#define BASEWORK_RQ_H_

#include "basework/os/osapi.h"
#include "basework/container/list.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*rq_exec_t)(void *);

struct rq_context {
    struct list_head frees;   /* Free list */
    struct list_head pending; /* Pending list */
    os_resched_declare(proc)
    void *buffer;
    uint16_t nq;
    pthread_t thread;
};

struct rq_node {
    struct list_head node;
    rq_exec_t exec; /* User function */
    void *arg;
};

#define RQ_BUFFER_SIZE(_nitem, _itemsize) \
    ((_nitem) * roundup(sizeof(struct rq_node) + (_itemsize), 4))

#define RQ_BUFFER_DEFINE(_name, _nitem) \
    char _name[RQ_BUFFER_SIZE(_nitem, sizeof(struct rq_node))] __aligned(4); \

/*
 * RQ_DEFINE - Define a runqueue context
 *
 * @_name: Object name
 * @_nitem: The maximum length of queue
 * @_itemsize: The maximum size of queue-item
 */
#define RQ_DEFINE(_name, _nitem) \
    static RQ_BUFFER_DEFINE(_name ## _buffer, _nitem); \
    struct rq_context _name __used = {    \
        .buffer = _name ## _buffer,   \
        .nq = _nitem,  \
    }

extern struct rq_context *_system_rq;

/*
 * _rq_static_init - Initialze run-queue context that defined by macro 'RQ_DEFINE()'
 *
 * @rq: run-queue context
 */
void _rq_static_init(struct rq_context *rq);

/*
 * _rq_init - Initialze run-queue context that defined by user
 *
 * @rq: Run-queue context
 * @buffer: Queue buffer
 * @size: The size of queue buffer
 * return 0 if success
 */
int _rq_init(struct rq_context *rq, void *buffer, size_t size);
    
/*
 * _rq_submit - Submit a user-work to run-queue
 *
 * @rq: Run-queue context
 * @exec: user function
 * @data: user parameter
 * @prepend: urgent call
 * return 0 if success
 */
int _rq_submit(struct rq_context *rq, void (*exec)(void *), void *arg, 
    bool prepend);

/*
 * _rq_submit_with_copy - Submit a work and copy buffer
 *
 * @rq: Run-queue context
 * @exec: user function
 * @data: buffer address
 * @size: buffer size
 * @prepend: urgent call
 * return 0 if success
 */
int _rq_submit_with_copy(struct rq_context *rq, void (*exec)(void *, size_t), 
    void *data, size_t size, bool prepend);

/*
 * _rq_reset - Reset run-queue
 *
 * @rq: Run-queue context
 */
void _rq_reset(struct rq_context *rq);

/*
 * _rq_schedule - Schedule run-queue (Asynchronous execute user function).
 * This should be called in a task context
 *
 * @rq: Run-queue context
 */
void _rq_schedule(struct rq_context *rq);

/*
 * _rq_new_thread - Create a new run-queue
 *
 * @rq: run queue context
 * @stack: thread stack address
 * @size: thrad stack size
 * @posix_prio: the priority of posix thread 
 * return 0 if success
 */
int _rq_new_thread(struct rq_context *rq, void *stack, size_t size, int posix_prio);

/*
 * _rq_cancel_thread - Aboart a run-queue thread
 *
 * @rq: run queue context
 * return 0 if success
 */
int _rq_cancel_thread(struct rq_context *rq);

/*
 * Default run-queue public interface
 */
static inline int rq_init(struct rq_context *rq) {
    if (!rq || !rq->buffer || !rq->nq)
        return -EINVAL;
    _rq_static_init(rq);
    if (!_system_rq)
        _system_rq = rq;
    return 0;
}

/*
 * Submit a user work to run-queue and pass argument by value.
 * That is mean to the data will be copy
 */
static inline int rq_submit_cp(void (*exec)(void *, size_t), void *data, 
    size_t size) {
    return _rq_submit_with_copy(_system_rq, exec, data, size, false);
}

static inline int rq_submit_urgent_cp(void (*exec)(void *, size_t), void *data, 
    size_t size) {
    return _rq_submit_with_copy(_system_rq, exec, data, size, true);
}

/*
 * Submit a user work to run-queue and pass argument by pointer
 */
static inline int rq_submit(void (*exec)(void *), void *arg) {
    return _rq_submit(_system_rq, exec, arg, false);
}

static inline int rq_submit_urgent(void (*exec)(void *), void *arg) {
    return _rq_submit(_system_rq, exec, arg, true);
}

static inline void rq_schedule(void) {
    _rq_schedule(_system_rq);
}

static inline void rq_reset(void) {
    _rq_reset(_system_rq);
}

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_RQ_H_ */
