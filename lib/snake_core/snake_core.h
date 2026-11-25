#pragma once

#include <stdint.h>

#include "rng.h"

/// Game rules for snake: the snake moves one square per step on a 32x14
/// grid; the knob turns it left or right relative to where it's heading.
/// Eating food makes it one square longer and a little faster. Hitting a wall
/// or itself ends the game. Pure logic (no Arduino), host-tested.
///
/// Coordinates: x = column 0..31 left to right, y = row 0..13 top to bottom.
namespace snake {

constexpr int kCols = 32;
constexpr int kRows = 14;
constexpr int kMaxLength = kCols * kRows;
constexpr int kStartLength = 3;

enum class Dir : uint8_t { kUp, kRight, kDown, kLeft };

struct Cell {
  int8_t x;
  int8_t y;
};

class Game {
 public:
  enum class State : uint8_t { kPlaying, kGameOver };

  explicit Game(uint32_t seed);

  /// Start a new game: length 3 in the middle, heading right.
  void reset(uint32_t seed);

  /// Queue a quarter turn, applied at the next step. Up to two turns can be
  /// queued, so quick double turns aren't lost; more are ignored.
  void turnLeft();
  void turnRight();

  /// Move one square. Returns true if the snake ate this step. Does nothing
  /// once the game is over.
  bool step();

  State state() const { return state_; }
  uint32_t score() const { return score_; }  // food eaten
  uint16_t length() const { return length_; }
  Dir direction() const { return dir_; }
  Cell food() const { return food_; }

  /// Segment i of the snake: 0 is the head, length()-1 the tail.
  Cell segment(uint16_t i) const;

  /// True if any segment is on (x, y).
  bool occupies(int x, int y) const;

  /// Time between steps: 180 ms at the start, 5 ms faster per food eaten,
  /// never below 70 ms.
  uint32_t stepIntervalMs() const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

  // Test hook: put the food on a chosen square.
  void setFoodForTest(int x, int y);

 private:
  void placeFood();

  Rng rng_;
  Cell body_[kMaxLength];  // ring buffer; body_[head_] is the head
  uint16_t head_ = 0;
  uint16_t length_ = 0;
  Dir dir_ = Dir::kRight;
  int8_t turns_[2] = {0, 0};  // queued turns: +1 right, -1 left
  uint8_t turn_count_ = 0;
  Cell food_ = {0, 0};
  State state_ = State::kPlaying;
  uint32_t score_ = 0;
  uint32_t revision_ = 0;
};

}  // namespace snake
