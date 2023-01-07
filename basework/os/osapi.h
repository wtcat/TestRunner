/*
 * Copyright 2022 wtcat
 *
 * RTOS abstract layer
 */

#ifndef BASEWORK_OS_OSAPI_H_
#define BASEWORK_OS_OSAPI_H_

#include "os_base_impl.h"
#include "basework/os/osapi_timer.h"
#include "basework/os/osapi_fs.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifndef os_panic
#define os_panic(...) for(;;)
#endif

#ifndef os_in_isr
#define os_in_isr() 0
#endif

/* Critical lock */
#ifndef os_critical_global_declare
#define os_critical_global_declare
#endif
#ifndef os_critical_declare
#define os_critical_declare
#endif
#ifndef os_critical_lock
#define os_critical_lock
#endif
#ifndef os_critical_unlock
#define os_critical_unlock
#endif

/* */
#ifndef os_resched_declare
#define os_resched_declare(_proc)  
#endif
#ifndef os_resched_init
#define os_resched_init(_proc)     (void)(_proc)
#endif
#ifndef os_resched_deinit
#define os_resched_deinit(_proc)   (void)(_proc)
#endif
#ifndef os_resched
#define os_resched(_proc)          (void)(_proc)
#endif
#ifndef os_wake_up
#define os_wake_up(_proc)          (void)(_proc)
#endif

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_OS_OSAPI_H_ */
