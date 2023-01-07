/*
 * Testsuite
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "basework/rq.h"
#include "basework/log.h"
#include "basework/lib/printer.h"
#include "basework/ccinit.h"
#include "gtest/gtest.h"


RQ_DEFINE(rq_context, 16);

int main(int argc, char *argv[]) {
    c_initializer_run();
    _rq_new_thread(&rq_context, NULL, 0, 0);
    testing::InitGoogleTest(&argc, argv);
    int err = RUN_ALL_TESTS();
    _rq_cancel_thread(&rq_context);

    return err;
}
