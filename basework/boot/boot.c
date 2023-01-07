/*
 * Copyright 2022 wtcat
 *
 * Environment variables:
 * 
 * @crash: yes/or (whether the system has occurred exception)
 * @runtime: $time (last running time of the system)
 * @crash-cnt: $times (crash count of the system)
 *
 */
#define pr_fmt(fmt) "boot: "fmt

#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "basework/boot/boot.h"
#include "basework/dev/partition.h"
#include "basework/dev/disk.h"
#include "basework/env.h"
#include "basework/log.h"


#define REBOOT_MAX_LIMIT 10
#define MIN_TIME 60  /* Unit: seconds */

struct fw_desc {
    struct disk_device *dd;
    long offset;
};

// static const struct disk_partition *fw_boot, *fw_swap, *fw_save;
static struct disk_device *fw_loader, *fw_runner;
static char fw_cache[4096];

extern void *sys_get_nvram(size_t *size);
extern uint32_t crc32_calc(uint32_t crc, const void *buf, size_t size);


static bool env_streq(const char *key, const char *s) {
    const char *env = env_get(key);
    if (env && s)
        return !strcmp(env, s);
    return false;
}

static unsigned long env_strtoul(const char *key) {
    const char *env = env_get(key);
    if (env)
        return strtoul(env, NULL, 16);
    return 0;
}

static int env_setint(const char *key, int v) {
    char number[16];
    size_t len;

    len = snprintf(number, sizeof(number)-1, "%x", v);
    number[len] = '\0';
    return env_set(key, number, 1);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
		0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
		0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};

	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];
		crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
	}
	return ~crc;
}

static bool is_fw_partition_valid(const struct fw_desc *fd) {
    struct firmware_header fw;
    // size_t ofs = 0;
    int ret;

    ret = disk_device_read(fd->dd, &fw, sizeof(fw), fd->offset);
    if (ret <= 0) 
        return ret;

    if (fw.fh_magic != FH_MAGIC) {
        pr_err("Invalid fireware header (%x)\n", fw.fh_magic);
        return false;
    }

    if (fw.fh_size > fw.fh_isize) {
        pr_err("Invalid fireware size(cur(0x%x) org(0x%x))\n", fw.fh_size, fw.fh_isize);
        return false;
    }

    
    // ret = disk_partition_erase_all(dp);
    // if (ret) {
    //     pr_err("Erase partition(%s) failed\n", dp->name);
    //     return false;
    // }

    //TODO: CRC check
    return true;
}

static bool is_fw_recovery_partiton_valid(void) {
    return is_fw_partition_valid(fw_swap);
}

static bool is_fw_runtime_okay(void) {
    if (env_strtoul("runtime") > 3600)
        return true;
    return false;
}

static bool is_fw_need_recovery(void) {
    if (env_streq("crash", "yes")) {
        unsigned long errs = env_strtoul("crash-cnt");
        if (errs > REBOOT_MAX_LIMIT)
            return true;

        if (env_strtoul("runtime") < MIN_TIME) {
            errs++;
            env_setint("crash-cnt", errs);
        }
    }
    return false;
}

static bool is_fw_need_update(void) {
    if (env_streq("ota-update", "yes"))
        return true;
    return false;
}

static int fw_partition_copy_decomp(const struct fw_desc *dst, 
    const struct fw_desc *src, void (*notify)(const char *, int), 
    const char *action) {
    struct firmware_header fw;
    size_t ofs = 0;
    uint32_t crc = 0;
    int ret;

    if (!is_fw_partition_valid(src))
        return -EINVAL;

    /*
     * Copy paritition data
     */
    while (ofs < fw.fh_isize) {
        ret = disk_partition_read(src, ofs, fw_cache, sizeof(fw_cache));
        if (ret < 0) {
            pr_err("Read partition(%s) address(0x%x) failed", src->name, ofs);
            break;
        }

        ret = disk_partition_write(dst, ofs, fw_cache, ret);
        if (ret < 0) {
            pr_err("Write partition(%s) address(0x%x) failed", dst->name, ofs);
            break;
        }

        ofs += ret;
        if (notify)
            notify(action, (ofs * 100) / (fw.fh_isize << 1));
    }

    /* Verify */
    ofs = sizeof(fw); //TODO: wether we need fw_header????
    do {
        ret = disk_partition_read(dst, ofs, fw_cache, sizeof(fw_cache));
        if (ret < 0) {
            pr_err("Read partition(%s) address(0x%x) failed", src->name, ofs);
            break;
        }
        crc = crc32_calc(crc, fw_cache, ret);
        ofs += ret;
        if (ofs == fw.fh_isize) {
            if (crc != fw.fh_dcrc) {
                pr_err("CRC check failed(cur(0x%x) org(0x%x))\n", crc, fw.fh_dcrc);
                return -EBADF;
            }
            if (notify)
                notify(action, 100);
            return 0;
        }
        if (notify)
            notify(action, (ofs * 100) / (fw.fh_isize << 1) + 50);
    } while (true);

    return ret;
}

static int fw_partition_copy_comp(const struct disk_partition *dst, 
    const struct disk_partition *src, void (*notify)(const char *, int),
    const char *action) {
    return fw_partition_copy_decomp(dst, src, notify, action);
}

static int fw_partition_init(void) {
    int err;

    err = disk_device_open(FW_LOAD_DEVICE, &fw_loader);
    if (err) {
        pr_emerg("Not found disk(%s) and error code is %d)\n", FW_LOAD_DEVICE, err);
        return -ENODEV;
    }

    err = disk_device_open(FW_RUN_DEVICE, &fw_runner);
    if (err) {
        pr_emerg("Not found disk(%s) and error code is %d)\n", FW_RUN_DEVICE, err);
        return -ENODEV;
    }
#if 0
    fw_boot = disk_partition_find(FW_BOOT_PARTITION);
    if (!fw_boot) {
        pr_emerg("Not found boot partition\n");
        return -ENODEV;
    }
    fw_swap = disk_partition_find(FW_SWAP_PARTITION);
    if (!fw_swap) {
        pr_emerg("Not found swap partition\n");
        return -ENODEV;
    }
    fw_save = disk_partition_find(FW_SAVE_PARTITION);
    if (!fw_save) {
        pr_emerg("Not found download partition\n");
        return -ENODEV;
    }
#endif
    return 0;
}

/*
 * Bootloader entry
 */
int general_boot(void (*boot)(void), 
    void (*notify)(const char *, int)) {
    bool force_recovery = false;
    int trycnt = 3;
    int i = 0, err;
    void *nvram;
    size_t nvram_size;
    int err;

    if (!boot)
        return -EINVAL;

    nvram = sys_get_nvram(&nvram_size);
    env_load_ram(nvram, nvram_size);
    err = fw_partition_init();
    assert(err == 0);

    /*
     * If the new firmware is ready and we will update it.
     */
    if (is_fw_need_update()) {
        pr_info("firmware update beginning...\n");
_repeat_update:
        err = fw_partition_copy_decomp(fw_boot, fw_save, notify, "update");
        if (!err) {
            env_unset("crash-cnt");
            pr_info("firmware update success!\n");
            goto _do_boot;
        }
        if (i++ < trycnt)
            goto _repeat_update;

        pr_err("firmware update failed(%d)\n", err);
        force_recovery = true;
        i = 0;
    }

    /*
     * If the current firmware has some problems and infinite reboot 
     */
    if (force_recovery || is_fw_need_recovery()) {
        if (!is_fw_recovery_partiton_valid()) {
            pr_err("the firmware recovery partition is invalid!!\n");
            goto _err;
        }
        pr_info("firmware recovery beginning...\n");
_repeat_recovery:
        err = fw_partition_copy_comp(fw_boot, fw_swap, notify, "recovery");
        if (!err) {
            pr_info("firmware recovery success!\n");
            goto _do_boot;
        }
        if (i++ < trycnt)
            goto _repeat_recovery;
        goto _err;
    }

_do_boot:
    pr_dbg("Booting firmware ...\n");
    if (is_fw_runtime_okay()) 
        env_unset("crash-cnt");
    env_set("crash", "yes", 1);
    env_unset("runtime");
    env_flush_ram(nvram, nvram_size);
    boot();
    /* Should never reached here */
_err:
    return err;
}
