// Host unit tests for Hide & Seek (make test).
#include <unity.h>

#include "hideseek_core.h"

using namespace hideseek;

void setUp() {}
void tearDown() {}

namespace {

void assertPhase(Phase expected, const Game& g) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(g.phase()));
}

void assertHint(Hint expected, Hint actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

/// Move the cursor to `index` from wherever it is.
void cursorTo(Game& g, int index) { g.moveCursor(index - g.cursor()); }

/// Hider picks `hide`; the board is passed; the seeker's turn begins.
void hideAt(Game& g, int hide) {
  cursorTo(g, hide);
  g.press();  // hide
  g.press();  // past "pass to seeker"
}

/// Play a whole round where the seeker searches `wrong` misses then finds.
void playRound(Game& g, int hide, int misses) {
  hideAt(g, hide);
  int searched = 0;
  for (int i = 0; searched < misses; ++i) {
    if (i == hide) continue;
    cursorTo(g, i);
    g.press();
    ++searched;
  }
  if (g.phase() == Phase::kSeek) {
    cursorTo(g, hide);
    g.press();
  }
}

}  // namespace

static void test_map_has_spots_left_to_right_on_screen() {
  for (int i = 0; i < kSpots; ++i) {
    TEST_ASSERT_TRUE(spot(i).x > 4 && spot(i).x < 156);
    TEST_ASSERT_TRUE(spot(i).y > 12 && spot(i).y < 76);
    if (i > 0) TEST_ASSERT_TRUE(spot(i).x >= spot(i - 1).x);
  }
}

static void test_hints_by_distance() {
  assertHint(Hint::kHot, hintFor(5, 6));    // (85,48) to (98,70): 25.5 px
  assertHint(Hint::kWarm, hintFor(1, 0));   // (30,55) to (12,20): 39 px
  assertHint(Hint::kCold, hintFor(11, 0));  // far corners
}

static void test_match_starts_with_player_one_hiding() {
  Game g;
  assertPhase(Phase::kHide, g);
  TEST_ASSERT_EQUAL_INT(0, g.round());
  TEST_ASSERT_EQUAL_INT(0, g.hider());
  TEST_ASSERT_EQUAL_INT(1, g.seeker());
  TEST_ASSERT_EQUAL_INT(0, g.cursor());
}

static void test_cursor_wraps() {
  Game g;
  g.moveCursor(-1);
  TEST_ASSERT_EQUAL_INT(kSpots - 1, g.cursor());
  g.moveCursor(2);
  TEST_ASSERT_EQUAL_INT(1, g.cursor());
}

static void test_hiding_then_passing() {
  Game g;
  cursorTo(g, 7);
  g.press();
  assertPhase(Phase::kPassToSeeker, g);
  TEST_ASSERT_EQUAL_INT(7, g.hiddenSpot());
  TEST_ASSERT_EQUAL_INT(0, g.cursor());  // reset so it doesn't give the spot away
  g.press();
  assertPhase(Phase::kSeek, g);
}

static void test_miss_gives_hint_and_marks_spot() {
  Game g;
  hideAt(g, 6);
  cursorTo(g, 5);
  g.press();
  assertPhase(Phase::kSeek, g);
  TEST_ASSERT_TRUE(g.searched(5));
  TEST_ASSERT_EQUAL_INT(1, g.searchesThisRound());
  assertHint(Hint::kHot, g.lastHint());
  g.press();  // same spot again: ignored
  TEST_ASSERT_EQUAL_INT(1, g.searchesThisRound());
}

static void test_finding_ends_the_round_and_scores_searches() {
  Game g;
  hideAt(g, 4);
  cursorTo(g, 0);
  g.press();
  cursorTo(g, 4);
  g.press();
  assertPhase(Phase::kRoundResult, g);
  TEST_ASSERT_TRUE(g.foundThisRound());
  TEST_ASSERT_EQUAL_UINT32(2, g.total(1));  // player 2 was seeking
  TEST_ASSERT_EQUAL_UINT32(0, g.total(0));
}

static void test_running_out_of_searches_costs_the_penalty() {
  Game g;
  playRound(g, 11, kMaxSearches);  // six misses, never found
  assertPhase(Phase::kRoundResult, g);
  TEST_ASSERT_FALSE(g.foundThisRound());
  TEST_ASSERT_EQUAL_UINT32(kMissPenalty, g.total(1));
}

static void test_roles_swap_each_round() {
  Game g;
  playRound(g, 3, 0);
  g.press();  // past the result
  assertPhase(Phase::kPassToHider, g);
  TEST_ASSERT_EQUAL_INT(1, g.round());
  TEST_ASSERT_EQUAL_INT(1, g.hider());
  g.press();
  assertPhase(Phase::kHide, g);
  for (int i = 0; i < kSpots; ++i) TEST_ASSERT_FALSE(g.searched(i));
  assertHint(Hint::kNone, g.lastHint());
}

static void test_match_winner_and_tie() {
  Game g;
  playRound(g, 2, 0);  // P2 seeks: 1
  g.press(); g.press();
  playRound(g, 2, 3);  // P1 seeks: 4
  g.press(); g.press();
  playRound(g, 9, 1);  // P2 seeks: 2 -> total 3
  g.press(); g.press();
  playRound(g, 9, 0);  // P1 seeks: 1 -> total 5
  g.press();
  assertPhase(Phase::kMatchOver, g);
  TEST_ASSERT_EQUAL_UINT32(5, g.total(0));
  TEST_ASSERT_EQUAL_UINT32(3, g.total(1));
  TEST_ASSERT_EQUAL_INT(1, g.winner());
  g.press();  // new match
  assertPhase(Phase::kHide, g);
  TEST_ASSERT_EQUAL_UINT32(0, g.total(1));

  Game t;
  for (int r = 0; r < kRounds; ++r) {
    playRound(t, 5, 1);
    t.press();
    if (r < kRounds - 1) t.press();
  }
  TEST_ASSERT_EQUAL_INT(-1, t.winner());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_map_has_spots_left_to_right_on_screen);
  RUN_TEST(test_hints_by_distance);
  RUN_TEST(test_match_starts_with_player_one_hiding);
  RUN_TEST(test_cursor_wraps);
  RUN_TEST(test_hiding_then_passing);
  RUN_TEST(test_miss_gives_hint_and_marks_spot);
  RUN_TEST(test_finding_ends_the_round_and_scores_searches);
  RUN_TEST(test_running_out_of_searches_costs_the_penalty);
  RUN_TEST(test_roles_swap_each_round);
  RUN_TEST(test_match_winner_and_tie);
  return UNITY_END();
}
