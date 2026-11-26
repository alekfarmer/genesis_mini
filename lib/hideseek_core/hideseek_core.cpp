#include "hideseek_core.h"

namespace hideseek {
namespace {

// The map: 12 spots spread over the 160x80 screen below the 10 px status
// line, ordered by x so turning the knob sweeps left to right.
const Spot kMap[kSpots] = {
    {12, 20, Kind::kTree}, {30, 55, Kind::kBush}, {45, 30, Kind::kRock},  {60, 65, Kind::kLog},
    {72, 22, Kind::kShed}, {85, 48, Kind::kTree}, {98, 70, Kind::kBush},  {108, 28, Kind::kWell},
    {122, 55, Kind::kRock}, {135, 20, Kind::kTree}, {146, 62, Kind::kBush}, {150, 40, Kind::kLog},
};

int32_t squared(int32_t v) { return v * v; }

}  // namespace

const Spot& spot(int index) { return kMap[index]; }

Hint hintFor(int searched, int hidden) {
  const int32_t d2 = squared(kMap[searched].x - kMap[hidden].x) + squared(kMap[searched].y - kMap[hidden].y);
  if (d2 < squared(kHotDistance)) return Hint::kHot;
  if (d2 < squared(kWarmDistance)) return Hint::kWarm;
  return Hint::kCold;
}

Game::Game() { reset(); }

void Game::reset() {
  phase_ = Phase::kHide;
  round_ = 0;
  cursor_ = 0;
  hidden_ = 0;
  searched_mask_ = 0;
  searches_ = 0;
  found_ = false;
  hint_ = Hint::kNone;
  totals_[0] = totals_[1] = 0;
  ++revision_;
}

void Game::moveCursor(int32_t steps) {
  if (phase_ != Phase::kHide && phase_ != Phase::kSeek) return;
  int32_t next = (cursor_ + steps) % kSpots;
  if (next < 0) next += kSpots;
  cursor_ = static_cast<int>(next);
  ++revision_;
}

void Game::press() {
  switch (phase_) {
    case Phase::kHide:
      hidden_ = cursor_;
      cursor_ = 0;  // so the seeker can't see where the cursor was left
      phase_ = Phase::kPassToSeeker;
      break;
    case Phase::kPassToSeeker:
      phase_ = Phase::kSeek;
      break;
    case Phase::kSeek:
      search();
      break;
    case Phase::kRoundResult:
      if (round_ + 1 >= kRounds) {
        phase_ = Phase::kMatchOver;
      } else {
        ++round_;
        searched_mask_ = 0;
        searches_ = 0;
        found_ = false;
        hint_ = Hint::kNone;
        cursor_ = 0;
        phase_ = Phase::kPassToHider;
      }
      break;
    case Phase::kPassToHider:
      phase_ = Phase::kHide;
      break;
    case Phase::kMatchOver:
      reset();
      return;
  }
  ++revision_;
}

bool Game::searched(int index) const { return (searched_mask_ >> index) & 1u; }

int Game::winner() const {
  if (totals_[0] < totals_[1]) return 0;
  if (totals_[1] < totals_[0]) return 1;
  return -1;
}

void Game::search() {
  if (searched(cursor_)) return;
  searched_mask_ = static_cast<uint16_t>(searched_mask_ | (1u << cursor_));
  ++searches_;
  if (cursor_ == hidden_) {
    found_ = true;
    endRound();
    return;
  }
  hint_ = hintFor(cursor_, hidden_);
  if (searches_ >= kMaxSearches) {
    endRound();
  }
}

void Game::endRound() {
  totals_[seeker()] += found_ ? static_cast<uint32_t>(searches_) : static_cast<uint32_t>(kMissPenalty);
  phase_ = Phase::kRoundResult;
}

}  // namespace hideseek
