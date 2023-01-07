/*
 * Copyright 2022 wtcat
 *
 * RTOS abstract layer
 */
#ifndef BASEWORK_OS_ZEPHYR_OS_BASE_H_
#define BASEWORK_OS_ZEPHYR_OS_BASE_H_

#include <assert.h>
#include <kernel.h>
#include <posix/pthread.h>

#ifdef __cplusplus
extern "C"{
#endif

#define os_in_isr() k_is_in_isr()
#define os_panic(...) k_panic()

/* Critical lock */
#define os_critical_global_declare
#define os_critical_declare int ___lock;
#define os_critical_lock   (___lock) = irq_lock();
#define os_critical_unlock irq_unlock(___lock);

/* */
#define os_resched_declare(_proc)  struct k_sem _proc;
#define os_resched_init(_proc)     k_sem_init(&(_proc), 0, 1)
#define os_resched_deinit(_proc)   
#define os_resched(_proc)          k_sem_take(&(_proc), K_FOREVER)
#define os_wake_up(_proc)          k_sem_give(&(_proc))

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_ZEPHYR_OS_BASE_H_ */
