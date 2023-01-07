/*
 * Copyright 2022 wtcat
 *
 */
#ifndef BASEWORK_OS_OBJ_H_
#define BASEWORK_OS_OBJ_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

enum obj_type {
    OBJ_FILE_CLASS,  /* file object */
    OBJ_TIMER_CLASS, /* timer object */
    OBJ_MAX_CLASS
};

struct obj_resource {
    void *start;
    size_t size;
};

#define OBJ_RES_INIT(a) {a, sizeof(a)}

/*
 * os_obj_ready - Determine if the object has been initialized
 * type: object type
 *
 * return true is ok
 */
bool os_obj_ready(enum obj_type type);

/*
 * os_obj_allocate - Allocate a new object
 * type: object type
 *
 * return object pointer if success 
 */
void *os_obj_allocate(enum obj_type type);

/*
 * os_obj_free - Free a object
 * type: object type
 *
 */
void os_obj_free(enum obj_type type, void *p);

/*
 * os_obj_initialize - Initialize object table
 * type: object type
 * buffer: start address
 * size: buffer size
 * isize: the size of single object
 *
 * return 0 if success
 */
int os_obj_initialize(enum obj_type type, void *buffer, size_t size, 
    size_t isize);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_OBJ_H_ */
