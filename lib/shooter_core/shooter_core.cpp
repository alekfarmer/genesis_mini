#include "shooter_core.h"

namespace shooter {
namespace {

constexpr uint8_t kMinRadius = 3;
constexpr uint8_t kMaxRadius = 7;
constexpr uint32_t kFirstSpawnTicks = 60;
constexpr uint32_t kFastestSpawnTicks = 15;
constexpr uint32_t kPointsPerSpawnSpeedup = 50;
constexpr int16_t kBaseRockSpeed = 12;      // 1/16 px per tick (0.75 px)
constexpr uint32_t kRockSpeedSpread = 16;   // + up to 1 px per tick
constexpr int16_t kMaxRockSpeedBonus = 24;  // + up to 1.5 px per tick with score

int32_t squared(int32_t v) { return v * v; }

}  // namespace

Game::Game(uint32_t seed) : rng_(seed) { reset(seed); }

void Game::reset(uint32_t seed) {
  rng_ = Rng(seed);
  for (Bullet& b : bullets_) b = {false, 0, 0};
  for (Rock& r : rocks_) r = {false, 0, 0, 0, 0};
  state_ = State::kPlaying;
  score_ = 0;
  lives_ = kStartLives;
  ship_y_ = (kShipMinY + kShipMaxY) / 2;
  cooldown_ = 0;
  invulnerable_ = 0;
  spawn_timer_ = spawnIntervalTicks();
  rocks_hit_ = 0;
  ship_hit_ = false;
  ++revision_;
}

void Game::moveShip(int32_t steps) {
  if (state_ != State::kPlaying || steps == 0) {
    return;
  }
  int32_t y = ship_y_ - steps * kShipStepPx;
  if (y < kShipMinY) y = kShipMinY;
  if (y > kShipMaxY) y = kShipMaxY;
  if (y != ship_y_) {
    ship_y_ = static_cast<int16_t>(y);
    ++revision_;
  }
}

bool Game::fire() {
  if (state_ != State::kPlaying || cooldown_ > 0) {
    return false;
  }
  for (Bullet& b : bullets_) {
    if (!b.active) {
      b = {true, kShipNoseX, static_cast<int16_t>(ship_y_ + kShipHeight / 2)};
      cooldown_ = kFireCooldownTicks;
      ++revision_;
      return true;
    }
  }
  return false;
}

void Game::tick() {
  if (state_ != State::kPlaying) {
    return;
  }
  rocks_hit_ = 0;
  ship_hit_ = false;
  if (cooldown_ > 0) --cooldown_;
  if (invulnerable_ > 0) --invulnerable_;

  for (Bullet& b : bullets_) {
    if (!b.active) continue;
    b.x = static_cast<int16_t>(b.x + kBulletSpeedPx);
    if (b.x >= kWidth) b.active = false;
  }
  for (Rock& r : rocks_) {
    if (!r.active) continue;
    r.x_subpx -= r.speed;
    if (r.x() + r.radius < 0) r.active = false;  // fully off the left edge
  }

  // Shots: a hit when the bullet's point is inside the rock's circle.
  for (Bullet& b : bullets_) {
    if (!b.active) continue;
    for (Rock& r : rocks_) {
      if (!r.active) continue;
      if (squared(b.x - r.x()) + squared(b.y - r.y) <= squared(r.radius)) {
        b.active = false;
        r.active = false;
        score_ += 10 + static_cast<uint32_t>(kMaxRadius - r.radius) * 5;
        ++rocks_hit_;
        break;
      }
    }
  }

  // The ship: circles touching. While invulnerable, rocks pass through.
  if (invulnerable_ == 0) {
    const int32_t cx = kShipX + 6;
    const int32_t cy = ship_y_ + kShipHeight / 2;
    for (Rock& r : rocks_) {
      if (!r.active) continue;
      if (squared(cx - r.x()) + squared(cy - r.y) <= squared(r.radius + kShipRadius)) {
        r.active = false;
        --lives_;
        ship_hit_ = true;
        invulnerable_ = kInvulnerableTicks;
        if (lives_ == 0) state_ = State::kGameOver;
        break;  // at most one hit per tick
      }
    }
  }

  if (spawning_ && state_ == State::kPlaying) {
    if (spawn_timer_ > 0) --spawn_timer_;
    if (spawn_timer_ == 0) {
      spawnRock();
      spawn_timer_ = spawnIntervalTicks();
    }
  }
  ++revision_;
}

uint32_t Game::spawnIntervalTicks() const {
  const uint32_t speedup = score_ / kPointsPerSpawnSpeedup;
  return speedup >= kFirstSpawnTicks - kFastestSpawnTicks ? kFastestSpawnTicks : kFirstSpawnTicks - speedup;
}

void Game::addRockForTest(int16_t x, int16_t y, uint8_t radius, int16_t speed) {
  addRock(static_cast<int32_t>(x) * kSubpx, y, radius, speed);
}

void Game::spawnRock() {
  const uint8_t radius = static_cast<uint8_t>(kMinRadius + rng_.below(kMaxRadius - kMinRadius + 1));
  // Keep the whole rock below the score line and above the bottom edge.
  const int16_t min_y = static_cast<int16_t>(kShipMinY + radius);
  const int16_t max_y = static_cast<int16_t>(kHeight - 1 - radius);
  const int16_t y = static_cast<int16_t>(min_y + rng_.below(static_cast<uint32_t>(max_y - min_y + 1)));
  int16_t bonus = static_cast<int16_t>((score_ / 100) * 2);
  if (bonus > kMaxRockSpeedBonus) bonus = kMaxRockSpeedBonus;
  const int16_t speed = static_cast<int16_t>(kBaseRockSpeed + rng_.below(kRockSpeedSpread) + bonus);
  addRock(static_cast<int32_t>(kWidth + radius) * kSubpx, y, radius, speed);
}

void Game::addRock(int32_t x_subpx, int16_t y, uint8_t radius, int16_t speed) {
  for (Rock& r : rocks_) {
    if (!r.active) {
      r = {true, x_subpx, y, radius, speed};
      ++revision_;
      return;
    }
  }
}

}  // namespace shooter
