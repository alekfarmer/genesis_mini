// Host unit tests for ThresholdAlarm (make test).
#include <math.h>
#include <unity.h>

#include "threshold_alarm.h"

using State = ThresholdAlarm::State;

void setUp() {}
void tearDown() {}

static void assertState(State expected, State actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

static void test_idle_at_or_below_threshold() {
  ThresholdAlarm a(80.0f, 1.0f);
  assertState(State::kIdle, a.update(70.0f));
  assertState(State::kIdle, a.update(80.0f));  // must exceed, not equal
}

static void test_alarms_above_threshold() {
  ThresholdAlarm a(80.0f, 1.0f);
  assertState(State::kAlarming, a.update(80.5f));
}

static void test_stays_alarming_inside_hysteresis_band() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.update(82.0f);
  assertState(State::kAlarming, a.update(79.5f));
  assertState(State::kAlarming, a.update(79.0f));  // exactly at the band edge
}

static void test_clears_below_band() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.update(82.0f);
  assertState(State::kIdle, a.update(78.9f));
}

static void test_silence_and_rearm() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.update(82.0f);
  a.silence();
  assertState(State::kSilenced, a.state());
  assertState(State::kSilenced, a.update(85.0f));  // still hot: stays quiet
  assertState(State::kSilenced, a.update(79.5f));  // inside band: stays quiet
  assertState(State::kIdle, a.update(78.0f));      // cooled: re-armed
  assertState(State::kAlarming, a.update(81.0f));  // hot again: alarms
}

static void test_silence_when_idle_does_nothing() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.silence();
  assertState(State::kIdle, a.state());
  assertState(State::kAlarming, a.update(81.0f));
}

static void test_nan_reading_ignored() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.update(82.0f);
  assertState(State::kAlarming, a.update(NAN));
  ThresholdAlarm b(80.0f, 1.0f);
  assertState(State::kIdle, b.update(NAN));
}

static void test_raising_threshold_clears_on_next_update() {
  ThresholdAlarm a(80.0f, 1.0f);
  a.update(82.0f);
  a.setThreshold(90.0f);
  TEST_ASSERT_EQUAL_FLOAT(90.0f, a.threshold());
  assertState(State::kIdle, a.update(82.0f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_idle_at_or_below_threshold);
  RUN_TEST(test_alarms_above_threshold);
  RUN_TEST(test_stays_alarming_inside_hysteresis_band);
  RUN_TEST(test_clears_below_band);
  RUN_TEST(test_silence_and_rearm);
  RUN_TEST(test_silence_when_idle_does_nothing);
  RUN_TEST(test_nan_reading_ignored);
  RUN_TEST(test_raising_threshold_clears_on_next_update);
  return UNITY_END();
}
