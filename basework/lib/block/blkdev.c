/*
 * Copyright 2022 wtcat
 */
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup libblock
 *
 * @brief Block Device IMFS
 */

/*
 * Copyright (c) 2012, 2018 embedded brains GmbH.  All rights reserved.
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

#include "basework/malloc.h"
#include "basework/lib/block/blkdev.h"
#include "basework/lib/block/bdbuf.h"


struct disk_device_context {
  rtems_disk_device dd;
  const char *name;
  struct disk_device_context *next;
};

static struct disk_device_context *disk_list;

static void disk_node_add(
  const char *name, 
  struct disk_device_context *ctx) {
  ctx->name = name;

  //TODO: add critical protect
  ctx->next = disk_list;
  disk_list = ctx;
}

ssize_t rtems_blkdev_read(
  rtems_disk_device *dd, 
  void *buffer, 
  size_t count, 
  off_t *ofs) {
  int rv;
  ssize_t remaining = (ssize_t) count;
  off_t offset = *ofs;
  ssize_t block_size = (ssize_t) rtems_disk_get_block_size(dd);
  rtems_blkdev_bnum block = (rtems_blkdev_bnum) (offset / block_size);
  ssize_t block_offset = (ssize_t) (offset % block_size);
  char *dst = buffer;

  while (remaining > 0) {
    rtems_bdbuf_buffer *bd;
    int sc = rtems_bdbuf_read(dd, block, &bd);

    if (sc == 0) {
      ssize_t copy = block_size - block_offset;

      if (copy > remaining) {
        copy = remaining;
      }

      memcpy(dst, (char *) bd->buffer + block_offset, (size_t) copy);

      sc = rtems_bdbuf_release(bd);
      if (sc == 0) {
        block_offset = 0;
        remaining -= copy;
        dst += copy;
        ++block;
      } else {
        remaining = -1;
      }
    } else {
      remaining = -1;
    }
  }

  if (remaining >= 0) {
    *ofs += count;
    rv = (ssize_t) count;
  } else {
    errno = EIO;
    rv = -1;
  }

  return rv;
}

ssize_t rtems_blkdev_write(
  rtems_disk_device *dd, 
  const void *buffer, 
  size_t count, 
  off_t *ofs) {
  int rv;
  ssize_t remaining = (ssize_t) count;
  off_t offset = *ofs;
  ssize_t block_size = (ssize_t) rtems_disk_get_block_size(dd);
  rtems_blkdev_bnum block = (rtems_blkdev_bnum) (offset / block_size);
  ssize_t block_offset = (ssize_t) (offset % block_size);
  const char *src = buffer;

  while (remaining > 0) {
    int sc;
    rtems_bdbuf_buffer *bd;

    if (block_offset == 0 && remaining >= block_size) {
       sc = rtems_bdbuf_get(dd, block, &bd);
    } else {
       sc = rtems_bdbuf_read(dd, block, &bd);
    }

    if (sc == 0) {
      ssize_t copy = block_size - block_offset;

      if (copy > remaining) {
        copy = remaining;
      }

      memcpy((char *) bd->buffer + block_offset, src, (size_t) copy);

      sc = rtems_bdbuf_release_modified(bd);
      if (sc == 0) {
        block_offset = 0;
        remaining -= copy;
        src += copy;
        ++block;
      } else {
        remaining = -1;
      }
    } else {
      remaining = -1;
    }
  }

  if (remaining >= 0) {
    *ofs += count;
    rv = (ssize_t) count;
  } else {
    errno = EIO;
    rv = -1;
  }

  return rv;
}

int rtems_blkdev_control(
  rtems_disk_device *dd, 
  ioctl_command_t request, 
  void *buffer) {
  int rv = 0;

  if (request != RTEMS_BLKIO_REQUEST) {
    rv = (*dd->ioctl)(dd, request, buffer);
  } else {
    /*
     * It is not allowed to directly access the driver circumventing the cache.
     */
    errno = EINVAL;
    rv = -1;
  }

  return rv;
}

int rtems_blkdev_open(
  const char *name, 
  rtems_disk_device **pdd) {

  if (!name)
    return -EINVAL;
  if (!pdd)
    return -EINVAL;
  
  for (struct disk_device_context *dc = disk_list; dc; dc = dc->next) {
    if (!strcmp(dc->name, name)) {
      *pdd = &dc->dd;
      return 0;
    }
  }
  return -ENODATA;
}

int rtems_blkdev_create(
  const char *device,
  uint32_t media_block_size,
  rtems_blkdev_bnum media_block_count,
  rtems_block_device_ioctl handler,
  void *driver_data
)
{
  int sc;
  struct disk_device_context *ctx;

  sc = rtems_bdbuf_init();
  if (sc != 0) {
    return sc;
  }

  ctx = general_malloc(sizeof(*ctx));
  if (ctx != NULL) {
    sc = rtems_disk_init_phys(
      &ctx->dd,
      media_block_size,
      media_block_count,
      handler,
      driver_data
    );

    // ctx->fd = -1;
    if (sc == 0) {
      disk_node_add(device, ctx);
    } else {
      general_free(ctx);
    }
  } else {
    sc = -ENOMEM;
  }

  return sc;
}

int rtems_blkdev_create_partition(
  const char *partition,
  const char *parent_block_device,
  rtems_blkdev_bnum media_block_begin,
  rtems_blkdev_bnum media_block_count
)
{
  rtems_disk_device *phys_dd;
  struct disk_device_context *ctx;
  int sc;

  sc = rtems_blkdev_open(parent_block_device, &phys_dd);
  if (sc == 0) {
    ctx = general_malloc(sizeof(*ctx));
    if (ctx != NULL) {
      sc = rtems_disk_init_log(
        &ctx->dd,
        phys_dd,
        
        media_block_begin,
        media_block_count
      );

      if (sc == 0) {
        disk_node_add(partition, ctx);
      } else {
        general_free(ctx);
      }
    } else {
      sc = -ENOMEM;
    }
  } else {
    sc = -EINVAL;
  }

  return sc;
}
