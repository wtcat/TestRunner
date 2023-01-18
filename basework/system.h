/*
 *  Copyright 2022 wtcat
 */

#ifndef BASEWORK_SYSTEM_H_
#define BASEWORK_SYSTEM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

struct nvram_desc {
    uint32_t magic;
#define NVRAM_MAGIC 0x5abcdefa
#ifdef _WIN32
    char __data_begin[1];
#else
    char __data_begin[0];
#endif
    char env_ram[512];
    char crash_ram[1024];
    char __data_end[0];
};


/*
 * Global system absract interface
 */
struct system_operations {
    /*
     * Reboot system
     */
    void (*reboot)(void);

    /*
     * Get crash recovry system data
     */
    uint32_t (*get_time_since_boot)(void);

};


struct nvram_desc *sys_nvram_get(void);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_SYSTEM_H_ */