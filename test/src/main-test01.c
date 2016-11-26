#include "unity.h"
#include "main.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(halInit);
FAKE_VOID_FUNC(chSysInit);

void setUp(void) {
	RESET_FAKE(halInit);
	RESET_FAKE(chSysInit);
}

void tearDown(void) {

}

void test_mainInitializesChibiOS(void) {
	deadlock_init();

	TEST_ASSERT_EQUAL_INT(chSysInit_fake.call_count, 1);
}

void test_mainInitializesHAL(void) {
	deadlock_init();

	TEST_ASSERT_EQUAL_INT(halInit_fake.call_count, 1);
}
