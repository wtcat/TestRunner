/*
 * Copyright 2022 wtcat
 *
 * Just only for bdbuf block device
 */
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "basework/lib/block/blkdev.h"
#include "basework/ccinit.h"

#define FLASH_DEVNAME "/dev/disk0"
#define FLASH_PGSZ 512
#define FLASH_NRBLK (FLASH_CAPACITY / FLASH_PGSZ)
#define FLASH_CAPACITY (1024 * 1024)

static uint8_t vdisk_memory[FLASH_CAPACITY];

static int vdisk_read(rtems_blkdev_request *req) {
    uint8_t *from = vdisk_memory;
    uint32_t   i;
    rtems_blkdev_sg_buffer *sg;

    for (i = 0, sg = req->bufs; i < req->bufnum; i++, sg++)
        memcpy(sg->buffer, from + (sg->block * FLASH_PGSZ), sg->length);
    rtems_blkdev_request_done (req, 0);
    return 0;
}

static int vdisk_write(rtems_blkdev_request *req) {
    uint8_t *to = vdisk_memory;
    uint32_t   i;
    rtems_blkdev_sg_buffer *sg;

    for (i = 0, sg = req->bufs; i < req->bufnum; i++, sg++)
        memcpy(to + (sg->block * FLASH_PGSZ), sg->buffer, sg->length);
    
    rtems_blkdev_request_done (req, 0);
    return 0;
}

static int vdisk_request(rtems_disk_device *dd, uint32_t req, void *argp) {
    switch (req) {
    case RTEMS_BLKIO_REQUEST: {
        rtems_blkdev_request *r = (rtems_blkdev_request *)argp;
        switch (r->req) {
            case RTEMS_BLKDEV_REQ_READ:
                return vdisk_read(r);
            case RTEMS_BLKDEV_REQ_WRITE:
                return vdisk_write(r);
            default:
                errno = EINVAL;
                return -1;
        }
        break;
    }
    case RTEMS_BLKIO_DELETED:
        break;
    default:
        return rtems_blkdev_ioctl (dd, req, argp);
    }

    return 0;
}
  

CC_INIT(posix_vdisk, kDeviceOrder, 10) {
    int err;
    err = rtems_blkdev_create(FLASH_DEVNAME, FLASH_PGSZ,
        FLASH_NRBLK, vdisk_request, NULL);
    assert(err == 0);
    return err;
}
  