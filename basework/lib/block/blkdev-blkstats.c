/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup rtems_blkdev Block Device Management
 *
 * @brief Block Device Statistics Command
 */

/*
 * Copyright (c) 2012 embedded brains GmbH.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2022 wtcat
 */
#include <errno.h>
#include <string.h>

#include "basework/lib/printer.h"
#include "basework/lib/block/blkdev.h"

void rtems_blkstats(struct printer* printer, const char *device, bool reset)
{
  rtems_disk_device *dd;
  int rv;

  rv = rtems_blkdev_open(device, &dd);
  if (rv == 0) {
    if (reset) {
        rv = rtems_disk_fd_reset_device_stats(dd);
        if (rv != 0) {
            virt_format(printer, "error: reset stats: %s\n", strerror(errno));
        }
    } else {
        uint32_t media_block_size = 0;
        uint32_t media_block_count = 0;
        uint32_t block_size = 0;
        rtems_blkdev_stats stats;

        rtems_disk_fd_get_media_block_size(dd, &media_block_size);
        rtems_disk_fd_get_block_count(dd, &media_block_count);
        rtems_disk_fd_get_block_size(dd, &block_size);

        rv = rtems_disk_fd_get_device_stats(dd, &stats);
        if (rv == 0) {
            rtems_blkdev_print_stats(
                &stats,
                media_block_size,
                media_block_count,
                block_size,
                printer
            );
        } else {
        virt_format(printer, "error: get stats: %s\n", strerror(errno));
        }
    }
  } else {
    virt_format(printer, "error: open device: %s\n", strerror(errno));
  }
}
