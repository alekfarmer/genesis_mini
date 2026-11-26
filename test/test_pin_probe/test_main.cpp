// Host unit tests for pin_probe (make test).
#include <string.h>
#include <unity.h>

#include "pin_probe.h"

using pin_probe::classify;
using pin_probe::describe;
using pin_probe::PinState;

void setUp() {}
void tearDown() {}

static void assertState(PinState expected, PinState actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

static void test_follows_internal_pull_is_floating() {
  assertState(PinState::kFloating, classify(true, false));
}

static void test_high_both_ways_is_pulled_high() {
  assertState(PinState::kPulledHigh, classify(true, true));
}

static void test_low_both_ways_is_pulled_low() {
  assertState(PinState::kPulledLow, classify(false, false));
}

static void test_opposite_of_pulls_is_odd() {
  assertState(PinState::kOdd, classify(false, true));
}

static void test_every_state_has_a_label() {
  TEST_ASSERT_EQUAL_STRING("floating", describe(PinState::kFloating));
  TEST_ASSERT_EQUAL_STRING("pulled HIGH", describe(PinState::kPulledHigh));
  TEST_ASSERT_EQUAL_STRING("pulled LOW", describe(PinState::kPulledLow));
  TEST_ASSERT_EQUAL_STRING("changing", describe(PinState::kOdd));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_follows_internal_pull_is_floating);
  RUN_TEST(test_high_both_ways_is_pulled_high);
  RUN_TEST(test_low_both_ways_is_pulled_low);
  RUN_TEST(test_opposite_of_pulls_is_odd);
  RUN_TEST(test_every_state_has_a_label);
  return UNITY_END();
}
