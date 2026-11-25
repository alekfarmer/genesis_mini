// Host unit tests for the shooter's game rules (make test).
#include <unity.h>

#include "shooter_core.h"

using namespace shooter;

void setUp() {}
void tearDown() {}

namespace {
constexpr uint32_t kSeed = 777;
constexpr int16_t kStartShipY = (kShipMinY + kShipMaxY) / 2;  // 41

/// A game with automatic rock spawning off, so tests control every rock.
Game quietGame() {
  Game g(kSeed);
  g.setSpawningForTest(false);
  return g;
}

int16_t shipCentreY(const Game& g) { return static_cast<int16_t>(g.shipY() + kShipHeight / 2); }

int activeRocks(const Game& g) {
  int n = 0;
  for (int i = 0; i < kMaxRocks; ++i) n += g.rock(i).active ? 1 : 0;
  return n;
}

int activeBullets(const Game& g) {
  int n = 0;
  for (int i = 0; i < kMaxBullets; ++i) n += g.bullet(i).active ? 1 : 0;
  return n;
}

bool isOver(const Game& g) { return g.state() == Game::State::kGameOver; }
}  // namespace

static void test_new_game() {
  Game g = quietGame();
  TEST_ASSERT_FALSE(isOver(g));
  TEST_ASSERT_EQUAL_UINT8(kStartLives, g.lives());
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT16(kStartShipY, g.shipY());
  TEST_ASSERT_EQUAL_INT(0, activeRocks(g));
  TEST_ASSERT_EQUAL_INT(0, activeBullets(g));
}

static void test_ship_moves_and_stays_on_screen() {
  Game g = quietGame();
  g.moveShip(1);
  TEST_ASSERT_EQUAL_INT16(kStartShipY - kShipStepPx, g.shipY());
  g.moveShip(100);
  TEST_ASSERT_EQUAL_INT16(kShipMinY, g.shipY());
  g.moveShip(-100);
  TEST_ASSERT_EQUAL_INT16(kShipMaxY, g.shipY());
}

static void test_fire_spawns_at_nose_with_cooldown() {
  Game g = quietGame();
  TEST_ASSERT_TRUE(g.fire());
  TEST_ASSERT_EQUAL_INT(1, activeBullets(g));
  TEST_ASSERT_EQUAL_INT16(kShipNoseX, g.bullet(0).x);
  TEST_ASSERT_EQUAL_INT16(shipCentreY(g), g.bullet(0).y);
  TEST_ASSERT_FALSE(g.fire());  // cooling down
  for (int i = 0; i < kFireCooldownTicks; ++i) g.tick();
  TEST_ASSERT_TRUE(g.fire());
}

static void test_bullet_flies_right_and_leaves() {
  Game g = quietGame();
  g.fire();
  g.tick();
  TEST_ASSERT_EQUAL_INT16(kShipNoseX + kBulletSpeedPx, g.bullet(0).x);
  for (int i = 0; i < 40; ++i) g.tick();
  TEST_ASSERT_EQUAL_INT(0, activeBullets(g));
}

static void test_shot_destroys_rock_and_scores() {
  Game g = quietGame();
  g.addRockForTest(40, shipCentreY(g), 5, 0);
  g.fire();
  uint8_t hits = 0;
  for (int i = 0; i < 10; ++i) {
    g.tick();
    hits = static_cast<uint8_t>(hits + g.rocksHitLastTick());
  }
  TEST_ASSERT_EQUAL_UINT8(1, hits);
  TEST_ASSERT_EQUAL_INT(0, activeRocks(g));
  TEST_ASSERT_EQUAL_INT(0, activeBullets(g));
  TEST_ASSERT_EQUAL_UINT32(20, g.score());  // radius 5: 10 + (7 - 5) * 5
}

static void test_smaller_rocks_score_more() {
  Game g = quietGame();
  g.addRockForTest(40, shipCentreY(g), 3, 0);
  g.fire();
  for (int i = 0; i < 10; ++i) g.tick();
  TEST_ASSERT_EQUAL_UINT32(30, g.score());  // radius 3: 10 + 4 * 5
}

static void test_rock_hits_ship_then_invulnerable() {
  Game g = quietGame();
  g.addRockForTest(30, shipCentreY(g), 4, kSubpx);  // 1 px per tick
  bool hit = false;
  for (int i = 0; i < 20; ++i) {
    g.tick();
    hit = hit || g.shipHitLastTick();
  }
  TEST_ASSERT_TRUE(hit);
  TEST_ASSERT_EQUAL_UINT8(kStartLives - 1, g.lives());
  TEST_ASSERT_TRUE(g.invulnerable());
  // A second rock during the invulnerable second passes harmlessly.
  g.addRockForTest(20, shipCentreY(g), 4, kSubpx);
  for (int i = 0; i < 28; ++i) g.tick();
  TEST_ASSERT_EQUAL_UINT8(kStartLives - 1, g.lives());
  TEST_ASSERT_EQUAL_INT(0, activeRocks(g));
}

static void test_game_over_after_three_hits() {
  Game g = quietGame();
  for (int hit = 0; hit < kStartLives; ++hit) {
    g.addRockForTest(20, shipCentreY(g), 4, kSubpx);
    for (int i = 0; i < 60; ++i) g.tick();
  }
  TEST_ASSERT_TRUE(isOver(g));
  TEST_ASSERT_EQUAL_UINT8(0, g.lives());
  TEST_ASSERT_FALSE(g.fire());
  const uint32_t r = g.revision();
  g.tick();
  TEST_ASSERT_EQUAL_UINT32(r, g.revision());  // frozen
}

static void test_rocks_that_miss_leave_without_penalty() {
  Game g = quietGame();
  g.addRockForTest(100, 12, 3, 2 * kSubpx);  // far above the ship
  for (int i = 0; i < 60; ++i) g.tick();
  TEST_ASSERT_EQUAL_INT(0, activeRocks(g));
  TEST_ASSERT_EQUAL_UINT8(kStartLives, g.lives());
}

static void test_rocks_spawn_on_their_own_inside_the_field() {
  Game g(kSeed);
  for (uint32_t i = 0; i <= g.spawnIntervalTicks(); ++i) g.tick();
  TEST_ASSERT_TRUE(activeRocks(g) >= 1);
  for (int i = 0; i < kMaxRocks; ++i) {
    const Rock& r = g.rock(i);
    if (!r.active) continue;
    TEST_ASSERT_TRUE(r.radius >= 3 && r.radius <= 7);
    TEST_ASSERT_TRUE(r.y - r.radius >= kShipMinY);
    TEST_ASSERT_TRUE(r.y + r.radius <= kHeight - 1);
    TEST_ASSERT_TRUE(r.x() > kWidth - 10);
    TEST_ASSERT_TRUE(r.speed > 0);
  }
}

static void test_spawning_speeds_up_with_score() {
  Game g = quietGame();
  TEST_ASSERT_EQUAL_UINT32(60, g.spawnIntervalTicks());
  g.setScoreForTest(1000);
  TEST_ASSERT_EQUAL_UINT32(40, g.spawnIntervalTicks());
  g.setScoreForTest(100000);
  TEST_ASSERT_EQUAL_UINT32(15, g.spawnIntervalTicks());
}

static void test_reset_starts_over() {
  Game g = quietGame();
  g.addRockForTest(20, shipCentreY(g), 4, kSubpx);
  for (int i = 0; i < 60; ++i) g.tick();
  g.reset(kSeed + 1);
  TEST_ASSERT_EQUAL_UINT8(kStartLives, g.lives());
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT(0, activeRocks(g));
  TEST_ASSERT_FALSE(g.invulnerable());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_new_game);
  RUN_TEST(test_ship_moves_and_stays_on_screen);
  RUN_TEST(test_fire_spawns_at_nose_with_cooldown);
  RUN_TEST(test_bullet_flies_right_and_leaves);
  RUN_TEST(test_shot_destroys_rock_and_scores);
  RUN_TEST(test_smaller_rocks_score_more);
  RUN_TEST(test_rock_hits_ship_then_invulnerable);
  RUN_TEST(test_game_over_after_three_hits);
  RUN_TEST(test_rocks_that_miss_leave_without_penalty);
  RUN_TEST(test_rocks_spawn_on_their_own_inside_the_field);
  RUN_TEST(test_spawning_speeds_up_with_score);
  RUN_TEST(test_reset_starts_over);
  return UNITY_END();
}
