// Host unit tests for blockfall's game rules (make test).
#include <unity.h>

#include "blockfall_core.h"

using namespace blockfall;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t kSeed = 12345;
constexpr uint8_t kFilled = 1;

bool sameCells(const Cell a[4], const Cell b[4]) {
  for (int i = 0; i < 4; ++i) {
    bool found = false;
    for (int j = 0; j < 4; ++j) {
      found = found || (a[i].x == b[j].x && a[i].y == b[j].y);
    }
    if (!found) return false;
  }
  return true;
}

void fillRow(Game& g, int y, int except_from, int except_to) {
  for (int x = 0; x < kCols; ++x) {
    if (x < except_from || x > except_to) g.setCellForTest(x, y, kFilled);
  }
}

// Fill the bottom two rows except columns 0-1, then drop an O there: exactly
// two lines clear and the well is empty again.
void clearTwoLines(Game& g) {
  fillRow(g, kRows - 2, 0, 1);
  fillRow(g, kRows - 1, 0, 1);
  g.setPieceForTest(PieceType::kO, 0, 0, 0);
  g.hardDrop();
}

}  // namespace

// ---- Pieces ----------------------------------------------------------------

static void test_every_piece_has_four_distinct_cells() {
  for (int t = 0; t < kPieceTypes; ++t) {
    for (uint8_t r = 0; r < 4; ++r) {
      Cell c[4];
      pieceCells(static_cast<PieceType>(t), r, c);
      for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
          TEST_ASSERT_FALSE(c[i].x == c[j].x && c[i].y == c[j].y);
        }
      }
    }
  }
}

static void test_t_piece_rotates_clockwise() {
  Cell c[4];
  pieceCells(PieceType::kT, 0, c);  // point up:  .#. / ###
  const Cell up[4] = {{1, 0}, {0, 1}, {1, 1}, {2, 1}};
  TEST_ASSERT_TRUE(sameCells(c, up));
  pieceCells(PieceType::kT, 1, c);  // point right: #. / ## / #.  (column 1)
  const Cell right[4] = {{1, 0}, {1, 1}, {2, 1}, {1, 2}};
  TEST_ASSERT_TRUE(sameCells(c, right));
}

static void test_o_piece_never_changes() {
  Cell a[4], b[4];
  pieceCells(PieceType::kO, 0, a);
  for (uint8_t r = 1; r < 4; ++r) {
    pieceCells(PieceType::kO, r, b);
    TEST_ASSERT_TRUE(sameCells(a, b));
  }
}

static void test_i_piece_horizontal_then_vertical() {
  Cell c[4];
  pieceCells(PieceType::kI, 0, c);
  const Cell flat[4] = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
  TEST_ASSERT_TRUE(sameCells(c, flat));
  pieceCells(PieceType::kI, 1, c);
  const Cell tall[4] = {{2, 0}, {2, 1}, {2, 2}, {2, 3}};
  TEST_ASSERT_TRUE(sameCells(c, tall));
}

// ---- Random and bag --------------------------------------------------------

static void test_random_is_repeatable_and_varies() {
  Random a(kSeed), b(kSeed), c(kSeed + 1);
  const uint32_t a1 = a.next();
  TEST_ASSERT_EQUAL_UINT32(a1, b.next());
  TEST_ASSERT_NOT_EQUAL(a1, c.next());
  TEST_ASSERT_NOT_EQUAL(a1, a.next());
}

static void test_bag_deals_each_type_once_per_seven() {
  Bag bag(kSeed);
  for (int round = 0; round < 3; ++round) {
    int seen[kPieceTypes] = {0};
    for (int i = 0; i < kPieceTypes; ++i) {
      ++seen[static_cast<int>(bag.next())];
    }
    for (int t = 0; t < kPieceTypes; ++t) {
      TEST_ASSERT_EQUAL_INT(1, seen[t]);
    }
  }
}

static void test_bag_peek_matches_next() {
  Bag bag(kSeed);
  for (int i = 0; i < 20; ++i) {
    const PieceType p = bag.peek();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(p), static_cast<int>(bag.next()));
  }
}

// ---- Game start, movement, gravity -----------------------------------------

static void test_new_game_starts_empty_and_playing() {
  Game g(kSeed);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Game::State::kPlaying), static_cast<int>(g.state()));
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_UINT32(0, g.lines());
  TEST_ASSERT_EQUAL_UINT32(1, g.level());
  TEST_ASSERT_EQUAL_INT(0, g.piece().y);
  TEST_ASSERT_TRUE(g.piece().x >= 3 && g.piece().x <= 4);  // centred
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) TEST_ASSERT_EQUAL_UINT8(0, g.cell(x, y));
  }
}

static void test_moves_stop_at_walls() {
  Game g(kSeed);
  g.setPieceForTest(PieceType::kO, 0, 4, 0);
  int moves = 0;
  while (g.moveLeft()) ++moves;
  TEST_ASSERT_EQUAL_INT(4, moves);
  TEST_ASSERT_EQUAL_INT(0, g.piece().x);
  moves = 0;
  while (g.moveRight()) ++moves;
  TEST_ASSERT_EQUAL_INT(8, moves);  // O is 2 wide: x 8 is the last column pair
}

static void test_tick_moves_down_then_locks_on_floor() {
  Game g(kSeed);
  g.setPieceForTest(PieceType::kO, 0, 0, kRows - 3);
  TEST_ASSERT_FALSE(g.tick());  // rows 17-18 -> 18-19
  TEST_ASSERT_EQUAL_INT(kRows - 2, g.piece().y);
  TEST_ASSERT_TRUE(g.tick());   // can't move: locks
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PieceType::kO) + 1, g.cell(0, kRows - 1));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PieceType::kO) + 1, g.cell(1, kRows - 2));
  TEST_ASSERT_EQUAL_INT(0, g.piece().y);  // a new piece spawned at the top
}

static void test_soft_drop_scores_one_per_row() {
  Game g(kSeed);
  g.setPieceForTest(PieceType::kO, 0, 0, 0);
  g.tick(true);
  g.tick(true);
  TEST_ASSERT_EQUAL_UINT32(2, g.score());
}

static void test_hard_drop_locks_at_bottom_scoring_two_per_row() {
  Game g(kSeed);
  g.setPieceForTest(PieceType::kO, 0, 0, 0);
  g.hardDrop();  // falls 18 rows (top row 0 -> 18)
  TEST_ASSERT_EQUAL_UINT32(36, g.score());
  TEST_ASSERT_NOT_EQUAL(0, g.cell(0, kRows - 1));
}

static void test_revision_changes_on_visible_change() {
  Game g(kSeed);
  const uint32_t r0 = g.revision();
  g.moveLeft();
  TEST_ASSERT_NOT_EQUAL(r0, g.revision());
}

// ---- Rotation and wall kicks -----------------------------------------------

static void test_rotate_in_open_space() {
  Game g(kSeed);
  g.setPieceForTest(PieceType::kT, 0, 4, 5);
  TEST_ASSERT_TRUE(g.rotate());
  TEST_ASSERT_EQUAL_UINT8(1, g.piece().rotation);
  TEST_ASSERT_EQUAL_INT(4, g.piece().x);
}

static void test_vertical_i_kicks_off_right_wall() {
  Game g(kSeed);
  // Vertical I (rotation 1) occupies box column 2; at x=7 it sits in column 9.
  g.setPieceForTest(PieceType::kI, 1, 7, 5);
  TEST_ASSERT_TRUE(g.rotate());  // flat needs box columns 0-3 -> must shift left
  TEST_ASSERT_EQUAL_UINT8(2, g.piece().rotation);
  TEST_ASSERT_TRUE(g.piece().x <= 6);
}

static void test_rotation_blocked_when_no_kick_fits() {
  Game g(kSeed);
  // Vertical I in a one-column shaft: walls of blocks on both sides.
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (x != 5) g.setCellForTest(x, y, kFilled);
    }
  }
  g.setPieceForTest(PieceType::kI, 1, 3, 8);  // box column 2 -> board column 5
  TEST_ASSERT_FALSE(g.rotate());
  TEST_ASSERT_EQUAL_UINT8(1, g.piece().rotation);
  TEST_ASSERT_EQUAL_INT(3, g.piece().x);
}

// ---- Line clears and scoring -----------------------------------------------

static void test_single_line_clear() {
  Game g(kSeed);
  fillRow(g, kRows - 1, 0, 3);           // gap in columns 0-3
  g.setCellForTest(9, kRows - 2, kFilled);  // a block above, to check shifting
  g.setPieceForTest(PieceType::kI, 0, 0, 0);  // flat I covers columns 0-3
  g.hardDrop();
  TEST_ASSERT_EQUAL_UINT8(1, g.lastCleared());
  TEST_ASSERT_EQUAL_UINT32(1, g.lines());
  // 100 for the line at level 1, plus 2 x 18 rows of hard drop (I sits on row 1 of its box).
  TEST_ASSERT_EQUAL_UINT32(100 + 36, g.score());
  TEST_ASSERT_EQUAL_UINT8(kFilled, g.cell(9, kRows - 1));  // shifted down a row
  TEST_ASSERT_EQUAL_UINT8(0, g.cell(0, kRows - 1));
}

static void test_four_line_clear_scores_most() {
  Game g(kSeed);
  for (int y = kRows - 4; y < kRows; ++y) fillRow(g, y, 9, 9);  // gap in column 9
  g.setPieceForTest(PieceType::kI, 1, 7, 0);  // vertical I in column 9
  g.hardDrop();
  TEST_ASSERT_EQUAL_UINT8(4, g.lastCleared());
  TEST_ASSERT_EQUAL_UINT32(4, g.lines());
  TEST_ASSERT_EQUAL_UINT32(800 + 2 * 16, g.score());
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) TEST_ASSERT_EQUAL_UINT8(0, g.cell(x, y));
  }
}

static void test_level_and_speed_rise_every_ten_lines() {
  Game g(kSeed);
  TEST_ASSERT_EQUAL_UINT32(800, g.fallIntervalMs());
  for (int i = 0; i < 4; ++i) clearTwoLines(g);
  TEST_ASSERT_EQUAL_UINT32(8, g.lines());
  TEST_ASSERT_EQUAL_UINT32(1, g.level());
  clearTwoLines(g);
  TEST_ASSERT_EQUAL_UINT32(10, g.lines());
  TEST_ASSERT_EQUAL_UINT32(2, g.level());
  TEST_ASSERT_EQUAL_UINT32(730, g.fallIntervalMs());
}

static void test_fall_interval_has_a_floor() {
  Game g(kSeed);
  for (int i = 0; i < 60; ++i) clearTwoLines(g);  // 120 lines -> level 13
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Game::State::kPlaying), static_cast<int>(g.state()));
  TEST_ASSERT_EQUAL_UINT32(13, g.level());
  TEST_ASSERT_EQUAL_UINT32(100, g.fallIntervalMs());
}

// ---- Game over and restart -------------------------------------------------

static void test_game_over_when_spawn_is_blocked() {
  Game g(kSeed);
  // Block the top four rows (leaving column 0 open so they never clear), then
  // lock a piece at the bottom: the next piece has nowhere to appear.
  for (int y = 0; y < 4; ++y) fillRow(g, y, 0, 0);
  g.setPieceForTest(PieceType::kO, 0, 8, kRows - 2);
  g.hardDrop();
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Game::State::kGameOver), static_cast<int>(g.state()));
  TEST_ASSERT_FALSE(g.moveLeft());
  TEST_ASSERT_FALSE(g.rotate());
  TEST_ASSERT_FALSE(g.tick());
}

static void test_reset_starts_over() {
  Game g(kSeed);
  g.setCellForTest(0, kRows - 1, kFilled);
  g.setPieceForTest(PieceType::kO, 0, 4, 0);
  g.hardDrop();
  g.reset(kSeed + 7);
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_UINT8(0, g.cell(0, kRows - 1));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Game::State::kPlaying), static_cast<int>(g.state()));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_piece_has_four_distinct_cells);
  RUN_TEST(test_t_piece_rotates_clockwise);
  RUN_TEST(test_o_piece_never_changes);
  RUN_TEST(test_i_piece_horizontal_then_vertical);
  RUN_TEST(test_random_is_repeatable_and_varies);
  RUN_TEST(test_bag_deals_each_type_once_per_seven);
  RUN_TEST(test_bag_peek_matches_next);
  RUN_TEST(test_new_game_starts_empty_and_playing);
  RUN_TEST(test_moves_stop_at_walls);
  RUN_TEST(test_tick_moves_down_then_locks_on_floor);
  RUN_TEST(test_soft_drop_scores_one_per_row);
  RUN_TEST(test_hard_drop_locks_at_bottom_scoring_two_per_row);
  RUN_TEST(test_revision_changes_on_visible_change);
  RUN_TEST(test_rotate_in_open_space);
  RUN_TEST(test_vertical_i_kicks_off_right_wall);
  RUN_TEST(test_rotation_blocked_when_no_kick_fits);
  RUN_TEST(test_single_line_clear);
  RUN_TEST(test_four_line_clear_scores_most);
  RUN_TEST(test_level_and_speed_rise_every_ten_lines);
  RUN_TEST(test_fall_interval_has_a_floor);
  RUN_TEST(test_game_over_when_spawn_is_blocked);
  RUN_TEST(test_reset_starts_over);
  return UNITY_END();
}
