#pragma once

#include <stdint.h>

#include "rng.h"

/// Rules for Penalty Kick: you shoot, the computer keeps goal. Seen from
/// behind the ball: the goal mouth spans x 30-130, from the crossbar at y 12
/// down to the ground at y 50; the ball starts at (80, 72).
///
/// Each shot: aim left/right with the knob, press to lock the aim, then
/// press again to stop a swinging power meter (0-100). More power means a
/// higher shot: above ~78 it flies over the bar, and above 70 it also starts
/// to drift off target. The goalie waits a moment, then dives: towards the
/// ball if it "reads" the shot, otherwise to a random spot. It reacts sooner,
/// moves faster and reads more often each round. 5 shots per round; score 3
/// or more to reach the next round, otherwise the game ends. The score is
/// the total number of goals.
///
/// Fixed 20 ms ticks. Pure logic (no Arduino), host-tested.
namespace penalty {

constexpr uint32_t kTickMs = 20;
constexpr int16_t kGoalLeft = 30;
constexpr int16_t kGoalRight = 130;
constexpr int16_t kCrossbarY = 12;
constexpr int16_t kGroundY = 50;
constexpr int16_t kBallStartX = 80;
constexpr int16_t kBallStartY = 72;
constexpr int16_t kAimMin = 18;
constexpr int16_t kAimMax = 142;
constexpr int16_t kAimStepPx = 3;      // per knob click
constexpr int16_t kMeterStep = 4;      // power per tick while the meter swings
constexpr int16_t kMaxPower = 100;
constexpr int16_t kWildPower = 70;     // above this, the shot drifts
constexpr uint16_t kFlightTicks = 30;  // 0.6 s from kick to goal line
constexpr uint16_t kResultTicks = 75;  // the result shows for 1.5 s
constexpr int kShotsPerRound = 5;
constexpr int kGoalsToAdvance = 3;
constexpr int16_t kGoalieStartX = 80;
constexpr int16_t kGoalieLowReach = 10;  // px either side, for shots below kGoalieTopY
constexpr int16_t kGoalieHighReach = 5;  // px either side, for shots up near the bar
constexpr int16_t kGoalieTopY = 26;

enum class Phase : uint8_t { kAim, kPower, kFlight, kResult, kRoundOver, kGameOver };
enum class Result : uint8_t { kNone, kGoal, kSaved, kWide, kOver };

/// Test hook: force the goalie's decision.
enum class GoalieMode : uint8_t { kNormal, kAlwaysReads, kStaysPut };

class Game {
 public:
  explicit Game(uint32_t seed);
  void reset(uint32_t seed);

  /// Move the aim (only while aiming), clamped to kAimMin..kAimMax.
  void moveAim(int32_t steps);

  /// The knob press: lock the aim, stop the power meter (shoots), or move on
  /// from a result / round screen. Does nothing during the flight or after
  /// the game is over.
  void press();

  /// Advance one 20 ms tick.
  void tick();

  Phase phase() const { return phase_; }
  Result lastResult() const { return result_; }
  int round() const { return round_; }  // 1, 2, ...
  int shot() const { return shot_; }    // shots taken this round
  int goalsThisRound() const { return round_goals_; }
  uint32_t score() const { return score_; }
  int16_t aimX() const { return aim_x_; }
  int16_t power() const { return power_; }
  int16_t goalieX() const { return goalie_x_; }
  /// Result of shot i (0-based) of the current round, kNone if not taken.
  Result shotResult(int i) const { return shots_[i]; }

  /// Ball position during the flight (and where it ended, afterwards).
  int16_t ballX() const;
  int16_t ballY() const;
  /// 0 at the kick to kFlightTicks at the goal line.
  uint16_t flightTicks() const { return flight_; }

  /// Goalie difficulty for the current round.
  uint16_t goalieReactionTicks() const;
  int16_t goalieSpeed() const;  // px per tick
  int goalieReadPercent() const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

  void setGoalieModeForTest(GoalieMode mode) { goalie_mode_ = mode; }

 private:
  void shoot();
  void resolve();
  void nextShot();

  Rng rng_;
  Phase phase_ = Phase::kAim;
  Result result_ = Result::kNone;
  Result shots_[kShotsPerRound];
  int round_ = 1;
  int shot_ = 0;
  int round_goals_ = 0;
  uint32_t score_ = 0;
  int16_t aim_x_ = kBallStartX;
  int16_t power_ = 0;
  int16_t meter_dir_ = 1;
  int16_t target_x_ = 0;
  int16_t target_y_ = 0;
  uint16_t flight_ = 0;
  uint16_t result_ticks_ = 0;
  int16_t goalie_x_ = kGoalieStartX;
  int16_t dive_x_ = kGoalieStartX;
  GoalieMode goalie_mode_ = GoalieMode::kNormal;
  uint32_t revision_ = 0;
};

}  // namespace penalty
