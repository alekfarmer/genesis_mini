// Host unit tests for the beat maker's sequencer (make test).
#include <unity.h>

#include "beat_core.h"

using namespace beat;

void setUp() {}
void tearDown() {}

namespace {
constexpr int kKick = 0;
constexpr int kSnare = 1;
constexpr int kHat = 2;
constexpr int kBleep = 3;
}  // namespace

static void test_starts_with_starter_beat_stopped() {
  Sequencer s;
  TEST_ASSERT_FALSE(s.playing());
  TEST_ASSERT_EQUAL_INT(-1, s.currentStep());
  TEST_ASSERT_EQUAL_UINT16(kDefaultBpm, s.bpm());
  TEST_ASSERT_TRUE(s.active(kKick, 0));
  TEST_ASSERT_TRUE(s.active(kKick, 8));
  TEST_ASSERT_TRUE(s.active(kSnare, 4));
  TEST_ASSERT_TRUE(s.active(kSnare, 12));
  TEST_ASSERT_TRUE(s.active(kHat, 2));
  TEST_ASSERT_FALSE(s.active(kHat, 3));
  TEST_ASSERT_TRUE(s.active(kBleep, 14));
  TEST_ASSERT_FALSE(s.active(kSnare, 0));
}

static void test_toggle_and_clear() {
  Sequencer s;
  s.toggle(kSnare, 0);
  TEST_ASSERT_TRUE(s.active(kSnare, 0));
  s.toggle(kSnare, 0);
  TEST_ASSERT_FALSE(s.active(kSnare, 0));
  s.clear();
  for (int t = 0; t < kTracks; ++t) {
    for (int st = 0; st < kSteps; ++st) TEST_ASSERT_FALSE(s.active(t, st));
  }
  TEST_ASSERT_FALSE(s.active(-1, 0));  // out of range is just "off"
  TEST_ASSERT_FALSE(s.active(0, kSteps));
}

static void test_cursor_moves_row_by_row_and_wraps() {
  Sequencer s;
  TEST_ASSERT_EQUAL_INT(0, s.cursorTrack());
  TEST_ASSERT_EQUAL_INT(0, s.cursorStep());
  s.moveCursor(1);
  TEST_ASSERT_EQUAL_INT(1, s.cursorStep());
  s.moveCursor(kSteps);  // one full row
  TEST_ASSERT_EQUAL_INT(1, s.cursorTrack());
  TEST_ASSERT_EQUAL_INT(1, s.cursorStep());
  s.moveCursor(-(kSteps + 2));  // back past the start
  TEST_ASSERT_EQUAL_INT(kTracks - 1, s.cursorTrack());
  TEST_ASSERT_EQUAL_INT(kSteps - 1, s.cursorStep());
  s.moveCursor(1);  // and forward past the end
  TEST_ASSERT_EQUAL_INT(0, s.cursorTrack());
  TEST_ASSERT_EQUAL_INT(0, s.cursorStep());
}

static void test_toggle_at_cursor() {
  Sequencer s;
  s.clear();
  s.moveCursor(kSteps + 3);  // track 1, step 3
  s.toggleAtCursor();
  TEST_ASSERT_TRUE(s.active(kSnare, 3));
}

static void test_tempo_clamps_and_sets_step_length() {
  Sequencer s;
  s.setBpm(120);
  TEST_ASSERT_EQUAL_UINT32(125, s.stepIntervalMs());
  s.setBpm(60);
  TEST_ASSERT_EQUAL_UINT32(250, s.stepIntervalMs());
  s.setBpm(10);
  TEST_ASSERT_EQUAL_UINT16(kMinBpm, s.bpm());
  s.changeBpm(1000);
  TEST_ASSERT_EQUAL_UINT16(kMaxBpm, s.bpm());
  s.changeBpm(-2);
  TEST_ASSERT_EQUAL_UINT16(kMaxBpm - 2, s.bpm());
}

static void test_transport_steps_on_time_and_wraps() {
  Sequencer s;
  s.setBpm(120);  // 125 ms per step
  TEST_ASSERT_EQUAL_INT(-1, s.update(0));  // stopped
  s.start(1000);
  TEST_ASSERT_TRUE(s.playing());
  TEST_ASSERT_EQUAL_INT(0, s.update(1000));
  TEST_ASSERT_EQUAL_INT(-1, s.update(1124));
  TEST_ASSERT_EQUAL_INT(1, s.update(1125));
  TEST_ASSERT_EQUAL_INT(1, s.currentStep());
  int last = -1;
  for (uint32_t t = 1126; t <= 1000 + 125 * 16; ++t) {
    const int st = s.update(t);
    if (st >= 0) last = st;
  }
  TEST_ASSERT_EQUAL_INT(0, last);  // wrapped from step 15 back to 0
  s.stop();
  TEST_ASSERT_FALSE(s.playing());
  TEST_ASSERT_EQUAL_INT(-1, s.currentStep());
  TEST_ASSERT_EQUAL_INT(-1, s.update(9999));
}

static void test_late_update_skips_but_stays_on_grid() {
  Sequencer s;
  s.setBpm(120);
  s.start(0);
  s.update(0);
  TEST_ASSERT_EQUAL_INT(3, s.update(3 * 125 + 40));  // steps 1-2 skipped
  TEST_ASSERT_EQUAL_INT(-1, s.update(4 * 125 - 1));
  TEST_ASSERT_EQUAL_INT(4, s.update(4 * 125));
}

static void test_topmost_track_wins_each_step() {
  Sequencer s;  // starter beat
  TEST_ASSERT_EQUAL_INT(kKick, s.trackToPlay(0));    // kick + hat
  TEST_ASSERT_EQUAL_INT(kHat, s.trackToPlay(2));     // hat only
  TEST_ASSERT_EQUAL_INT(kSnare, s.trackToPlay(4));   // snare + hat
  TEST_ASSERT_EQUAL_INT(kHat, s.trackToPlay(14));    // hat + bleep
  TEST_ASSERT_EQUAL_INT(-1, s.trackToPlay(1));       // nothing
  s.clear();
  s.toggle(kBleep, 5);
  TEST_ASSERT_EQUAL_INT(kBleep, s.trackToPlay(5));
}

static void test_revision_tracks_visible_changes() {
  Sequencer s;
  uint32_t r = s.revision();
  s.moveCursor(1);
  TEST_ASSERT_NOT_EQUAL(r, s.revision());
  r = s.revision();
  s.changeBpm(1);
  TEST_ASSERT_NOT_EQUAL(r, s.revision());
  r = s.revision();
  s.start(0);
  s.update(0);
  TEST_ASSERT_NOT_EQUAL(r, s.revision());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_with_starter_beat_stopped);
  RUN_TEST(test_toggle_and_clear);
  RUN_TEST(test_cursor_moves_row_by_row_and_wraps);
  RUN_TEST(test_toggle_at_cursor);
  RUN_TEST(test_tempo_clamps_and_sets_step_length);
  RUN_TEST(test_transport_steps_on_time_and_wraps);
  RUN_TEST(test_late_update_skips_but_stays_on_grid);
  RUN_TEST(test_topmost_track_wins_each_step);
  RUN_TEST(test_revision_tracks_visible_changes);
  return UNITY_END();
}
