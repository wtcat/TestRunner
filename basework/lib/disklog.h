/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_LIB_DISKLOG_H_
#define BASEWORK_LIB_DISKLOG_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * disklog_init - Initialize disk log module
 * return 0 if success
 */
int disklog_init(void);

/*
 * disklog_reset - Reset disk log(clear all)
 * return 0 if success
 */
void disklog_reset(void);

/*
 * disklog_input - Write log to disk
 * 
 * @buf: log buffer pointer
 * @size: log size
 * return 0 if success
 */
int disklog_input(const char *buf, size_t size);

/*
 * disklog_ouput - Read disk log and pass to user callback
 * 
 * @output: log output callback
 * @ctx: user parameter
 * return 0 if success
 */
int disklog_ouput(void (*output)(void *ctx, char *buf, size_t size), void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_LIB_DISKLOG_H_ */
