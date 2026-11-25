#pragma once

#include <stdint.h>

#include "rng.h"

/// Game rules for the shooter: a ship on the left edge moves up and down and
/// fires to the right; asteroids drift in from the right. Hitting an asteroid
/// with a shot scores (smaller rocks score more); an asteroid reaching the
/// ship costs a life, followed by one second of invulnerability. Three lives.
/// Rocks get faster and more frequent as the score rises.
///
/// The game advances in fixed 20 ms ticks (call tick() 50 times a second).
/// Screen coordinates in pixels, 160x80, y down; the top 10 px are kept free
/// for the score. Pure logic (no Arduino), host-tested.
namespace shooter {

constexpr int16_t kWidth = 160;
constexpr int16_t kHeight = 80;
constexpr uint32_t kTickMs = 20;
constexpr int kMaxBullets = 6;
constexpr int kMaxRocks = 8;
constexpr int kStartLives = 3;

// Ship geometry: 12 x 8 px with its nose at x = kShipNoseX. It is hit when a
// rock's circle touches a circle of radius kShipRadius around its centre.
constexpr int16_t kShipX = 4;
constexpr int16_t kShipNoseX = 16;
constexpr int16_t kShipHeight = 8;
constexpr int16_t kShipRadius = 4;
constexpr int16_t kShipMinY = 10;  // top of the ship, below the score line
constexpr int16_t kShipMaxY = kHeight - kShipHeight;
constexpr int16_t kShipStepPx = 4;  // per knob click

constexpr int16_t kBulletSpeedPx = 4;     // per tick
constexpr uint8_t kFireCooldownTicks = 7;  // 140 ms between shots
constexpr uint8_t kInvulnerableTicks = 50; // 1 s after being hit
constexpr int16_t kSubpx = 16;             // rock x positions are in 1/16 px

struct Bullet {
  bool active;
  int16_t x;
  int16_t y;
};

struct Rock {
  bool active;
  int32_t x_subpx;  // centre x in 1/16 px (smooth slow movement)
  int16_t y;        // centre y in px
  uint8_t radius;   // 3..7 px
  int16_t speed;    // 1/16 px per tick, leftwards

  int16_t x() const { return static_cast<int16_t>(x_subpx / kSubpx); }
};

class Game {
 public:
  enum class State : uint8_t { kPlaying, kGameOver };

  explicit Game(uint32_t seed);
  void reset(uint32_t seed);

  /// Move the ship by knob clicks: + = up, 4 px per click, kept on screen.
  void moveShip(int32_t steps);

  /// Fire a shot from the nose. Returns false if still cooling down, all
  /// shots are in flight, or the game is over.
  bool fire();

  /// Advance one 20 ms tick: move everything, resolve hits, spawn rocks.
  void tick();

  State state() const { return state_; }
  uint32_t score() const { return score_; }
  uint8_t lives() const { return lives_; }
  int16_t shipY() const { return ship_y_; }  // top of the ship
  bool invulnerable() const { return invulnerable_ > 0; }
  const Bullet& bullet(int i) const { return bullets_[i]; }
  const Rock& rock(int i) const { return rocks_[i]; }

  /// What happened in the last tick, for sound effects.
  uint8_t rocksHitLastTick() const { return rocks_hit_; }
  bool shipHitLastTick() const { return ship_hit_; }

  /// Ticks between new rocks at the current score: 60 at the start,
  /// one fewer per 50 points, never below 15.
  uint32_t spawnIntervalTicks() const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

  // Test hooks.
  void setSpawningForTest(bool enabled) { spawning_ = enabled; }
  void setScoreForTest(uint32_t score) { score_ = score; }
  void addRockForTest(int16_t x, int16_t y, uint8_t radius, int16_t speed);

 private:
  void spawnRock();
  void addRock(int32_t x_subpx, int16_t y, uint8_t radius, int16_t speed);

  Rng rng_;
  Bullet bullets_[kMaxBullets];
  Rock rocks_[kMaxRocks];
  State state_ = State::kPlaying;
  uint32_t score_ = 0;
  uint8_t lives_ = kStartLives;
  int16_t ship_y_ = 0;
  uint8_t cooldown_ = 0;
  uint8_t invulnerable_ = 0;
  uint32_t spawn_timer_ = 0;
  bool spawning_ = true;
  uint8_t rocks_hit_ = 0;
  bool ship_hit_ = false;
  uint32_t revision_ = 0;
};

}  // namespace shooter
