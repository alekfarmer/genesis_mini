#pragma once

/// Over-temperature alarm with hysteresis and a "silence" button.
///
///   kIdle      --(temp > threshold)-->               kAlarming
///   kAlarming  --(silence())-->                      kSilenced
///   kAlarming / kSilenced --(temp < threshold - hysteresis)--> kIdle
///
/// Hysteresis stops the alarm flickering when the temperature hovers at the
/// threshold. A silenced alarm stays quiet until the temperature has dropped
/// back below the band, then re-arms. NaN readings (failed sensor reads) are
/// ignored. Units are whatever the caller uses (this project: °F). Pure logic,
/// host-tested.
class ThresholdAlarm {
 public:
  enum class State { kIdle, kAlarming, kSilenced };

  ThresholdAlarm(float threshold, float hysteresis);

  /// Change the threshold; takes effect on the next update().
  void setThreshold(float threshold);
  float threshold() const { return threshold_; }

  /// Feed a new temperature reading; returns the new state.
  State update(float temperature);

  /// Silence an active alarm. Does nothing unless the state is kAlarming.
  void silence();

  State state() const { return state_; }

 private:
  float threshold_;
  float hysteresis_;
  State state_ = State::kIdle;
};
