#include "penalty_core.h"

namespace penalty {
namespace {

// How high the ball goes: power 0 rolls along the ground (y 50); every 85
// points of power lift it 44 px, so ~78 reaches the crossbar (y 12).
constexpr int32_t kLiftPx = kGroundY - kCrossbarY + 6;
constexpr int32_t kLiftPower = 85;
constexpr int16_t kPostMargin = 2;  // a shot this close to a post hits it: wide
constexpr int16_t kGuessMargin = 8; // a guessing goalie dives at least this far inside the posts

int16_t towards(int16_t from, int16_t to, int16_t step) {
  if (from < to) return static_cast<int16_t>(from + step > to ? to : from + step);
  if (from > to) return static_cast<int16_t>(from - step < to ? to : from - step);
  return from;
}

int16_t absDiff(int16_t a, int16_t b) { return static_cast<int16_t>(a > b ? a - b : b - a); }

}  // namespace

Game::Game(uint32_t seed) : rng_(seed) { reset(seed); }

void Game::reset(uint32_t seed) {
  rng_ = Rng(seed);
  phase_ = Phase::kAim;
  result_ = Result::kNone;
  for (Result& r : shots_) r = Result::kNone;
  round_ = 1;
  shot_ = 0;
  round_goals_ = 0;
  score_ = 0;
  aim_x_ = kBallStartX;
  power_ = 0;
  meter_dir_ = 1;
  target_x_ = kBallStartX;
  target_y_ = kBallStartY;
  flight_ = 0;
  result_ticks_ = 0;
  goalie_x_ = kGoalieStartX;
  dive_x_ = kGoalieStartX;
  ++revision_;
}

uint16_t Game::goalieReactionTicks() const {
  const int ticks = 12 - 2 * (round_ - 1);
  return static_cast<uint16_t>(ticks < 2 ? 2 : ticks);
}

int16_t Game::goalieSpeed() const {
  const int speed = 2 + (round_ - 1) / 2;
  return static_cast<int16_t>(speed > 5 ? 5 : speed);
}

int Game::goalieReadPercent() const {
  const int percent = 30 + 10 * (round_ - 1);
  return percent > 80 ? 80 : percent;
}

void Game::moveAim(int32_t steps) {
  if (phase_ != Phase::kAim || steps == 0) return;
  int32_t x = aim_x_ + steps * kAimStepPx;
  if (x < kAimMin) x = kAimMin;
  if (x > kAimMax) x = kAimMax;
  aim_x_ = static_cast<int16_t>(x);
  ++revision_;
}

void Game::press() {
  switch (phase_) {
    case Phase::kAim:
      phase_ = Phase::kPower;
      power_ = 0;
      meter_dir_ = 1;
      break;
    case Phase::kPower:
      shoot();
      break;
    case Phase::kFlight:
    case Phase::kGameOver:
      return;  // nothing to do; no visible change
    case Phase::kResult:
      nextShot();
      break;
    case Phase::kRoundOver:
      if (round_goals_ >= kGoalsToAdvance) {
        ++round_;
        shot_ = 0;
        round_goals_ = 0;
        for (Result& r : shots_) r = Result::kNone;
        result_ = Result::kNone;
        goalie_x_ = kGoalieStartX;
        phase_ = Phase::kAim;
      } else {
        phase_ = Phase::kGameOver;
      }
      break;
  }
  ++revision_;
}

void Game::tick() {
  switch (phase_) {
    case Phase::kPower:
      power_ = static_cast<int16_t>(power_ + meter_dir_ * kMeterStep);
      if (power_ >= kMaxPower) {
        power_ = kMaxPower;
        meter_dir_ = -1;
      } else if (power_ <= 0) {
        power_ = 0;
        meter_dir_ = 1;
      }
      break;
    case Phase::kFlight:
      ++flight_;
      if (flight_ > goalieReactionTicks()) {
        goalie_x_ = towards(goalie_x_, dive_x_, goalieSpeed());
      }
      if (flight_ >= kFlightTicks) resolve();
      break;
    case Phase::kResult:
      if (++result_ticks_ >= kResultTicks) nextShot();
      break;
    case Phase::kAim:
    case Phase::kRoundOver:
    case Phase::kGameOver:
      return;  // nothing moves
  }
  ++revision_;
}

void Game::shoot() {
  target_x_ = aim_x_;
  if (power_ > kWildPower) {
    const int16_t drift = static_cast<int16_t>((power_ - kWildPower) / 2);
    target_x_ = static_cast<int16_t>(target_x_ + (rng_.below(2) == 0 ? drift : -drift));
  }
  target_y_ = static_cast<int16_t>(kGroundY - (static_cast<int32_t>(power_) * kLiftPx) / kLiftPower);

  switch (goalie_mode_) {
    case GoalieMode::kStaysPut:
      dive_x_ = goalie_x_;
      break;
    case GoalieMode::kAlwaysReads:
      dive_x_ = target_x_;
      break;
    case GoalieMode::kNormal:
      if (static_cast<int>(rng_.below(100)) < goalieReadPercent()) {
        dive_x_ = target_x_;
      } else {
        const int16_t span = static_cast<int16_t>(kGoalRight - kGoalLeft - 2 * kGuessMargin);
        dive_x_ = static_cast<int16_t>(kGoalLeft + kGuessMargin + rng_.below(static_cast<uint32_t>(span + 1)));
      }
      break;
  }
  flight_ = 0;
  phase_ = Phase::kFlight;
}

void Game::resolve() {
  if (target_y_ < kCrossbarY) {
    result_ = Result::kOver;
  } else if (target_x_ < kGoalLeft + kPostMargin || target_x_ > kGoalRight - kPostMargin) {
    result_ = Result::kWide;
  } else {
    const int16_t reach = target_y_ >= kGoalieTopY ? kGoalieLowReach : kGoalieHighReach;
    if (absDiff(goalie_x_, target_x_) <= reach) {
      result_ = Result::kSaved;
    } else {
      result_ = Result::kGoal;
      ++score_;
      ++round_goals_;
    }
  }
  shots_[shot_] = result_;
  ++shot_;
  result_ticks_ = 0;
  phase_ = Phase::kResult;
}

void Game::nextShot() {
  flight_ = 0;
  power_ = 0;
  goalie_x_ = kGoalieStartX;
  phase_ = shot_ >= kShotsPerRound ? Phase::kRoundOver : Phase::kAim;
}

int16_t Game::ballX() const {
  switch (phase_) {
    case Phase::kAim:
    case Phase::kPower:
      return kBallStartX;
    case Phase::kFlight:
      return static_cast<int16_t>(kBallStartX + (target_x_ - kBallStartX) * flight_ / kFlightTicks);
    default:
      return target_x_;
  }
}

int16_t Game::ballY() const {
  switch (phase_) {
    case Phase::kAim:
    case Phase::kPower:
      return kBallStartY;
    case Phase::kFlight:
      return static_cast<int16_t>(kBallStartY + (target_y_ - kBallStartY) * flight_ / kFlightTicks);
    default:
      return target_y_;
  }
}

}  // namespace penalty
