/*
 * Copyright 2022 wtcat
 *
 * OS abstract layer
 */
#ifndef BASEWORK_OS_POSIX_OS_BASE_H_
#define BASEWORK_OS_POSIX_OS_BASE_H_

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "basework/generic.h"
#ifdef __cplusplus
extern "C"{
#endif

#define os_in_isr() false

#define os_panic(...) assert(0)

/* Critical lock */
#define os_critical_global_declare  static pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;
#define os_critical_declare
#define os_critical_lock   pthread_mutex_lock(&__mutex);
#define os_critical_unlock pthread_mutex_unlock(&__mutex);

/* */
#define os_resched_declare(_proc)  sem_t _proc;
#define os_resched_init(_proc)     sem_init(&(_proc), 0, 0)
#define os_resched_deinit(_proc)   sem_close(&(_proc)) 
#define os_resched(_proc)          sem_wait(&(_proc))
#define os_wake_up(_proc)          sem_post(&(_proc))

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_POSIX_OS_BASE_H_ */
