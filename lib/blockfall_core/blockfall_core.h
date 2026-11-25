#pragma once

#include <stdint.h>

/// Game rules for blockfall, a falling-block puzzle: pieces of four squares
/// fall into a 10x20 well; full rows clear and score; the game ends when a new
/// piece has no room. Pure logic (no Arduino), so it is unit-tested on the host.
///
/// Coordinates: x = column 0..9 left to right, y = row 0..19 top to bottom.
namespace blockfall {

constexpr int kCols = 10;
constexpr int kRows = 20;
constexpr int kPieceTypes = 7;

enum class PieceType : uint8_t { kI, kO, kT, kS, kZ, kJ, kL };

struct Cell {
  int8_t x;
  int8_t y;
};

/// The four squares of `type` at `rotation` (0-3, each a quarter turn
/// clockwise), relative to the top-left corner of the piece's bounding box.
void pieceCells(PieceType type, uint8_t rotation, Cell out[4]);

/// Small deterministic random generator (xorshift32). Same seed, same numbers,
/// which keeps the tests repeatable; the board seeds it from hardware.
class Random {
 public:
  explicit Random(uint32_t seed);
  uint32_t next();

 private:
  uint32_t state_;
};

/// Deals pieces in shuffled sets of all seven, so no type is ever missing for
/// long and none repeats more than twice in a row.
class Bag {
 public:
  explicit Bag(uint32_t seed);
  PieceType next();
  PieceType peek() const;  // the piece next() will return

 private:
  void refill();

  Random rng_;
  PieceType items_[kPieceTypes];
  uint8_t index_ = kPieceTypes;
};

/// The falling piece: its type, rotation and the top-left of its box.
struct Piece {
  PieceType type;
  uint8_t rotation;
  int8_t x;
  int8_t y;
};

class Game {
 public:
  enum class State : uint8_t { kPlaying, kGameOver };

  explicit Game(uint32_t seed);

  /// Empty the well and start a new game.
  void reset(uint32_t seed);

  /// Player moves. Each returns false (and changes nothing) if blocked or if
  /// the game is over. rotate() tries small sideways shifts ("wall kicks")
  /// when the plain rotation doesn't fit.
  bool moveLeft();
  bool moveRight();
  bool rotate();

  /// One step of gravity: move the piece down a row, or lock it if it can't
  /// move. soft_drop = the player is holding drop (1 point per row). Returns
  /// true if the piece locked.
  bool tick(bool soft_drop = false);

  /// Drop the piece straight to the bottom and lock it (2 points per row).
  void hardDrop();

  State state() const { return state_; }
  uint32_t score() const { return score_; }
  uint32_t lines() const { return lines_; }
  uint32_t level() const { return 1 + lines_ / 10; }

  /// Time between gravity steps at the current level: 800 ms at level 1,
  /// 70 ms faster per level, never below 100 ms.
  uint32_t fallIntervalMs() const;

  /// Locked squares: 0 = empty, otherwise the PieceType + 1 that left it.
  uint8_t cell(int x, int y) const;

  const Piece& piece() const { return piece_; }
  PieceType nextType() const { return bag_.peek(); }

  /// Rows cleared by the most recent lock (0-4), for sound and light effects.
  uint8_t lastCleared() const { return last_cleared_; }

  /// Increases whenever anything visible changes, so the screen redraws only
  /// when needed.
  uint32_t revision() const { return revision_; }

  // Test hooks: set up exact board situations in unit tests.
  void setCellForTest(int x, int y, uint8_t value);
  void setPieceForTest(PieceType type, uint8_t rotation, int x, int y);

 private:
  bool fits(PieceType type, uint8_t rotation, int x, int y) const;
  void lock();
  void spawn();
  uint8_t clearLines();

  uint8_t board_[kRows][kCols];
  Bag bag_;
  Piece piece_;
  State state_ = State::kPlaying;
  uint32_t score_ = 0;
  uint32_t lines_ = 0;
  uint8_t last_cleared_ = 0;
  uint32_t revision_ = 0;
};

}  // namespace blockfall
