/*
 * Copyright 2022 wtcat
 * 
 * This is simple partition file component,
 * The file size is fixed and can't dynamic create a new file 
 * 
 */
#define pr_fmt(fmt) "<partition-file>: "fmt

#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#ifdef __ZEPHYR__
#include <posix/pthread.h>
#else
#include <pthread.h>
#endif
#include "basework/dev/partition_file.h"
#include "basework/dev/partition.h"
#include "basework/dev/disk.h"
#include "basework/dev/blkdev.h"
#include "basework/lib/crc.h"
#include "basework/malloc.h"
#include "basework/minmax.h"
#include "basework/log.h"


struct sfile_metadata {
    uint16_t magic;
#define USR_FILE_MAGIC 0x3721
    uint16_t crc;
    uint32_t len;
    uint32_t end;
} __packed;

struct sfile {
    struct sfile *link;
    const struct disk_partition *dp;
    struct disk_device *dd;
    pthread_mutex_t lock;
    struct sfile_metadata metadata;
    size_t base;
    size_t offset;
    size_t fsize;
    int refcnt;
    bool dirty;
};

STATIC_ASSERT(SFILE_METADATA_SIZE == sizeof(struct sfile_metadata), "");

#define MTX_INIT() pthread_mutex_init(&(fp)->lock, NULL)
#define MTX_LOCK() pthread_mutex_lock(&(fp)->lock)
#define MTX_UNLOCK() pthread_mutex_unlock(&(fp)->lock)

#define SFILE_MTX_LOCK()   pthread_mutex_lock(&sfile_mtx)
#define SFILE_MTX_UNLOCK() pthread_mutex_unlock(&sfile_mtx)

#ifdef __ZEPHYR__
static PTHREAD_MUTEX_DEFINE(sfile_mtx);
#else
static pthread_mutex_t sfile_mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

static struct sfile *sfile_head;

static inline ssize_t sfile_read(struct sfile *fp, void *buf, 
    size_t size, uint32_t offset) {
    return blkdev_read(fp->dd, buf, size, fp->dp->offset + offset);
}

static inline ssize_t sfile_write(struct sfile *fp, const void *buf, 
    size_t size, uint32_t offset) {
    return blkdev_write(fp->dd, buf, size, fp->dp->offset + offset);
}

static struct sfile* sfile_open(const char *name, bool *found) {
    const struct disk_partition *dp;
    struct disk_device *dd;
    struct sfile *fp;

    dp = disk_partition_find(name);
    if (dp == NULL) {
        pr_err("not found partition file(%s)\n", name);
        return NULL;
    }
    if (disk_device_open(dp->parent, &dd)) {
        pr_err("not found disk device(%s)\n", dp->parent);
        return NULL;
    }
    SFILE_MTX_LOCK();
    for (fp = sfile_head; fp; fp = fp->link) {
        if (fp->dp == dp) {
            fp->refcnt++;
            if (found)
                *found = true;
            SFILE_MTX_UNLOCK();
            return fp;
        }
    }
    fp = general_malloc(sizeof(*fp));
    if (!fp) {
        SFILE_MTX_UNLOCK();
        pr_err("no more memory!\n");
        return NULL;
    }

    fp->link = sfile_head;
    sfile_head = fp;
    fp->dd = dd;
    fp->dp = dp;
    *found = false;
    MTX_INIT();
    SFILE_MTX_UNLOCK();
    return fp;
}

static int sfile_close(struct sfile *fp) {
    struct sfile **ifp = &sfile_head;
    SFILE_MTX_LOCK();
    while (*ifp) {
        if ((*ifp) == fp) {
            *ifp = fp->link;
            fp->link = NULL;
            general_free(fp);
            SFILE_MTX_UNLOCK();
            return 0;
        }
        ifp = &(*ifp)->link;
    }
    SFILE_MTX_UNLOCK();
    return -ENODATA;
}

static uint16_t usr_sfile_chksum(struct sfile *fp) {
    size_t size = fp->metadata.len;
    size_t ofs = fp->base;
    char buffer[256];
    uint16_t crc = 0;

    while (size > 0) {
        size_t bytes = min(sizeof(buffer), size);
        int ret = sfile_read(fp, buffer, bytes, ofs);
        if ((size_t)ret != bytes) {
            pr_err("read file failed\n");
            return false;
        }
        crc = crc16part((const uint8_t *)buffer, bytes, crc);
        size -= bytes;
        ofs += bytes;
    }
    return crc;
}

int usr_sfile_open(const char *name, struct sfile **p) {
    const struct disk_partition *dp;
    struct sfile *fp;
    bool found = false;
    int err;

    if (!name || !p)
        return -EINVAL;

    fp = sfile_open(name, &found);
    if (found) {
        *p = fp;
        return 0;
    }
    dp = fp->dp;
    assert(dp != NULL);
    MTX_LOCK();

    err = sfile_read(fp, &fp->metadata, sizeof(fp->metadata), 0);
    if (err <= 0) {
        sfile_close(fp);
        err = -EIO;
        goto _unlock;
    }
    if (fp->metadata.magic != USR_FILE_MAGIC ||
        usr_sfile_chksum(fp) != fp->metadata.crc) {
        memset(&fp->metadata, 0, sizeof(fp->metadata));
        fp->metadata.magic = USR_FILE_MAGIC;
    }

    fp->refcnt = 1;
    fp->dirty = false;
    fp->base = sizeof(fp->metadata);
    fp->fsize = fp->metadata.len;
    fp->offset = 0;
    *p = fp;
_unlock:
    MTX_UNLOCK();
    return 0;
}

int usr_sfile_invalid(struct sfile *fp) {
    int err;
    if (!fp || !fp->dp)
        return -EINVAL;

    MTX_LOCK();
    memset(&fp->metadata, 0, sizeof(fp->metadata));
    fp->metadata.magic = USR_FILE_MAGIC;
    err = lgpt_write(fp->dp, 0, &fp->metadata, sizeof(fp->metadata));
    MTX_UNLOCK();
    return err > 0? 0: err;
}

ssize_t usr_sfile_write(struct sfile *fp, const void *buffer ,size_t size) {
    int ret;
    assert(fp != NULL);
    assert(fp->dp != NULL);
    if (!size)
        return 0;

    MTX_LOCK();
    if (fp->base + fp->offset + size > fp->dp->len) {
        ret = -EINVAL;
        goto _unlock;
    }
    ret = sfile_write(fp, buffer, size, fp->base + fp->offset);
    if (ret != (int)size) {
        ret = -EIO;
        goto _unlock;
    }
    fp->dirty = true;
    fp->offset += ret;
    if (fp->offset > fp->fsize)
        fp->fsize = fp->offset;
_unlock:
    MTX_UNLOCK();
    return ret;
}

ssize_t usr_sfile_read(struct sfile *fp, void *buffer ,size_t size) {
    int ret;
    assert(fp != NULL);
    assert(fp->dp != NULL);
    
    if (!size)
        return 0;
    MTX_LOCK();
    if (fp->base + fp->offset + size > fp->dp->len) {
        ret = -EINVAL;
        goto _unlock;
    }
    ret = sfile_read(fp, buffer, size, fp->base + fp->offset);
    if (ret != (int)size) {
        ret = -EIO;
        goto _unlock;
    }
    fp->offset += ret;

_unlock:
    MTX_UNLOCK();
    return ret;
}

int usr_sfile_setoffset(struct sfile *fp, off_t offset) {
    assert(fp != NULL);
    assert(fp->dp != NULL);
    MTX_LOCK();
    if (offset > (long)fp->dp->len) {
        MTX_UNLOCK();
        return -EINVAL;
    }
    fp->offset = (long)offset;
    MTX_UNLOCK();
    return 0;
}

size_t usr_sfile_size(struct sfile *fp) {
    assert(fp != NULL);
    assert(fp->dp != NULL);
    
    MTX_LOCK();
    size_t size = fp->fsize;
    MTX_UNLOCK();
    return size;
}

int usr_sfile_close(struct sfile *fp) {
    int err = 0;
    assert(fp != NULL);
    assert(fp->dp != NULL);
    MTX_LOCK();
    if (fp->refcnt > 0) {
        fp->refcnt--;
        if (!fp->refcnt) {
            if (fp->dirty) {
                fp->dirty = false;
                fp->metadata.len = fp->fsize;
                fp->metadata.crc = usr_sfile_chksum(fp);
                err = sfile_write(fp, &fp->metadata, 
                    sizeof(fp->metadata), 0);
                if (err <= 0) {
                    err = -EIO;
                    goto _unlock;
                }
                blkdev_sync();
            }
            err = sfile_close(fp);
        }
    }
_unlock:
    MTX_UNLOCK();
    return err;
}
