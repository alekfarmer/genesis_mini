#pragma once

#include <stdint.h>

#include "rng.h"

/// Rules for Chameleon: you are a small creature walking around a field with
/// coloured objects (bushes, rocks, flowers). Computer seekers wander the
/// field, each looking ahead in a 90 degree cone. If one sees you, you're
/// caught. Next to an object you can camouflage: take its colour for 3 s,
/// during which seekers can't see you as long as you stay by that object.
/// Then camouflage needs 3 s to recharge. More seekers join over time; the
/// score is the number of seconds survived.
///
/// Fixed 20 ms ticks. Screen coordinates in pixels, 160x80, y down; the top
/// 10 px are kept free for the HUD. Pure logic (no Arduino), host-tested.
namespace chameleon {

constexpr int16_t kWidth = 160;
constexpr int16_t kHeight = 80;
constexpr int16_t kTopY = 10;
constexpr uint32_t kTickMs = 20;
constexpr int kHeadings = 16;  // 22.5 degree steps; 0 = right, 4 = down
constexpr int kObjects = 7;
constexpr int kColors = 6;
constexpr int kStartSeekers = 2;
constexpr int kMaxSeekers = 5;
constexpr int32_t kSubpx = 16;               // positions are kept in 1/16 px
constexpr int32_t kPlayerSpeed = 10;         // 1/16 px per tick (~31 px/s)
constexpr int32_t kSeekerSpeed = 7;          // 1/16 px per tick (~22 px/s)
constexpr int16_t kSightRange = 34;          // px
constexpr int16_t kTouchRange = 6;           // px: caught this close, whatever the facing
constexpr int16_t kCamoReach = 8;            // px beyond an object's edge
constexpr uint16_t kCamoTicks = 150;         // 3 s
constexpr uint16_t kCooldownTicks = 150;     // 3 s
constexpr uint32_t kSeekerJoinTicks = 1000;  // a new seeker every 20 s
constexpr int16_t kVector = 64;              // length of heading/facing vectors

struct Object {
  int16_t x;
  int16_t y;
  uint8_t radius;
  uint8_t color;  // 0 .. kColors-1
};

struct Seeker {
  bool active;
  int32_t x_subpx;
  int32_t y_subpx;
  int16_t target_x;  // px; it walks towards this, then picks another
  int16_t target_y;
  int16_t face_x;    // facing direction, length ~kVector
  int16_t face_y;
  bool stationary;   // test hook: doesn't move

  int16_t x() const { return static_cast<int16_t>(x_subpx / kSubpx); }
  int16_t y() const { return static_cast<int16_t>(y_subpx / kSubpx); }
};

/// Unit-ish vector (length kVector) for heading 0..15.
void headingVector(int heading, int16_t* dx, int16_t* dy);

class Game {
 public:
  enum class State : uint8_t { kPlaying, kCaught };

  explicit Game(uint32_t seed);
  void reset(uint32_t seed);

  /// Turn by knob clicks (+ = clockwise), wrapping around.
  void turn(int32_t steps);

  /// Stop / start walking.
  void toggleWalking();
  bool walking() const { return walking_; }

  /// Try to camouflage. Works only next to an object, when not already
  /// camouflaged and not recharging. Returns true if it worked.
  bool camouflage();

  /// True if camouflage() would work right now (for the button's LED).
  bool canCamouflage() const;

  /// Advance one 20 ms tick.
  void tick();

  State state() const { return state_; }
  uint32_t score() const { return ticks_ / 50; }  // seconds survived
  int16_t playerX() const { return static_cast<int16_t>(px_ / kSubpx); }
  int16_t playerY() const { return static_cast<int16_t>(py_ / kSubpx); }
  int heading() const { return heading_; }
  bool camouflaged() const { return camo_ticks_ > 0; }
  uint16_t camoTicksLeft() const { return camo_ticks_; }
  uint16_t cooldownTicks() const { return cooldown_; }
  uint8_t camoColor() const { return camo_color_; }
  const Object& object(int i) const { return objects_[i]; }
  const Seeker& seeker(int i) const { return seekers_[i]; }
  int activeSeekers() const;

  /// Index of the object within reach of the player, or -1.
  int nearbyObject() const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

  // Test hooks.
  void setPlayerForTest(int16_t x, int16_t y, int heading, bool walking);
  void setObjectForTest(int i, int16_t x, int16_t y, uint8_t radius, uint8_t color);
  void clearSeekersForTest();
  void addSeekerForTest(int16_t x, int16_t y, int16_t face_x, int16_t face_y);
  void setJoiningForTest(bool enabled) { joining_ = enabled; }

 private:
  void placeObjects();
  void addSeeker(int16_t x, int16_t y, bool stationary);
  void pickTarget(Seeker& s);
  bool seenBy(const Seeker& s) const;

  Rng rng_;
  Object objects_[kObjects];
  Seeker seekers_[kMaxSeekers];
  int32_t px_ = 0;
  int32_t py_ = 0;
  int heading_ = 0;
  bool walking_ = true;
  uint16_t camo_ticks_ = 0;
  uint16_t cooldown_ = 0;
  int camo_object_ = -1;
  uint8_t camo_color_ = 0;
  uint32_t ticks_ = 0;
  bool joining_ = true;
  State state_ = State::kPlaying;
  uint32_t revision_ = 0;
};

}  // namespace chameleon
