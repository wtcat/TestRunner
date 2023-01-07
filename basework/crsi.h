/*
 * Copyright 2022 wtcat
 *
 * Crash recovery system information
 */
#ifndef BASEWORK_SOS_H_
#define BASEWORK_SOS_H_

#include <stddef.h>
#include <stdint.h>

#include "basework/circ_buffer.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifndef CONFIG_KDUMP_SIZE
#define CONFIG_KDUMP_SIZE 4096
#endif

struct crsi_key {
#define CRSI_MAGIC 0x5abcdefa
    uint32_t magic;
    union {
        char kdump[CONFIG_KDUMP_SIZE];
        struct circ_buffer circdump;
    };
};

_Static_assert((CONFIG_KDUMP_SIZE & (CONFIG_KDUMP_SIZE - 1)) == 0, "");

/*
 * crsi_get_key - Get system recovery informat
 * 
 * When the system is shutdown or reboot the key will save in and
 * it should not be lost
 */
struct crsi_key *crsi_get_key(void);

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_SOS_H_ */
