/*
 * Copyright 2022 wtcat
 */
#include "basework/lib/printer.h"
#include "basework/log.h"
#include "basework/ccinit.h"

static struct printer log_printer;
static struct printer disk_printer;

CC_INIT(platform_log, kDeviceOrder, 0) {
	printf_format_init(&log_printer);
	pr_log_init(&log_printer);
    return 0;
}

CC_INIT(posix_platform, kApplicationOrder, 0) {
	disklog_format_init(&disk_printer);
	pr_disklog_init(&disk_printer);
    return 0;
}