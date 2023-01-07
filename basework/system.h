/*
 *  Copyright 2022 wtcat
 */

#ifndef BASEWORK_SYSTEM_H_
#define BASEWORK_SYSTEM_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif

struct crsi_key;

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
    struct crsi_key *(*get_crsi)(void);

    uint32_t (*get_time_since_boot)(void);

    char *(*getenv)(const char *name);
    int (*setenv)(const char *name, const char *value, int overwrite);

};

#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_SYSTEM_H_ */