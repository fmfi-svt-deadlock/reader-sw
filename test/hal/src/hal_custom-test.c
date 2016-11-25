#include "unity.h"
#include "fff.h"

#include "hal_custom.h"

void setUp(void) {

}

void tearDown(void) {

}

void test_something(void) {
    halCustomInit();
    TEST_ASSERT_TRUE(true);
}
