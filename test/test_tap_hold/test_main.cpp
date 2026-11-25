// Host unit tests for TapHold (make test).
#include <unity.h>

#include "tap_hold.h"

using Event = TapHold::Event;

void setUp() {}
void tearDown() {}

static void assertEvent(Event expected, Event actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

// Active-low button: pressed = false (LOW), released = true (HIGH).
constexpr bool kDown = false;
constexpr bool kUp = true;

static void test_idle_reports_nothing() {
  TapHold b(false, 250, 20);
  assertEvent(Event::kNone, b.update(kUp, 0));
  assertEvent(Event::kNone, b.update(kUp, 1000));
  TEST_ASSERT_FALSE(b.held());
}

static void test_short_press_is_a_tap_on_release() {
  TapHold b(false, 250, 20);
  b.update(kDown, 0);
  assertEvent(Event::kNone, b.update(kDown, 30));  // press accepted, no event yet
  b.update(kUp, 100);
  assertEvent(Event::kTap, b.update(kUp, 125));    // release settled
}

static void test_long_press_is_a_hold() {
  TapHold b(false, 250, 20);
  b.update(kDown, 0);
  b.update(kDown, 25);
  assertEvent(Event::kNone, b.update(kDown, 200));
  assertEvent(Event::kHoldStart, b.update(kDown, 260));
  TEST_ASSERT_TRUE(b.held());
  assertEvent(Event::kNone, b.update(kDown, 1000));  // hold start reported once
  b.update(kUp, 1100);
  assertEvent(Event::kHoldEnd, b.update(kUp, 1125)); // no tap after a hold
  TEST_ASSERT_FALSE(b.held());
}

static void test_bounce_shorter_than_settle_is_ignored() {
  TapHold b(false, 250, 20);
  assertEvent(Event::kNone, b.update(kDown, 0));
  assertEvent(Event::kNone, b.update(kUp, 5));   // bounce
  assertEvent(Event::kNone, b.update(kDown, 10));
  assertEvent(Event::kNone, b.update(kUp, 15));
  assertEvent(Event::kNone, b.update(kUp, 100)); // never stable long enough: no tap
}

static void test_active_high_button() {
  TapHold b(true, 250, 20);
  b.update(true, 0);
  b.update(true, 25);
  b.update(false, 50);
  assertEvent(Event::kTap, b.update(false, 75));
}

static void test_works_across_millis_rollover() {
  TapHold b(false, 250, 20);
  const uint32_t t = 0xFFFFFFFFu - 100;
  b.update(kDown, t);
  b.update(kDown, t + 25);
  assertEvent(Event::kHoldStart, b.update(kDown, t + 260));  // wrapped
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_idle_reports_nothing);
  RUN_TEST(test_short_press_is_a_tap_on_release);
  RUN_TEST(test_long_press_is_a_hold);
  RUN_TEST(test_bounce_shorter_than_settle_is_ignored);
  RUN_TEST(test_active_high_button);
  RUN_TEST(test_works_across_millis_rollover);
  return UNITY_END();
}
