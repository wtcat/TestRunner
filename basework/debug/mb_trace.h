/*
 * Copyright 2022 wtcat
 */
#ifndef DEBUG_MB_TRACE_H_
#define DEBUG_MB_TRACE_H_

#include "basework/container/list.h"

#ifdef __cplusplus
extern "C"{
#endif

struct mb_tracer;

struct mb_node {
    struct hlist_node node;
    const void *ptr;
    size_t size;
    const char *sym;
};

int mb_tracer_add(struct mb_tracer *tracer, void *ptr, size_t size, 
    const char *sym);
int mb_tracer_del(struct mb_tracer *tracer, void *ptr);
void mb_tracer_visit(struct mb_tracer *tracer, bool (*visitor)(struct mb_node *));
struct mb_tracer *mb_tracer_create(size_t maxitems, void *(*alloc)(size_t), 
    void (*mfree)(void *));
void mb_tracer_destory(struct mb_tracer *tracer);

#ifdef __cplusplus
}
#endif
#endif /* DEBUG_MB_TRACE_H_ */
