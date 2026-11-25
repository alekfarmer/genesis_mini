// Host unit tests for Debounce (make test).
#include <unity.h>

#include "debounce.h"

void setUp() {}
void tearDown() {}

static void test_no_press_while_released() {
  Debounce d(true, 150);
  TEST_ASSERT_FALSE(d.pressed(false, 0));
  TEST_ASSERT_FALSE(d.pressed(false, 1000));
}

static void test_press_reported_once_on_edge() {
  Debounce d(true, 150);
  TEST_ASSERT_FALSE(d.pressed(false, 0));
  TEST_ASSERT_TRUE(d.pressed(true, 10));
  TEST_ASSERT_FALSE(d.pressed(true, 20));    // held
  TEST_ASSERT_FALSE(d.pressed(true, 5000));  // still held
}

static void test_bounce_within_lockout_ignored() {
  Debounce d(true, 150);
  TEST_ASSERT_TRUE(d.pressed(true, 100));
  TEST_ASSERT_FALSE(d.pressed(false, 105));  // contact bounce
  TEST_ASSERT_FALSE(d.pressed(true, 110));
  TEST_ASSERT_FALSE(d.pressed(false, 200));
}

static void test_second_press_after_lockout() {
  Debounce d(true, 150);
  TEST_ASSERT_TRUE(d.pressed(true, 100));
  TEST_ASSERT_FALSE(d.pressed(false, 200));
  TEST_ASSERT_TRUE(d.pressed(true, 300));
}

static void test_first_press_at_time_zero() {
  Debounce d(true, 150);
  TEST_ASSERT_TRUE(d.pressed(true, 0));
}

static void test_active_low() {
  Debounce d(false, 150);
  TEST_ASSERT_FALSE(d.pressed(true, 0));  // HIGH = released
  TEST_ASSERT_TRUE(d.pressed(false, 10));
}

static void test_lockout_across_millis_rollover() {
  Debounce d(true, 150);
  TEST_ASSERT_TRUE(d.pressed(true, 0xFFFFFFFFu - 50));
  TEST_ASSERT_FALSE(d.pressed(false, 0xFFFFFFFFu - 40));
  TEST_ASSERT_FALSE(d.pressed(true, 50));   // only 101 ms later, after the wrap
  TEST_ASSERT_FALSE(d.pressed(false, 90));
  TEST_ASSERT_TRUE(d.pressed(true, 120));   // 171 ms later
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_no_press_while_released);
  RUN_TEST(test_press_reported_once_on_edge);
  RUN_TEST(test_bounce_within_lockout_ignored);
  RUN_TEST(test_second_press_after_lockout);
  RUN_TEST(test_first_press_at_time_zero);
  RUN_TEST(test_active_low);
  RUN_TEST(test_lockout_across_millis_rollover);
  return UNITY_END();
}
