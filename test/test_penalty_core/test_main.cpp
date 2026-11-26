// Host unit tests for Penalty Kick (make test).
#include <unity.h>

#include "penalty_core.h"

using namespace penalty;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t kSeed = 2026;

void assertPhase(Phase expected, const Game& g) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(g.phase()));
}

void assertResult(Result expected, Result actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

void ticks(Game& g, int n) {
  for (int i = 0; i < n; ++i) g.tick();
}

/// Aim at `x`, then let the meter rise for `meter_ticks` and shoot. The meter
/// starts at 0 and rises kMeterStep per tick.
void shootAt(Game& g, int16_t x, int meter_ticks) {
  g.moveAim((x - g.aimX()) / kAimStepPx);
  g.press();
  ticks(g, meter_ticks);
  g.press();
}

/// Shoot, wait for the ball to arrive, and return the result.
Result takeShot(Game& g, int16_t x, int meter_ticks) {
  shootAt(g, x, meter_ticks);
  ticks(g, kFlightTicks);
  return g.lastResult();
}

/// Finish the current result screen.
void nextShot(Game& g) { g.press(); }

}  // namespace

static void test_new_game() {
  Game g(kSeed);
  assertPhase(Phase::kAim, g);
  TEST_ASSERT_EQUAL_INT(1, g.round());
  TEST_ASSERT_EQUAL_INT(0, g.shot());
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT16(kBallStartX, g.aimX());
  TEST_ASSERT_EQUAL_INT16(kGoalieStartX, g.goalieX());
  TEST_ASSERT_EQUAL_INT16(kBallStartX, g.ballX());
  TEST_ASSERT_EQUAL_INT16(kBallStartY, g.ballY());
}

static void test_aim_moves_and_clamps_only_while_aiming() {
  Game g(kSeed);
  g.moveAim(2);
  TEST_ASSERT_EQUAL_INT16(kBallStartX + 2 * kAimStepPx, g.aimX());
  g.moveAim(1000);
  TEST_ASSERT_EQUAL_INT16(kAimMax, g.aimX());
  g.moveAim(-1000);
  TEST_ASSERT_EQUAL_INT16(kAimMin, g.aimX());
  g.press();  // lock
  assertPhase(Phase::kPower, g);
  g.moveAim(5);
  TEST_ASSERT_EQUAL_INT16(kAimMin, g.aimX());
}

static void test_power_meter_swings_up_and_down() {
  Game g(kSeed);
  g.press();
  TEST_ASSERT_EQUAL_INT16(0, g.power());
  ticks(g, 10);
  TEST_ASSERT_EQUAL_INT16(10 * kMeterStep, g.power());
  ticks(g, 15);  // reaches 100
  TEST_ASSERT_EQUAL_INT16(kMaxPower, g.power());
  ticks(g, 5);   // and comes back down
  TEST_ASSERT_EQUAL_INT16(kMaxPower - 5 * kMeterStep, g.power());
}

static void test_flight_then_result() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  shootAt(g, 120, 10);
  assertPhase(Phase::kFlight, g);
  g.press();  // ignored mid-flight
  assertPhase(Phase::kFlight, g);
  ticks(g, kFlightTicks - 1);
  assertPhase(Phase::kFlight, g);
  g.tick();
  assertPhase(Phase::kResult, g);
}

static void test_goal_when_the_goalie_stays_put() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  assertResult(Result::kGoal, takeShot(g, 120, 10));
  TEST_ASSERT_EQUAL_UINT32(1, g.score());
  TEST_ASSERT_EQUAL_INT(1, g.goalsThisRound());
  TEST_ASSERT_EQUAL_INT(1, g.shot());
  assertResult(Result::kGoal, g.shotResult(0));
}

static void test_saved_when_the_goalie_reads_a_close_shot() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kAlwaysReads);
  assertResult(Result::kSaved, takeShot(g, 92, 10));
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
}

static void test_too_much_power_goes_over_the_bar() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  assertResult(Result::kOver, takeShot(g, 100, 23));  // power 92
}

static void test_aim_outside_the_posts_is_wide() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  assertResult(Result::kWide, takeShot(g, 140, 10));
}

static void test_result_screen_moves_on_by_press_or_by_itself() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  takeShot(g, 120, 10);
  nextShot(g);
  assertPhase(Phase::kAim, g);
  TEST_ASSERT_EQUAL_INT16(kBallStartX, g.ballX());  // ball back on the spot
  takeShot(g, 120, 10);
  ticks(g, kResultTicks);
  assertPhase(Phase::kAim, g);
  TEST_ASSERT_EQUAL_INT(2, g.shot());
}

static void test_three_goals_reach_the_next_round_and_it_gets_harder() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  const uint16_t react1 = g.goalieReactionTicks();
  const int16_t speed1 = g.goalieSpeed();
  const int read1 = g.goalieReadPercent();
  for (int s = 0; s < kShotsPerRound; ++s) {
    takeShot(g, s < 3 ? 120 : 140, 10);  // 3 goals, 2 wide
    nextShot(g);
  }
  assertPhase(Phase::kRoundOver, g);
  TEST_ASSERT_EQUAL_INT(3, g.goalsThisRound());
  g.press();
  assertPhase(Phase::kAim, g);
  TEST_ASSERT_EQUAL_INT(2, g.round());
  TEST_ASSERT_EQUAL_INT(0, g.shot());
  TEST_ASSERT_EQUAL_INT(0, g.goalsThisRound());
  assertResult(Result::kNone, g.shotResult(0));
  TEST_ASSERT_EQUAL_UINT32(3, g.score());  // the total carries on
  TEST_ASSERT_TRUE(g.goalieReactionTicks() < react1);
  TEST_ASSERT_TRUE(g.goalieSpeed() >= speed1);
  TEST_ASSERT_TRUE(g.goalieReadPercent() > read1);
}

static void test_fewer_than_three_goals_ends_the_game() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  for (int s = 0; s < kShotsPerRound; ++s) {
    takeShot(g, s < 2 ? 120 : 140, 10);  // only 2 goals
    nextShot(g);
  }
  assertPhase(Phase::kRoundOver, g);
  g.press();
  assertPhase(Phase::kGameOver, g);
  TEST_ASSERT_EQUAL_UINT32(2, g.score());
  const uint32_t r = g.revision();
  g.press();
  g.tick();
  assertPhase(Phase::kGameOver, g);
  TEST_ASSERT_EQUAL_UINT32(r, g.revision());
}

static void test_goalie_difficulty_has_limits() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  for (int round = 0; round < 12 && g.phase() != Phase::kGameOver; ++round) {
    for (int s = 0; s < kShotsPerRound; ++s) {
      takeShot(g, 120, 10);
      nextShot(g);
    }
    g.press();
  }
  TEST_ASSERT_TRUE(g.round() >= 12);
  TEST_ASSERT_TRUE(g.goalieReactionTicks() >= 2);
  TEST_ASSERT_TRUE(g.goalieReadPercent() <= 80);
  TEST_ASSERT_TRUE(g.goalieSpeed() <= 5);
}

static void test_reset() {
  Game g(kSeed);
  g.setGoalieModeForTest(GoalieMode::kStaysPut);
  takeShot(g, 120, 10);
  g.reset(kSeed + 1);
  assertPhase(Phase::kAim, g);
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT(1, g.round());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_new_game);
  RUN_TEST(test_aim_moves_and_clamps_only_while_aiming);
  RUN_TEST(test_power_meter_swings_up_and_down);
  RUN_TEST(test_flight_then_result);
  RUN_TEST(test_goal_when_the_goalie_stays_put);
  RUN_TEST(test_saved_when_the_goalie_reads_a_close_shot);
  RUN_TEST(test_too_much_power_goes_over_the_bar);
  RUN_TEST(test_aim_outside_the_posts_is_wide);
  RUN_TEST(test_result_screen_moves_on_by_press_or_by_itself);
  RUN_TEST(test_three_goals_reach_the_next_round_and_it_gets_harder);
  RUN_TEST(test_fewer_than_three_goals_ends_the_game);
  RUN_TEST(test_goalie_difficulty_has_limits);
  RUN_TEST(test_reset);
  return UNITY_END();
}
