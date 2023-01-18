/*
 * Copyright 2022 wtcat
 */
#include <string.h>

#include "basework/generic.h"
#include "basework/system.h"

#if defined (__ZEPHYR__)
static struct nvram_desc nvram_region __section(".system.nvram");
#else
static struct nvram_desc nvram_region;
#endif

struct nvram_desc *sys_nvram_get(void) {
    //TODO: fix critical problem (use atomic variable)
    if (nvram_region.magic != NVRAM_MAGIC) {
        nvram_region.magic = NVRAM_MAGIC;
        memset(nvram_region.__data_begin, 0, 
            nvram_region.__data_end - nvram_region.__data_begin);
    }

    return &nvram_region;
}
