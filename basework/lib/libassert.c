/*
 * Copyright 2022 wtcat
 */
#define pr_fmt(fmt) fmt
#include "basework/log.h"
#include "basework/os/osapi.h"

void __lib_assert_func(const char *file, int line, const char *func, 
    const char *failedexpr) {
    pr_emerg("Assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
        failedexpr,
        file,
        line,
        (func) ? ", function: " : "",
        (func) ? func : ""
    );
    os_panic();

    /* Never reached here */
}