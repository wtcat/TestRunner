/*
 * Copyright 2022 wtcat
 */
#include <errno.h>
#include <string.h>

#include "basework/hashtbl.h"
#include "basework/debug/mb_trace.h"

struct mb_tracer {
    DECLARE_HASHTABLE(hashtbl, 8);
    void *(*alloc)(size_t);
    void (*free)(void *);
    char *freelist;
    size_t size;
    struct mb_node freends[0];
};

/*
 * mb_node_alloc - allocate a new <struct mb_node>
 * @tracer: tracer object
 */
static struct mb_node *mb_node_alloc(struct mb_tracer *tracer) {
    char *newnode = tracer->freelist;
    if (newnode) 
        tracer->freelist = *(char **)newnode;
    return (struct mb_node *)newnode;
}

/*
 * mb_node_alloc - free node<struct mb_node>
 * @tracer: tracer object
 * @node: record node
 */
static void mb_node_free(struct mb_tracer *tracer, struct mb_node *node) {
    char *p = (char *)node;
    *(char **)p = tracer->freelist;
    tracer->freelist = p;
}

/*
 * mb_tracer_add - add a node to tracer that record memory informations
 * @tracer: tracer object
 * @ptr: memory address
 * @size: memory size
 * @sym: symbol information
 * return 0 if success
 */
int mb_tracer_add(struct mb_tracer *tracer, void *ptr, size_t size, 
    const char *sym) {
    struct mb_node *newn;
    newn = mb_node_alloc(tracer);
    if (newn) {
        newn->ptr = ptr;
        newn->size = size;
        newn->sym = sym;
        hash_add(tracer->hashtbl, &newn->node, (uintptr_t)ptr);
        return 0;
    }
    return -ENOMEM;
}

/*
 * mb_tracer_del - delete the node associated with ptr
 * @tracer: tracer object
 * @ptr: memory address
 * return 0 if success
 */
int mb_tracer_del(struct mb_tracer *tracer, void *ptr) {
    struct mb_node *pos;
    hash_for_each_possible(tracer->hashtbl, pos, node, (uintptr_t)ptr) {
        if (pos->ptr == ptr) {
            hash_del(&pos->node);
            mb_node_free(tracer, pos);
            return 0;
        }
    }
    return -ENODATA;
}

/*
 * mb_tracer_visit - iterate tracer class
 * @tracer: tracer object
 * @visitor: iterator callback
 */
void mb_tracer_visit(struct mb_tracer *tracer, bool (*visitor)(struct mb_node *)) {
    struct mb_node *pos;
    int bkt;

    if (!tracer || !visitor)
        return;
    hash_for_each(tracer->hashtbl, bkt, pos, node) {
        if (!visitor(pos))
            break;
    }
}

struct mb_tracer *mb_tracer_create(size_t maxitems, void *(*alloc)(size_t), 
    void (*mfree)(void *)) {
    struct mb_tracer *tracer;
    size_t alloc_size;
    char *ptr;

    if (!maxitems || !alloc || !mfree) {
        errno = -EINVAL;
        return NULL;
    }

    alloc_size = sizeof(*tracer) + sizeof(struct mb_node) * maxitems;
    tracer = alloc(alloc_size);
    if (!tracer) {
        errno = -ENOMEM;
        return NULL;
    }

    /* Initialize free-node list */
    memset(tracer, 0, alloc_size);
    ptr = (char *)tracer->freends;
    for (size_t i = 0; i < maxitems; i++) {
        *(char **)ptr = tracer->freelist;
        tracer->freelist = ptr;
        ptr += sizeof(struct mb_node);
    }

    hash_init(tracer->hashtbl);
    tracer->free = mfree;
    tracer->size = maxitems;
    return tracer;
}

void mb_tracer_destory(struct mb_tracer *tracer) {
    if (tracer) 
        tracer->free(tracer);
}
