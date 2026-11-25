#pragma once

#include <stddef.h>
#include <stdint.h>

/// Non-blocking beep/melody sequencer. It decides *what* should be sounding
/// and *when* it changes; a buzzer driver turns that into tone()/noTone().
/// Pure logic, host-tested.
class ToneSequence {
 public:
  struct Step {
    uint16_t freq_hz;      // 0 = silence
    uint16_t duration_ms;  // must be > 0
  };

  /// @param steps array of steps; must outlive this object.
  /// @param count number of steps (> 0).
  /// @param loop  restart from the first step after the last one.
  ToneSequence(const Step* steps, size_t count, bool loop);

  /// Start (or restart) from the first step at now_ms.
  void start(uint32_t now_ms);

  /// Stop; the next update() reports silence once.
  void stop();

  bool active() const { return active_; }

  /// Advance to now_ms. Returns true when the output must change, and writes
  /// the frequency to play (0 = silence) to *freq_hz. Returns false while the
  /// current step keeps sounding. A late call skips whole steps correctly.
  bool update(uint32_t now_ms, uint16_t* freq_hz);

 private:
  const Step* steps_;
  size_t count_;
  bool loop_;
  bool active_ = false;
  bool changed_ = false;
  size_t index_ = 0;
  uint32_t step_start_ms_ = 0;
};
