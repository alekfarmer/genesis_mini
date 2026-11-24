// Host unit tests for the heartbeat pattern (make test).
#include <unity.h>

#include "heartbeat.h"

using heartbeat::isOn;
using heartbeat::kCycleMs;
using heartbeat::kPulseMs;
using heartbeat::kSecondPulseStartMs;

void setUp() {}
void tearDown() {}

static void test_first_pulse() {
  TEST_ASSERT_TRUE(isOn(0));
  TEST_ASSERT_TRUE(isOn(kPulseMs - 1));
  TEST_ASSERT_FALSE(isOn(kPulseMs));
}

static void test_gap_between_pulses() {
  TEST_ASSERT_FALSE(isOn(kSecondPulseStartMs - 1));
}

static void test_second_pulse() {
  TEST_ASSERT_TRUE(isOn(kSecondPulseStartMs));
  TEST_ASSERT_TRUE(isOn(kSecondPulseStartMs + kPulseMs - 1));
  TEST_ASSERT_FALSE(isOn(kSecondPulseStartMs + kPulseMs));
}

static void test_long_pause_until_cycle_end() {
  TEST_ASSERT_FALSE(isOn(kCycleMs - 1));
}

static void test_pattern_repeats_each_cycle() {
  TEST_ASSERT_TRUE(isOn(kCycleMs));
  TEST_ASSERT_TRUE(isOn(5 * kCycleMs + kSecondPulseStartMs));
  TEST_ASSERT_FALSE(isOn(5 * kCycleMs + kPulseMs));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_first_pulse);
  RUN_TEST(test_gap_between_pulses);
  RUN_TEST(test_second_pulse);
  RUN_TEST(test_long_pause_until_cycle_end);
  RUN_TEST(test_pattern_repeats_each_cycle);
  return UNITY_END();
}
