// Host unit tests for Chameleon (make test).
#include <unity.h>

#include "chameleon_core.h"

using namespace chameleon;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t kSeed = 99;

/// A game with no seekers, no newcomers, the player standing still at
/// (80,45), and object 0 a bush (radius 6, colour 2) at (100,45). The other
/// objects are parked far away in the top-left corner.
Game quietGame() {
  Game g(kSeed);
  g.clearSeekersForTest();
  g.setJoiningForTest(false);
  for (int i = 1; i < kObjects; ++i) g.setObjectForTest(i, 8, 16, 3, 0);
  g.setObjectForTest(0, 100, 45, 6, 2);
  g.setPlayerForTest(80, 45, 0, false);
  return g;
}

bool caught(const Game& g) { return g.state() == Game::State::kCaught; }

void ticks(Game& g, int n) {
  for (int i = 0; i < n; ++i) g.tick();
}

}  // namespace

static void test_heading_vectors() {
  int16_t dx, dy;
  headingVector(0, &dx, &dy);
  TEST_ASSERT_EQUAL_INT16(kVector, dx);
  TEST_ASSERT_EQUAL_INT16(0, dy);
  headingVector(4, &dx, &dy);  // down
  TEST_ASSERT_EQUAL_INT16(0, dx);
  TEST_ASSERT_EQUAL_INT16(kVector, dy);
  headingVector(10, &dx, &dy);  // up-left
  TEST_ASSERT_TRUE(dx < 0 && dy < 0);
}

static void test_new_game() {
  Game g(kSeed);
  TEST_ASSERT_FALSE(caught(g));
  TEST_ASSERT_EQUAL_UINT32(0, g.score());
  TEST_ASSERT_EQUAL_INT(kStartSeekers, g.activeSeekers());
  TEST_ASSERT_TRUE(g.walking());
  TEST_ASSERT_FALSE(g.camouflaged());
  for (int i = 0; i < kObjects; ++i) {
    const Object& o = g.object(i);
    TEST_ASSERT_TRUE(o.radius >= 4);
    TEST_ASSERT_TRUE(o.x - o.radius >= 0 && o.x + o.radius < kWidth);
    TEST_ASSERT_TRUE(o.y - o.radius >= kTopY && o.y + o.radius < kHeight);
    TEST_ASSERT_TRUE(o.color < kColors);
  }
}

static void test_turning_wraps() {
  Game g = quietGame();
  g.turn(-1);
  TEST_ASSERT_EQUAL_INT(kHeadings - 1, g.heading());
  g.turn(3);
  TEST_ASSERT_EQUAL_INT(2, g.heading());
}

static void test_walks_in_heading_direction_and_stops_at_walls() {
  Game g = quietGame();
  g.setPlayerForTest(80, 45, 4, true);  // down
  ticks(g, 16);                          // 16 x 10/16 px = 10 px
  TEST_ASSERT_EQUAL_INT16(80, g.playerX());
  TEST_ASSERT_EQUAL_INT16(55, g.playerY());
  ticks(g, 200);
  TEST_ASSERT_TRUE(g.playerY() <= kHeight - 3);  // held at the bottom edge
  g.toggleWalking();
  const int16_t y = g.playerY();
  g.turn(8);  // now facing up, but stopped
  ticks(g, 20);
  TEST_ASSERT_EQUAL_INT16(y, g.playerY());
}

static void test_camouflage_only_next_to_an_object() {
  Game g = quietGame();
  TEST_ASSERT_EQUAL_INT(-1, g.nearbyObject());  // 20 px from the bush centre, reach is 6 + 8
  TEST_ASSERT_FALSE(g.canCamouflage());
  TEST_ASSERT_FALSE(g.camouflage());
  g.setPlayerForTest(88, 45, 0, false);  // 12 px from the centre
  TEST_ASSERT_EQUAL_INT(0, g.nearbyObject());
  TEST_ASSERT_TRUE(g.canCamouflage());
  TEST_ASSERT_TRUE(g.camouflage());
  TEST_ASSERT_TRUE(g.camouflaged());
  TEST_ASSERT_EQUAL_UINT8(2, g.camoColor());
  TEST_ASSERT_FALSE(g.canCamouflage());  // already on
}

static void test_camouflage_lasts_then_recharges() {
  Game g = quietGame();
  g.setPlayerForTest(90, 45, 0, false);
  g.camouflage();
  ticks(g, kCamoTicks - 1);
  TEST_ASSERT_TRUE(g.camouflaged());
  g.tick();
  TEST_ASSERT_FALSE(g.camouflaged());
  TEST_ASSERT_EQUAL_UINT16(kCooldownTicks, g.cooldownTicks());
  TEST_ASSERT_FALSE(g.camouflage());  // recharging
  ticks(g, kCooldownTicks);
  TEST_ASSERT_TRUE(g.camouflage());
}

static void test_walking_away_ends_camouflage_early() {
  Game g = quietGame();
  g.setPlayerForTest(90, 45, 8, false);  // facing left, away from the bush
  g.camouflage();
  g.toggleWalking();
  ticks(g, 40);  // 25 px: well out of reach
  TEST_ASSERT_FALSE(g.camouflaged());
  // Camouflage broke partway through, and recharging started then.
  TEST_ASSERT_TRUE(g.cooldownTicks() > 0);
  TEST_ASSERT_TRUE(g.cooldownTicks() < kCooldownTicks);
}

static void test_seeker_sees_you_in_its_cone() {
  Game g = quietGame();
  g.addSeekerForTest(60, 45, kVector, 0);  // 20 px left of you, facing right
  g.tick();
  TEST_ASSERT_TRUE(caught(g));
}

static void test_seeker_facing_away_or_too_far_does_not() {
  Game g = quietGame();
  g.addSeekerForTest(60, 45, -kVector, 0);  // facing away
  g.addSeekerForTest(30, 45, kVector, 0);   // facing you, 50 px away
  g.addSeekerForTest(80, 20, kVector, 0);   // 25 px above, looking sideways
  g.tick();
  TEST_ASSERT_FALSE(caught(g));
}

static void test_too_close_is_caught_whatever_the_facing() {
  Game g = quietGame();
  g.addSeekerForTest(76, 45, -kVector, 0);  // 4 px away, facing away
  g.tick();
  TEST_ASSERT_TRUE(caught(g));
}

static void test_camouflage_hides_you_from_a_seeker() {
  Game g = quietGame();
  g.setPlayerForTest(90, 45, 0, false);
  g.camouflage();
  g.addSeekerForTest(70, 45, kVector, 0);  // 20 px away, looking right at you
  ticks(g, 50);
  TEST_ASSERT_FALSE(caught(g));
  ticks(g, kCamoTicks);  // camouflage wears off
  TEST_ASSERT_TRUE(caught(g));
}

static void test_score_is_seconds_survived_and_freezes_when_caught() {
  Game g = quietGame();
  ticks(g, 250);
  TEST_ASSERT_EQUAL_UINT32(5, g.score());
  g.addSeekerForTest(60, 45, kVector, 0);
  g.tick();
  TEST_ASSERT_TRUE(caught(g));
  const uint32_t s = g.score();
  const uint32_t r = g.revision();
  ticks(g, 500);
  TEST_ASSERT_EQUAL_UINT32(s, g.score());
  TEST_ASSERT_EQUAL_UINT32(r, g.revision());
}

static void test_a_seeker_joins_every_twenty_seconds() {
  Game g = quietGame();
  g.setJoiningForTest(true);
  ticks(g, kSeekerJoinTicks - 1);
  TEST_ASSERT_EQUAL_INT(0, g.activeSeekers());
  g.tick();
  TEST_ASSERT_EQUAL_INT(1, g.activeSeekers());
  TEST_ASSERT_FALSE(caught(g));  // newcomers start at the edge, out of sight
}

static void test_no_more_than_the_max_seekers() {
  Game g = quietGame();
  // Fill every slot with seekers in the corners, looking out of the field.
  for (int i = 0; i < kMaxSeekers; ++i) {
    g.addSeekerForTest(static_cast<int16_t>(4 + i), 14, -kVector, 0);
  }
  g.setJoiningForTest(true);
  ticks(g, kSeekerJoinTicks);
  TEST_ASSERT_EQUAL_INT(kMaxSeekers, g.activeSeekers());
  TEST_ASSERT_FALSE(caught(g));
}

static void test_wandering_seekers_stay_on_the_field() {
  Game g(kSeed);
  g.setPlayerForTest(80, 45, 0, false);
  for (int t = 0; t < 2000 && !caught(g); ++t) {
    g.tick();
    for (int i = 0; i < kMaxSeekers; ++i) {
      const Seeker& s = g.seeker(i);
      if (!s.active) continue;
      TEST_ASSERT_TRUE(s.x() >= 0 && s.x() < kWidth);
      TEST_ASSERT_TRUE(s.y() >= kTopY && s.y() < kHeight);
    }
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_heading_vectors);
  RUN_TEST(test_new_game);
  RUN_TEST(test_turning_wraps);
  RUN_TEST(test_walks_in_heading_direction_and_stops_at_walls);
  RUN_TEST(test_camouflage_only_next_to_an_object);
  RUN_TEST(test_camouflage_lasts_then_recharges);
  RUN_TEST(test_walking_away_ends_camouflage_early);
  RUN_TEST(test_seeker_sees_you_in_its_cone);
  RUN_TEST(test_seeker_facing_away_or_too_far_does_not);
  RUN_TEST(test_too_close_is_caught_whatever_the_facing);
  RUN_TEST(test_camouflage_hides_you_from_a_seeker);
  RUN_TEST(test_score_is_seconds_survived_and_freezes_when_caught);
  RUN_TEST(test_a_seeker_joins_every_twenty_seconds);
  RUN_TEST(test_no_more_than_the_max_seekers);
  RUN_TEST(test_wandering_seekers_stay_on_the_field);
  return UNITY_END();
}
