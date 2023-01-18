/*
 * Testsuite
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "basework/rq.h"
#include "basework/log.h"
#include "basework/dev/blkdev.h"
#include "basework/lib/printer.h"
#include "basework/ccinit.h"
#include "gtest/gtest.h"


RQ_DEFINE(rq_context, 16);

static void usr_exit(void) {
    blkdev_destroy();
    printf("Program exited\n");
}

int main(int argc, char *argv[]) {
    atexit(usr_exit);
    _rq_new_thread(&rq_context, NULL, 0, 0);
    c_initializer_run();
    testing::InitGoogleTest(&argc, argv);
    int err = RUN_ALL_TESTS();
    _rq_cancel_thread(&rq_context);

    return err;
}
