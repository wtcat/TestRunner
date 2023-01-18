/*
 * Copyright 2022 wtcat
 */
#ifndef BASEWORK_LIB_DISKLOG_H_
#define BASEWORK_LIB_DISKLOG_H_

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif

struct log_upload_class {
    void (*begin)(void *ctx);
    bool (*upload)(void *ctx, char *buf, size_t size);
    void (*end)(void *ctx, int err);
    void *ctx;
    size_t maxsize;
};

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
 * @maxsize: the maximum data size for output per times
 * return 0 if success
 */
int disklog_ouput(bool (*output)(void *ctx, char *buf, size_t size), 
    void *ctx, size_t maxsize);

/*
 * disklog_read - Read disk log
 * 
 * @buffer: log buffer
 * @maxlen: buffer size
 * @first: first read
 * return read size if success else less than 0
 */
ssize_t disklog_read(char *buffer, size_t maxlen, bool first);

/*
 * disklog_upload - log upload callback 
 * 
 * @luc: callback handle
 */
void disklog_upload_cb(struct log_upload_class *luc);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_LIB_DISKLOG_H_ */
