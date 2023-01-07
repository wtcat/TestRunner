/*
 * Copyright 2022 wtcat
 */
#define pr_fmt(fmt) fmt
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "basework/log.h"
#include "basework/dev/disk.h"
#include "basework/dev/blkdev.h"
#include "basework/os/osapi.h"


os_critical_global_declare
static struct disk_device *disk_list;

#define DISK_FOREACH(dd) \
    for ((dd) = disk_list; (dd) != NULL; (dd) = (dd)->next)

static inline const char *disk_get_name(struct disk_device *dd) {
    assert(dd->get_name != NULL);
    return dd->get_name(dd->dev);
}

int disk_device_open(const char *name, struct disk_device **dd) {
    struct disk_device *pd;
    assert(dd != NULL);

    DISK_FOREACH(pd) {
        if (!strcmp(disk_get_name(pd), name)) {
            *dd = pd;
            return 0;
        }
    }

    return -ENODEV;
}

int disk_device_close(struct disk_device *dd) {
    (void) dd;
    return 0;
}

int disk_device_write(struct disk_device *dd, const void *buf, size_t size, 
    long offset) {
    assert(dd != NULL);
    assert(dd->write != NULL);
    if (buf == NULL)
        return -EINVAL;
    if (size == 0)
        return 0;
    if (offset + size > dd->len) {
        pr_err("Disk read address out of bound.");
        return -EINVAL;
    }

    return dd->write(dd->dev, buf, size, offset);
}

int disk_device_read(struct disk_device *dd, void *buf, size_t size, 
    long offset) {
    assert(dd != NULL);
    assert(dd->read != NULL);
    if (buf == NULL)
        return -EINVAL;
    if (size == 0)
        return 0;
    if (offset + size > dd->len) {
        pr_err("Disk write address out of bound.");
        return -EINVAL;
    }

    return dd->read(dd->dev, buf, size, offset);
}

int disk_device_erase(struct disk_device *dd, long offset, 
    size_t size) {
    assert(dd != NULL);
    assert(dd->erase != NULL);

    if (offset & (dd->blk_size - 1))
        return -EINVAL;
    if (size & (dd->blk_size - 1))
        return -EINVAL;
    if (size == 0)
        return 0;
    if (offset + size > dd->len) {
        pr_err("Disk write address out of bound.");
        return -EINVAL;
    }

    return dd->erase(dd->dev, offset, size);
}

int disk_device_ioctl(struct disk_device *dd, long cmd, void *arg) {
    assert(dd != NULL);
    assert(dd->ioctl != NULL);
    switch (cmd) {
    case DISK_GETBLKSIZE:
        assert(arg != NULL);
        *(size_t *)arg = dd->blk_size;
        break;
    case DISK_GETCAPACITY:
        assert(arg != NULL);
        *(size_t *)arg = dd->len;
        break;
    default:
        return dd->ioctl(dd->dev, cmd, arg);
    }
    return 0;
}

int disk_device_register(struct disk_device *dd) {
    os_critical_declare

    if (dd == NULL) {
        pr_err("invalid parameter\n");
        return -EINVAL;
    }
    if (dd->dev == NULL || dd->len == 0) {
        pr_err("invalid device handle or disk size\n");
        return -EINVAL;
    }
    if (dd->blk_size & (dd->blk_size - 1)) {
        pr_err("invalid disk block size\n");
        return -EINVAL;
    }
    if (dd->read == NULL || dd->write == NULL || dd->erase == NULL) {
        pr_err("no base operations\n");
        return -EINVAL;
    }
    
    blkdev_init();
    os_critical_lock
    dd->next = disk_list;
    disk_list = dd;
    os_critical_unlock
    return 0;
}
