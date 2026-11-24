// Host unit tests for Periodic (make test).
#include <unity.h>

#include "periodic.h"

void setUp() {}
void tearDown() {}

static void test_not_due_before_interval() {
  Periodic p(100, 0);
  TEST_ASSERT_FALSE(p.due(0));
  TEST_ASSERT_FALSE(p.due(99));
}

static void test_due_at_interval_then_rearms() {
  Periodic p(100, 0);
  TEST_ASSERT_TRUE(p.due(100));
  TEST_ASSERT_FALSE(p.due(150));  // already fired for this interval
  TEST_ASSERT_TRUE(p.due(200));
}

static void test_late_call_fires_once_without_drift() {
  Periodic p(100, 0);
  TEST_ASSERT_TRUE(p.due(350));   // three intervals late: fires once...
  TEST_ASSERT_FALSE(p.due(350));  // ...not three times
  TEST_ASSERT_FALSE(p.due(399));
  TEST_ASSERT_TRUE(p.due(400));   // cadence stays on the 100 ms grid
}

static void test_survives_millis_rollover() {
  const uint32_t start = 0xFFFFFFFFu - 49;  // 50 ms before the counter wraps
  Periodic p(100, start);
  TEST_ASSERT_FALSE(p.due(start + 99));   // wraps to 49
  TEST_ASSERT_TRUE(p.due(start + 100));   // wraps to 50
  TEST_ASSERT_FALSE(p.due(start + 150));
}

static void test_reset_restarts_interval() {
  Periodic p(100, 0);
  p.reset(90);
  TEST_ASSERT_FALSE(p.due(100));
  TEST_ASSERT_TRUE(p.due(190));
}

static void test_zero_interval_is_always_due() {
  Periodic p(0, 0);
  TEST_ASSERT_TRUE(p.due(0));
  TEST_ASSERT_TRUE(p.due(0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_not_due_before_interval);
  RUN_TEST(test_due_at_interval_then_rearms);
  RUN_TEST(test_late_call_fires_once_without_drift);
  RUN_TEST(test_survives_millis_rollover);
  RUN_TEST(test_reset_restarts_interval);
  RUN_TEST(test_zero_interval_is_always_due);
  return UNITY_END();
}
