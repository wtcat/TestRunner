/*
 * Copyright 2023 wtcat
 */
#include "basework/kfifo.h"
// #include "gtest/gtest.h"

struct point_val {
    int x, y;
};

static DEFINE_KFIFO(fifo, struct point_val, 16);

void test_fifo(void) {


}
// TEST(kfifo, use) {

//     // INIT_KFIFO(fifo);

// }