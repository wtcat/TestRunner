/*
 * Copyright 2022 wtcat
 *
 * The simple object allocator implement
 */
#define pr_fmt(fmt) "osapi_obj: "fmt
#include <errno.h>
#include <stddef.h>

#include "basework/os/osapi.h"
#include "basework/generic.h"
#include "basework/os/osapi_obj.h"
#include "basework/log.h"

#define OBJ_DEFAULT_VAL (void *)(~0ul)

/*
 * The object table must be initialized with default value
 */
static void *_os_object_table[OBJ_MAX_CLASS] = {
    [OBJ_FILE_CLASS]  = OBJ_DEFAULT_VAL,
    [OBJ_TIMER_CLASS] = OBJ_DEFAULT_VAL
};
os_critical_global_declare

bool os_obj_ready(enum obj_type type) {
    return _os_object_table[type] != OBJ_DEFAULT_VAL;
}

void *os_obj_allocate(enum obj_type type) {
    os_critical_declare
    
    os_critical_lock
    void *p = _os_object_table[type];
    if (p) 
        _os_object_table[type] = *(char **)p;
    os_critical_unlock
    return p;
}

void os_obj_free(enum obj_type type, void *p) {
    os_critical_declare

    os_critical_lock
    *(char **)p = _os_object_table[type];
    _os_object_table[type] = p;
    os_critical_unlock
}

int os_obj_initialize(enum obj_type type, void *buffer, size_t size, 
    size_t isize) {
    os_critical_declare
    size_t n, fixed_isize = roundup(isize, sizeof(void *));
    char *p, *head = NULL;

    if (_os_object_table[type] && os_obj_ready(type)) {
        pr_warn("The object has been initialized\n");
        return -EBUSY;
    }

    if (buffer == NULL)
        return -EINVAL;

    if (!fixed_isize || size < fixed_isize) {
        pr_warn("object buffer is too small\n");
        return -EINVAL;
    }

    os_critical_lock

    /*
     * We must be check object again, Maybe another task just has been 
     * initialized object
     */
    if (os_obj_ready(type)) {
        os_critical_unlock
        return 0;
    }

    for (p = buffer, n = size / fixed_isize; n > 0; n--) {
        *(char **)p = head;
        head = p;
        p = p + fixed_isize;
    }
   
    _os_object_table[type] = head;
    os_critical_unlock

    return 0;
}
