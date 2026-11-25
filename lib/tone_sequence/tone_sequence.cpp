#include "tone_sequence.h"

ToneSequence::ToneSequence(const Step* steps, size_t count, bool loop)
    : steps_(steps), count_(count), loop_(loop) {}

void ToneSequence::start(uint32_t now_ms) {
  active_ = (count_ > 0);
  changed_ = true;
  index_ = 0;
  step_start_ms_ = now_ms;
}

void ToneSequence::stop() {
  if (active_) {
    active_ = false;
    changed_ = true;
  }
}

bool ToneSequence::update(uint32_t now_ms, uint16_t* freq_hz) {
  if (active_) {
    // Advance past every step that has fully elapsed. Each step's start is
    // derived from the previous one, so timing stays on the original grid.
    while (now_ms - step_start_ms_ >= steps_[index_].duration_ms) {
      step_start_ms_ += steps_[index_].duration_ms;
      ++index_;
      changed_ = true;
      if (index_ == count_) {
        if (!loop_) {
          active_ = false;
          break;
        }
        index_ = 0;
      }
    }
  }
  if (!changed_) {
    return false;
  }
  changed_ = false;
  *freq_hz = active_ ? steps_[index_].freq_hz : 0;
  return true;
}
