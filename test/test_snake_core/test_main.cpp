// Host unit tests for snake's game rules (make test).
#include <unity.h>

#include "snake_core.h"

using namespace snake;

void setUp() {}
void tearDown() {}

namespace {
constexpr uint32_t kSeed = 4242;

void assertCell(int x, int y, Cell c) {
  TEST_ASSERT_EQUAL_INT(x, c.x);
  TEST_ASSERT_EQUAL_INT(y, c.y);
}

bool isOver(const Game& g) { return g.state() == Game::State::kGameOver; }

/// A fresh game with the food parked in the top-left corner, out of the way.
Game freshGame() {
  Game g(kSeed);
  g.setFoodForTest(0, 0);
  return g;
}
}  // namespace

static void test_new_game_layout() {
  Game g = freshGame();
  TEST_ASSERT_FALSE(isOver(g));
  TEST_ASSERT_EQUAL_UINT16(kStartLength, g.length());
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kRight), static_cast<int>(g.direction()));
  assertCell(16, 7, g.segment(0));
  assertCell(15, 7, g.segment(1));
  assertCell(14, 7, g.segment(2));
  TEST_ASSERT_TRUE(g.occupies(15, 7));
  TEST_ASSERT_FALSE(g.occupies(17, 7));
}

static void test_food_starts_off_the_snake() {
  for (uint32_t seed = 1; seed < 50; ++seed) {
    Game g(seed);
    TEST_ASSERT_FALSE(g.occupies(g.food().x, g.food().y));
  }
}

static void test_step_moves_forward() {
  Game g = freshGame();
  TEST_ASSERT_FALSE(g.step());
  assertCell(17, 7, g.segment(0));
  assertCell(15, 7, g.segment(2));  // tail followed
  TEST_ASSERT_EQUAL_UINT16(kStartLength, g.length());
}

static void test_turns_are_relative() {
  Game g = freshGame();
  g.turnRight();
  g.step();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kDown), static_cast<int>(g.direction()));
  assertCell(16, 8, g.segment(0));
  g.turnLeft();
  g.step();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kRight), static_cast<int>(g.direction()));
  assertCell(17, 8, g.segment(0));
}

static void test_two_queued_turns_apply_over_two_steps() {
  Game g = freshGame();
  g.turnLeft();
  g.turnLeft();
  g.turnLeft();  // third is ignored
  g.step();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kUp), static_cast<int>(g.direction()));
  g.step();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kLeft), static_cast<int>(g.direction()));
  g.step();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Dir::kLeft), static_cast<int>(g.direction()));
}

static void test_wall_ends_the_game() {
  Game g = freshGame();
  for (int i = 0; i < 15; ++i) g.step();  // head x 16 -> 31
  TEST_ASSERT_FALSE(isOver(g));
  assertCell(31, 7, g.segment(0));
  g.step();
  TEST_ASSERT_TRUE(isOver(g));
  TEST_ASSERT_FALSE(g.step());  // nothing happens once over
  assertCell(31, 7, g.segment(0));
}

static void test_eating_grows_scores_and_moves_food() {
  Game g = freshGame();
  g.setFoodForTest(17, 7);
  TEST_ASSERT_TRUE(g.step());
  TEST_ASSERT_EQUAL_UINT16(kStartLength + 1, g.length());
  TEST_ASSERT_EQUAL_UINT32(1, g.score());
  assertCell(14, 7, g.segment(3));  // tail stayed put this step
  TEST_ASSERT_FALSE(g.occupies(g.food().x, g.food().y));
}

static void test_speed_rises_with_score_and_has_a_floor() {
  Game g = freshGame();
  TEST_ASSERT_EQUAL_UINT32(180, g.stepIntervalMs());
  g.setFoodForTest(17, 7);
  g.step();
  TEST_ASSERT_EQUAL_UINT32(175, g.stepIntervalMs());
  // Eat along a row: 14 more foods in front of the head.
  for (int x = 18; x <= 31; ++x) {
    g.setFoodForTest(x, 7);
    g.step();
  }
  TEST_ASSERT_EQUAL_UINT32(15, g.score());
  TEST_ASSERT_EQUAL_UINT32(105, g.stepIntervalMs());
  // Floor: 180 - 5 * 22 = 70; anything beyond stays at 70.
  Game h = freshGame();
  for (int i = 0; i < 25 && !isOver(h); ++i) {
    Cell head = h.segment(0);
    const int nx = head.x + 1;
    if (nx >= kCols) break;
    h.setFoodForTest(nx, head.y);
    h.step();
  }
  TEST_ASSERT_TRUE(h.stepIntervalMs() >= 70);
}

static void test_hitting_itself_ends_the_game() {
  Game g = freshGame();
  g.setFoodForTest(17, 7);
  g.step();
  g.setFoodForTest(18, 7);
  g.step();  // length 5, head (18,7)
  g.setFoodForTest(0, 0);
  g.turnRight();
  g.step();  // down  -> (18,8)
  g.turnRight();
  g.step();  // left  -> (17,8)
  TEST_ASSERT_FALSE(isOver(g));
  g.turnRight();
  g.step();  // up    -> (17,7): still body
  TEST_ASSERT_TRUE(isOver(g));
}

static void test_moving_into_the_tail_square_is_allowed() {
  Game g = freshGame();
  g.setFoodForTest(17, 7);
  g.step();  // length 4, head (17,7), tail (14,7)
  g.setFoodForTest(0, 0);
  g.turnRight();
  g.step();  // (17,8)
  g.turnRight();
  g.step();  // (16,8)
  g.turnRight();
  g.step();  // (16,7): the tail leaves this square on the same step
  TEST_ASSERT_FALSE(isOver(g));
  assertCell(16, 7, g.segment(0));
}

static void test_reset_starts_over() {
  Game g = freshGame();
  for (int i = 0; i < 20; ++i) g.step();
  TEST_ASSERT_TRUE(isOver(g));
  g.reset(kSeed + 1);
  TEST_ASSERT_FALSE(isOver(g));
  TEST_ASSERT_EQUAL_UINT16(kStartLength, g.length());
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
}

static void test_revision_changes_each_step() {
  Game g = freshGame();
  const uint32_t r = g.revision();
  g.step();
  TEST_ASSERT_NOT_EQUAL(r, g.revision());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_new_game_layout);
  RUN_TEST(test_food_starts_off_the_snake);
  RUN_TEST(test_step_moves_forward);
  RUN_TEST(test_turns_are_relative);
  RUN_TEST(test_two_queued_turns_apply_over_two_steps);
  RUN_TEST(test_wall_ends_the_game);
  RUN_TEST(test_eating_grows_scores_and_moves_food);
  RUN_TEST(test_speed_rises_with_score_and_has_a_floor);
  RUN_TEST(test_hitting_itself_ends_the_game);
  RUN_TEST(test_moving_into_the_tail_square_is_allowed);
  RUN_TEST(test_reset_starts_over);
  RUN_TEST(test_revision_changes_each_step);
  return UNITY_END();
}
