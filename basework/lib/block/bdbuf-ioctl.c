/**
 * @file
 *
 * @ingroup rtems_blkdev
 *
 * @brief Common IO Control Primitive
 */

/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 */

#include <errno.h>

#include "basework/lib/block/blkdev.h"
#include "basework/lib/block/bdbuf.h"

int
rtems_blkdev_ioctl(rtems_disk_device *dd, uint32_t req, void *argp)
{
    int  sc;
    int  rc = 0;

    switch (req)
    {
        case RTEMS_BLKIO_GETMEDIABLKSIZE:
            *(uint32_t *) argp = dd->media_block_size;
            break;

        case RTEMS_BLKIO_GETBLKSIZE:
            *(uint32_t *) argp = dd->block_size;
            break;

        case RTEMS_BLKIO_SETBLKSIZE:
            sc = rtems_bdbuf_set_block_size(dd, *(uint32_t *) argp, true);
            if (sc != 0) {
                errno = EIO;
                rc = -1;
            }
            break;

        case RTEMS_BLKIO_GETSIZE:
            *(rtems_blkdev_bnum *) argp = dd->size;
            break;

        case RTEMS_BLKIO_SYNCDEV:
            sc = rtems_bdbuf_syncdev(dd);
            if (sc != 0) {
                errno = EIO;
                rc = -1;
            }
            break;

        case RTEMS_BLKIO_GETDISKDEV:
            *(rtems_disk_device **) argp = dd;
            break;

        case RTEMS_BLKIO_PURGEDEV:
            rtems_bdbuf_purge_dev(dd);
            break;

        case RTEMS_BLKIO_GETDEVSTATS:
            rtems_bdbuf_get_device_stats(dd, (rtems_blkdev_stats *) argp);
            break;

        case RTEMS_BLKIO_RESETDEVSTATS:
            rtems_bdbuf_reset_device_stats(dd);
            break;

        default:
            errno = EINVAL;
            rc = -1;
            break;
    }

    return rc;
}
