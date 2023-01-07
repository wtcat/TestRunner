#include "basework/ccinit.h"
#include "basework/log.h"

extern "C" int cstd_fs_init(void);

CC_INIT(win32_platform, kApplicationOrder, 0) {
	static struct printer log_printer;
	printf_format_init(&log_printer);
	pr_log_init(&log_printer);
	cstd_fs_init();
	return 0;
}