#include "chameleon_core.h"

namespace chameleon {
namespace {

// 16 headings, 22.5 degrees apart, as vectors of length 64 (y down, so
// heading 4 points down and turning +1 turns clockwise on screen).
const int16_t kHeadingVectors[kHeadings][2] = {
    {64, 0},   {59, 24},   {45, 45},   {24, 59},   {0, 64},  {-24, 59}, {-45, 45}, {-59, 24},
    {-64, 0}, {-59, -24}, {-45, -45}, {-24, -59}, {0, -64}, {24, -59}, {45, -45}, {59, -24},
};

// Where seekers appear: the corners and the top middle. Each newcomer uses
// the point farthest from the player.
const int16_t kSpawnPoints[][2] = {{6, 14}, {154, 14}, {6, 76}, {154, 76}, {80, 14}};

constexpr int16_t kCos45x64 = 45;  // cos(45 deg) * kVector: the cone's edge
constexpr int16_t kEdge = 3;       // px the player keeps from the screen edges
constexpr int16_t kTargetMargin = 4;

int32_t squared(int32_t v) { return v * v; }

uint32_t isqrt(uint32_t v) {
  uint32_t r = 0;
  uint32_t bit = 1u << 30;
  while (bit > v) bit >>= 2;
  while (bit != 0) {
    if (v >= r + bit) {
      v -= r + bit;
      r = (r >> 1) + bit;
    } else {
      r >>= 1;
    }
    bit >>= 2;
  }
  return r;
}

int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

void headingVector(int heading, int16_t* dx, int16_t* dy) {
  const int h = ((heading % kHeadings) + kHeadings) % kHeadings;
  *dx = kHeadingVectors[h][0];
  *dy = kHeadingVectors[h][1];
}

Game::Game(uint32_t seed) : rng_(seed) { reset(seed); }

void Game::reset(uint32_t seed) {
  rng_ = Rng(seed);
  px_ = static_cast<int32_t>(kWidth / 2) * kSubpx;
  py_ = static_cast<int32_t>((kTopY + kHeight) / 2) * kSubpx;
  heading_ = 0;
  walking_ = true;
  camo_ticks_ = 0;
  cooldown_ = 0;
  camo_object_ = -1;
  camo_color_ = 0;
  ticks_ = 0;
  state_ = State::kPlaying;
  for (Seeker& s : seekers_) s = {false, 0, 0, 0, 0, 0, 0, false};
  placeObjects();
  for (int i = 0; i < kStartSeekers; ++i) {
    addSeeker(kSpawnPoints[i][0], kSpawnPoints[i][1], false);
  }
  ++revision_;
}

void Game::placeObjects() {
  for (int i = 0; i < kObjects; ++i) {
    Object o = {0, 0, 0, 0};
    for (int attempt = 0; attempt < 60; ++attempt) {
      const uint8_t r = static_cast<uint8_t>(5 + rng_.below(4));
      o.radius = r;
      o.x = static_cast<int16_t>(r + 2 + rng_.below(static_cast<uint32_t>(kWidth - 2 * r - 4)));
      o.y = static_cast<int16_t>(kTopY + r + 2 + rng_.below(static_cast<uint32_t>(kHeight - kTopY - 2 * r - 4)));
      o.color = static_cast<uint8_t>(rng_.below(kColors));
      // Keep clear of the player's start, the other objects and the spawn points.
      bool ok = squared(o.x - playerX()) + squared(o.y - playerY()) > squared(r + 14);
      for (int j = 0; ok && j < i; ++j) {
        ok = squared(o.x - objects_[j].x) + squared(o.y - objects_[j].y) > squared(r + objects_[j].radius + 6);
      }
      for (const auto& p : kSpawnPoints) {
        ok = ok && squared(o.x - p[0]) + squared(o.y - p[1]) > squared(r + 6);
      }
      if (ok) break;
    }
    objects_[i] = o;
  }
}

void Game::addSeeker(int16_t x, int16_t y, bool stationary) {
  for (Seeker& s : seekers_) {
    if (s.active) continue;
    s = {true, static_cast<int32_t>(x) * kSubpx, static_cast<int32_t>(y) * kSubpx, x, y, kVector, 0, stationary};
    if (!stationary) pickTarget(s);
    return;
  }
}

void Game::pickTarget(Seeker& s) {
  s.target_x = static_cast<int16_t>(kTargetMargin + rng_.below(kWidth - 2 * kTargetMargin));
  s.target_y = static_cast<int16_t>(kTopY + kTargetMargin + rng_.below(kHeight - kTopY - 2 * kTargetMargin));
}

void Game::turn(int32_t steps) {
  if (state_ != State::kPlaying || steps == 0) return;
  heading_ = static_cast<int>(((heading_ + steps) % kHeadings + kHeadings) % kHeadings);
  ++revision_;
}

void Game::toggleWalking() {
  if (state_ != State::kPlaying) return;
  walking_ = !walking_;
  ++revision_;
}

int Game::nearbyObject() const {
  int best = -1;
  int32_t best_d2 = 0;
  for (int i = 0; i < kObjects; ++i) {
    const Object& o = objects_[i];
    const int32_t d2 = squared(playerX() - o.x) + squared(playerY() - o.y);
    if (d2 <= squared(o.radius + kCamoReach) && (best < 0 || d2 < best_d2)) {
      best = i;
      best_d2 = d2;
    }
  }
  return best;
}

bool Game::canCamouflage() const {
  return state_ == State::kPlaying && camo_ticks_ == 0 && cooldown_ == 0 && nearbyObject() >= 0;
}

bool Game::camouflage() {
  if (!canCamouflage()) return false;
  camo_object_ = nearbyObject();
  camo_color_ = objects_[camo_object_].color;
  camo_ticks_ = kCamoTicks;
  ++revision_;
  return true;
}

int Game::activeSeekers() const {
  int n = 0;
  for (const Seeker& s : seekers_) n += s.active ? 1 : 0;
  return n;
}

bool Game::seenBy(const Seeker& s) const {
  const int32_t vx = playerX() - s.x();
  const int32_t vy = playerY() - s.y();
  const int32_t d2 = squared(vx) + squared(vy);
  if (d2 < squared(kTouchRange)) return true;
  if (d2 > squared(kSightRange)) return false;
  // Inside the 90 degree cone when the angle to the player is within 45
  // degrees of the facing: dot(facing, v) >= |v| * cos(45) * |facing|.
  const int32_t dot = vx * s.face_x + vy * s.face_y;
  return dot >= static_cast<int32_t>(isqrt(static_cast<uint32_t>(d2))) * kCos45x64;
}

void Game::tick() {
  if (state_ != State::kPlaying) return;
  ++ticks_;

  if (walking_) {
    int16_t dx, dy;
    headingVector(heading_, &dx, &dy);
    px_ = clamp(px_ + dx * kPlayerSpeed / kVector, kEdge * kSubpx, (kWidth - kEdge) * kSubpx);
    py_ = clamp(py_ + dy * kPlayerSpeed / kVector, (kTopY + kEdge) * kSubpx, (kHeight - kEdge) * kSubpx);
  }

  // Camouflage only works by the object it was taken from.
  if (camo_ticks_ > 0) {
    const Object& o = objects_[camo_object_];
    const bool still_near = squared(playerX() - o.x) + squared(playerY() - o.y) <= squared(o.radius + kCamoReach);
    camo_ticks_ = still_near ? static_cast<uint16_t>(camo_ticks_ - 1) : 0;
    if (camo_ticks_ == 0) cooldown_ = kCooldownTicks;
  } else if (cooldown_ > 0) {
    --cooldown_;
  }

  for (Seeker& s : seekers_) {
    if (!s.active || s.stationary) continue;
    const int32_t dx = static_cast<int32_t>(s.target_x) * kSubpx - s.x_subpx;
    const int32_t dy = static_cast<int32_t>(s.target_y) * kSubpx - s.y_subpx;
    const int32_t dist = static_cast<int32_t>(isqrt(static_cast<uint32_t>(squared(dx) + squared(dy))));
    if (dist <= kSeekerSpeed) {
      s.x_subpx = static_cast<int32_t>(s.target_x) * kSubpx;
      s.y_subpx = static_cast<int32_t>(s.target_y) * kSubpx;
      pickTarget(s);
    } else {
      s.x_subpx += dx * kSeekerSpeed / dist;
      s.y_subpx += dy * kSeekerSpeed / dist;
      s.face_x = static_cast<int16_t>(dx * kVector / dist);
      s.face_y = static_cast<int16_t>(dy * kVector / dist);
    }
  }

  if (camo_ticks_ == 0) {
    for (const Seeker& s : seekers_) {
      if (s.active && seenBy(s)) {
        state_ = State::kCaught;
        ++revision_;
        return;
      }
    }
  }

  if (joining_ && ticks_ % kSeekerJoinTicks == 0 && activeSeekers() < kMaxSeekers) {
    int best = 0;
    int32_t best_d2 = -1;
    for (int i = 0; i < static_cast<int>(sizeof(kSpawnPoints) / sizeof(kSpawnPoints[0])); ++i) {
      const int32_t d2 = squared(kSpawnPoints[i][0] - playerX()) + squared(kSpawnPoints[i][1] - playerY());
      if (d2 > best_d2) {
        best = i;
        best_d2 = d2;
      }
    }
    addSeeker(kSpawnPoints[best][0], kSpawnPoints[best][1], false);
  }
  ++revision_;
}

void Game::setPlayerForTest(int16_t x, int16_t y, int heading, bool walking) {
  px_ = static_cast<int32_t>(x) * kSubpx;
  py_ = static_cast<int32_t>(y) * kSubpx;
  heading_ = heading;
  walking_ = walking;
  ++revision_;
}

void Game::setObjectForTest(int i, int16_t x, int16_t y, uint8_t radius, uint8_t color) {
  objects_[i] = {x, y, radius, color};
}

void Game::clearSeekersForTest() {
  for (Seeker& s : seekers_) s.active = false;
}

void Game::addSeekerForTest(int16_t x, int16_t y, int16_t face_x, int16_t face_y) {
  for (Seeker& s : seekers_) {
    if (s.active) continue;
    s = {true, static_cast<int32_t>(x) * kSubpx, static_cast<int32_t>(y) * kSubpx, x, y, face_x, face_y, true};
    return;
  }
}

}  // namespace chameleon
