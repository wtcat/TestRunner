/*
 * Copyright 2022 wtcat
 */
#include "basework/generic.h"
#include "basework/dev/partition.h"
#include "basework/dev/partition_file.h"

#define BYTE(n) SFILE_SIZE(n)
#define PT_ENTRY(name, size) \
    PARTITION_ENTRY(name, NULL, -1, size)

/*
 * Partition configure table
 */
PARTITION_TABLE_DEFINE(partitions_usr_configure) {
    PT_ENTRY("bindinfo", BYTE(64)), /* User bind information */
    PT_ENTRY("burn_in_main", BYTE(32)), /* User burn_in information */
    PT_ENTRY("burn_in_restart", BYTE(32)), /* User burn_in information */
    PT_ENTRY("burn_in_cp", BYTE(64)), /* User burn_in information */
    PT_ENTRY("burn_in_pd", BYTE(64)), /* User burn_in information */

    PARTITION_TERMINAL
};

struct disk_partition* __unused usr_partition_get(void) {
    return partitions_usr_configure;
}
