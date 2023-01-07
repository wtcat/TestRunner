/*
 * Copyright 2022 wtcat
 */
#define pr_fmt(fmt) "partitons_cfg: "fmt
#include <errno.h>

#include "basework/dev/partition.h"
#include "basework/log.h"

#define KB(n) ((n) * 1024)
#define MB(n) (KB(n) * 1024ul)

#define PT_ENTRY(name, size) \
    PARTITION_ENTRY(name, NULL, -1, size)

/*
 * Partition configure table
 */
PARTITION_TABLE_DEFINE(partitions_configure) {
    PT_ENTRY("syslog", KB(16)),

    PARTITION_TERMINAL
};


int partitions_configure_build(long base_addr, size_t size, const char *phydev) {
    struct disk_partition *dp = partitions_configure;
    uint32_t offset;
    int err;

    if (dp->offset == -1)
        dp->offset = base_addr;
    offset = dp->offset;

    while (dp->name) {
        if (dp->offset == -1)
            dp->offset = offset;

        if (!dp->parent)
            dp->parent = phydev;

        offset += dp->len;
        if (offset > base_addr + size)
            return -EINVAL;
        dp++;
    }

    err = disk_partition_register(partitions_configure, dp-partitions_configure);
    if (err) {
        pr_err("partition register failed: %d\n", err);
        return err;
    }
    
    disk_partition_dump();
    return 0;
}
