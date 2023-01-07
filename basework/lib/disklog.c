/*
 * Copyright 2022 wtcat
 */
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#ifdef __ZEPHYR__
#include <posix/pthread.h>
#else
#include <pthread.h>
#endif

#include "basework/dev/partition.h"
#include "basework/lib/printer.h"
#include "basework/log.h"
#include "basework/minmax.h"
#include "basework/malloc.h"

struct disk_log {
    uint32_t magic;
#define DISKLOG_MAGIC 0xabdcebcf
    uint32_t start;
    uint32_t end;
    uint32_t size;
    uint32_t wr_ofs;
    uint32_t rd_ofs;
    uint32_t d_size;
};

#ifndef MIN
#define MIN(a, b) ((a) < (b)? (a): (b))
#endif

#define MTX_LOCK()   pthread_mutex_lock(&log_mtx)
#define MTX_UNLOCK() pthread_mutex_unlock(&log_mtx)
#define MTX_INIT()   pthread_mutex_init(&log_mtx, NULL)
#define MTX_UNINIT() pthread_mutex_destroy(&log_mtx)

static pthread_mutex_t log_mtx;
static struct disk_log log_file;
static const struct disk_partition *dp_dev;

static void logfile_save(void) {
    assert(dp_dev != NULL);
    int ret = lgpt_write(dp_dev, 0, &log_file, sizeof(log_file));
    assert(ret > 0);
    (void) ret;
}

static void disklog_reset_locked(void) {
    size_t blksz = 0;
    lgpt_get_block_size(dp_dev, &blksz);
    log_file.magic = DISKLOG_MAGIC;
    log_file.start = blksz;
    log_file.size = ((dp_dev->len - blksz) / blksz) * blksz;
    log_file.end = log_file.start + log_file.size;
    log_file.rd_ofs = log_file.start;
    log_file.wr_ofs = log_file.start;
    log_file.d_size = 0;
    logfile_save();
}

void disklog_reset(void) {
    assert(dp_dev != NULL);
    MTX_LOCK();
    disklog_reset_locked();
    MTX_UNLOCK();
}

int disklog_init(void) {
    static bool initialized;

    if (initialized)
        return 0;
    MTX_INIT();
    dp_dev = disk_partition_find("syslog");
    assert(dp_dev != NULL);

    MTX_LOCK();
    int ret = lgpt_read(dp_dev, 0, &log_file, sizeof(log_file));
    if (ret < 0) {
        MTX_UNLOCK();
        pr_err("read disklog file failed(%d)\n", ret);
        return ret;
    }
    if (log_file.magic != DISKLOG_MAGIC)
        disklog_reset_locked();
    initialized = true;
    MTX_UNLOCK();

    return 0;
}

int disklog_input(const char *buf, size_t size) {
    assert(dp_dev != NULL);
    size_t remain = size;
    uint32_t wr_ofs, bytes;
    int ret;

    if (buf == NULL || size == 0)
        return -EINVAL;

    MTX_LOCK();
    wr_ofs = log_file.wr_ofs;
    while (remain > 0) {
        bytes = MIN(log_file.end - wr_ofs, remain);
        ret = lgpt_write(dp_dev, wr_ofs, buf, bytes);
        if (ret <= 0) 
            goto _next;
        
        remain -= bytes;
        wr_ofs += bytes;
        buf += bytes;
        if (wr_ofs >= log_file.end)
            wr_ofs = log_file.start;
    }

    log_file.d_size += size - remain;
    if (log_file.d_size > log_file.size) {
        log_file.d_size = log_file.size;
        log_file.rd_ofs = wr_ofs;
    }
    ret = 0;
_next:
    log_file.wr_ofs = wr_ofs;
    logfile_save();
    MTX_UNLOCK();
    return ret;
}

int disklog_ouput(void (*output)(void *ctx, char *buf, size_t size), 
    void *ctx) {
#define LOG_SIZE 1024
    assert(dp_dev != NULL);
    uint32_t rd_ofs, bytes;
    char *buffer;
    int ret;

    if (output == NULL)
        return -EINVAL;
    buffer = general_malloc(LOG_SIZE+1);
    if (buffer == NULL)
        return -ENOMEM;

    MTX_LOCK();
    size_t size = log_file.d_size;
    size_t remain = size;
    rd_ofs = log_file.rd_ofs;
    while (remain > 0) {
        bytes = MIN(log_file.end - rd_ofs, remain);
        bytes = MIN(bytes, LOG_SIZE);
        ret = lgpt_read(dp_dev, rd_ofs, buffer, bytes);
        if (ret <= 0)
            goto _next;

        output(ctx, buffer, bytes);
        remain -= bytes;
        rd_ofs += bytes;
        if (rd_ofs >= log_file.end)
            rd_ofs = log_file.start;
    }
    ret = 0;
_next:
    MTX_UNLOCK();
    general_free(buffer);
    return ret;    
}

