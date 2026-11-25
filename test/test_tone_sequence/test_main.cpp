// Host unit tests for ToneSequence (make test).
#include <unity.h>

#include "tone_sequence.h"

namespace {
const ToneSequence::Step kBeep[] = {{2000, 100}, {0, 100}, {1500, 200}};
constexpr size_t kBeepCount = sizeof(kBeep) / sizeof(kBeep[0]);
}  // namespace

void setUp() {}
void tearDown() {}

static void test_inactive_until_started() {
  ToneSequence s(kBeep, kBeepCount, false);
  uint16_t f = 1;
  TEST_ASSERT_FALSE(s.active());
  TEST_ASSERT_FALSE(s.update(0, &f));
}

static void test_start_emits_first_step() {
  ToneSequence s(kBeep, kBeepCount, false);
  uint16_t f = 0;
  s.start(1000);
  TEST_ASSERT_TRUE(s.active());
  TEST_ASSERT_TRUE(s.update(1000, &f));
  TEST_ASSERT_EQUAL_UINT16(2000, f);
  TEST_ASSERT_FALSE(s.update(1050, &f));  // mid-step: no change
}

static void test_advances_through_steps() {
  ToneSequence s(kBeep, kBeepCount, false);
  uint16_t f = 0;
  s.start(0);
  s.update(0, &f);
  TEST_ASSERT_TRUE(s.update(100, &f));
  TEST_ASSERT_EQUAL_UINT16(0, f);
  TEST_ASSERT_TRUE(s.update(200, &f));
  TEST_ASSERT_EQUAL_UINT16(1500, f);
}

static void test_one_shot_ends_silent_and_inactive() {
  ToneSequence s(kBeep, kBeepCount, false);
  uint16_t f = 0;
  s.start(0);
  s.update(0, &f);
  TEST_ASSERT_TRUE(s.update(400, &f));  // total length is 400 ms
  TEST_ASSERT_EQUAL_UINT16(0, f);
  TEST_ASSERT_FALSE(s.active());
  TEST_ASSERT_FALSE(s.update(500, &f));
}

static void test_loop_restarts() {
  ToneSequence s(kBeep, kBeepCount, true);
  uint16_t f = 0;
  s.start(0);
  s.update(0, &f);
  TEST_ASSERT_TRUE(s.update(400, &f));
  TEST_ASSERT_EQUAL_UINT16(2000, f);
  TEST_ASSERT_TRUE(s.active());
}

static void test_late_update_skips_whole_steps() {
  ToneSequence s(kBeep, kBeepCount, true);
  uint16_t f = 0;
  s.start(0);
  s.update(0, &f);
  TEST_ASSERT_TRUE(s.update(250, &f));  // 250 ms is inside step 3 (200-400)
  TEST_ASSERT_EQUAL_UINT16(1500, f);
  TEST_ASSERT_FALSE(s.update(399, &f));
  TEST_ASSERT_TRUE(s.update(400, &f));  // stays on the original grid
  TEST_ASSERT_EQUAL_UINT16(2000, f);
}

static void test_stop_reports_silence_once() {
  ToneSequence s(kBeep, kBeepCount, true);
  uint16_t f = 0;
  s.start(0);
  s.update(0, &f);
  s.stop();
  TEST_ASSERT_FALSE(s.active());
  TEST_ASSERT_TRUE(s.update(10, &f));
  TEST_ASSERT_EQUAL_UINT16(0, f);
  TEST_ASSERT_FALSE(s.update(20, &f));
}

static void test_survives_millis_rollover() {
  ToneSequence s(kBeep, kBeepCount, false);
  uint16_t f = 0;
  const uint32_t start = 0xFFFFFFFFu - 49;
  s.start(start);
  s.update(start, &f);
  TEST_ASSERT_FALSE(s.update(start + 99, &f));
  TEST_ASSERT_TRUE(s.update(start + 100, &f));  // wrapped
  TEST_ASSERT_EQUAL_UINT16(0, f);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_inactive_until_started);
  RUN_TEST(test_start_emits_first_step);
  RUN_TEST(test_advances_through_steps);
  RUN_TEST(test_one_shot_ends_silent_and_inactive);
  RUN_TEST(test_loop_restarts);
  RUN_TEST(test_late_update_skips_whole_steps);
  RUN_TEST(test_stop_reports_silence_once);
  RUN_TEST(test_survives_millis_rollover);
  return UNITY_END();
}
