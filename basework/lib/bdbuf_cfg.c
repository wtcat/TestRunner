/*
 * Copyright 2022 wtcat
 */
#include "basework/lib/bdbuf.h"

#ifndef CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS
  #define CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS \
    RTEMS_BDBUF_MAX_READ_AHEAD_BLOCKS_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_MAX_WRITE_BLOCKS
  #define CONFIGURE_BDBUF_MAX_WRITE_BLOCKS \
    RTEMS_BDBUF_MAX_WRITE_BLOCKS_DEFAULT
#endif

#ifndef CONFIGURE_SWAPOUT_TASK_PRIORITY
  #define CONFIGURE_SWAPOUT_TASK_PRIORITY \
    RTEMS_BDBUF_SWAPOUT_TASK_PRIORITY_DEFAULT
#endif

#ifndef CONFIGURE_SWAPOUT_SWAP_PERIOD
  #define CONFIGURE_SWAPOUT_SWAP_PERIOD \
    RTEMS_BDBUF_SWAPOUT_TASK_SWAP_PERIOD_DEFAULT
#endif

#ifndef CONFIGURE_SWAPOUT_BLOCK_HOLD
  #define CONFIGURE_SWAPOUT_BLOCK_HOLD \
    RTEMS_BDBUF_SWAPOUT_TASK_BLOCK_HOLD_DEFAULT
#endif

#ifndef CONFIGURE_SWAPOUT_WORKER_TASKS
  #define CONFIGURE_SWAPOUT_WORKER_TASKS \
    RTEMS_BDBUF_SWAPOUT_WORKER_TASKS_DEFAULT
#endif

#ifndef CONFIGURE_SWAPOUT_WORKER_TASK_PRIORITY
  #define CONFIGURE_SWAPOUT_WORKER_TASK_PRIORITY \
    RTEMS_BDBUF_SWAPOUT_WORKER_TASK_PRIORITY_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_TASK_STACK_SIZE
  #define CONFIGURE_BDBUF_TASK_STACK_SIZE \
    RTEMS_BDBUF_TASK_STACK_SIZE_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_CACHE_MEMORY_SIZE
  #define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE \
    RTEMS_BDBUF_CACHE_MEMORY_SIZE_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_BUFFER_MIN_SIZE
  #define CONFIGURE_BDBUF_BUFFER_MIN_SIZE \
    RTEMS_BDBUF_BUFFER_MIN_SIZE_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_BUFFER_MAX_SIZE
  #define CONFIGURE_BDBUF_BUFFER_MAX_SIZE \
    RTEMS_BDBUF_BUFFER_MAX_SIZE_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_READ_AHEAD_TASK_PRIORITY
  #define CONFIGURE_BDBUF_READ_AHEAD_TASK_PRIORITY \
    RTEMS_BDBUF_READ_AHEAD_TASK_PRIORITY_DEFAULT
#endif

#ifndef CONFIGURE_BDBUF_CACHELINE_SIZE
#define CONFIGURE_BDBUF_CACHELINE_SIZE 32
#endif

const rtems_bdbuf_config rtems_bdbuf_configuration = {
  CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS,
  CONFIGURE_BDBUF_MAX_WRITE_BLOCKS,
  CONFIGURE_SWAPOUT_TASK_PRIORITY,
  CONFIGURE_SWAPOUT_SWAP_PERIOD,
  CONFIGURE_SWAPOUT_BLOCK_HOLD,
  CONFIGURE_SWAPOUT_WORKER_TASKS,
  CONFIGURE_SWAPOUT_WORKER_TASK_PRIORITY,
  CONFIGURE_BDBUF_TASK_STACK_SIZE,
  CONFIGURE_BDBUF_CACHE_MEMORY_SIZE,
  CONFIGURE_BDBUF_BUFFER_MIN_SIZE,
  CONFIGURE_BDBUF_BUFFER_MAX_SIZE,
  CONFIGURE_BDBUF_READ_AHEAD_TASK_PRIORITY,
  CONFIGURE_BDBUF_CACHELINE_SIZE
};

