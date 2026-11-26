#pragma once

#include <stdint.h>

/// Rules for Hide & Seek, a two-player pass-the-board game on a map with 12
/// fixed hiding spots.
///
/// Each round, the hider secretly picks a spot, then hands over the board.
/// The seeker searches spots one at a time. Each miss gives a hint (hot /
/// warm / cold, by distance to the hiding spot). Finding the hider within 6
/// searches costs the seeker that many points; not finding them costs 7.
/// Players swap roles every round; after 4 rounds (each player seeks twice)
/// the lower total wins. Pure logic (no Arduino), host-tested.
namespace hideseek {

constexpr int kSpots = 12;
constexpr int kMaxSearches = 6;
constexpr int kMissPenalty = kMaxSearches + 1;
constexpr int kRounds = 4;
constexpr int kHotDistance = 30;   // px: closer than this is "hot"
constexpr int kWarmDistance = 60;  // px: closer than this is "warm"

enum class Kind : uint8_t { kTree, kBush, kRock, kShed, kLog, kWell };

/// A hiding spot: its centre on the 160x80 map, and what's drawn there.
struct Spot {
  int16_t x;
  int16_t y;
  Kind kind;
};

/// The fixed map, ordered left to right so the knob sweeps across it.
const Spot& spot(int index);

enum class Phase : uint8_t {
  kHide,          // the hider picks a spot
  kPassToSeeker,  // "hand the board over" screen
  kSeek,          // the seeker searches
  kRoundResult,   // found or not; the hiding spot is revealed
  kPassToHider,   // "hand the board over" before the next round
  kMatchOver,     // final totals and the winner
};

enum class Hint : uint8_t { kNone, kCold, kWarm, kHot };

/// The hint for searching spot `searched` when the hider is at `hidden`.
Hint hintFor(int searched, int hidden);

class Game {
 public:
  Game();

  /// Start a new match: round 0, player 1 hides first, totals 0.
  void reset();

  /// Move the cursor over the spots (+ = right), wrapping around.
  void moveCursor(int32_t steps);
  int cursor() const { return cursor_; }

  /// The knob press: choose a spot, search, or continue past a screen.
  /// Searching an already searched spot does nothing.
  void press();

  Phase phase() const { return phase_; }
  int round() const { return round_; }               // 0 .. kRounds-1
  int hider() const { return round_ % 2; }           // player 0 or 1
  int seeker() const { return 1 - hider(); }
  bool searched(int index) const;
  int searchesThisRound() const { return searches_; }
  bool foundThisRound() const { return found_; }
  Hint lastHint() const { return hint_; }
  int hiddenSpot() const { return hidden_; }         // only shown after a round

  /// Points a player has collected as seeker (lower is better).
  uint32_t total(int player) const { return totals_[player]; }

  /// After the match: 0 or 1 for the winner, -1 for a tie.
  int winner() const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

 private:
  void search();
  void endRound();

  Phase phase_ = Phase::kHide;
  int round_ = 0;
  int cursor_ = 0;
  int hidden_ = 0;
  uint16_t searched_mask_ = 0;
  int searches_ = 0;
  bool found_ = false;
  Hint hint_ = Hint::kNone;
  uint32_t totals_[2] = {0, 0};
  uint32_t revision_ = 0;
};

}  // namespace hideseek
