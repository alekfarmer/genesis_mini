#include "threshold_alarm.h"

#include <math.h>

ThresholdAlarm::ThresholdAlarm(float threshold, float hysteresis)
    : threshold_(threshold), hysteresis_(hysteresis) {}

void ThresholdAlarm::setThreshold(float threshold) { threshold_ = threshold; }

ThresholdAlarm::State ThresholdAlarm::update(float temperature) {
  if (isnan(temperature)) {
    return state_;  // failed read: keep the last decision
  }
  switch (state_) {
    case State::kIdle:
      if (temperature > threshold_) {
        state_ = State::kAlarming;
      }
      break;
    case State::kAlarming:
    case State::kSilenced:
      if (temperature < threshold_ - hysteresis_) {
        state_ = State::kIdle;
      }
      break;
  }
  return state_;
}

void ThresholdAlarm::silence() {
  if (state_ == State::kAlarming) {
    state_ = State::kSilenced;
  }
}
