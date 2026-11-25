#include "beat_core.h"

namespace beat {
namespace {
constexpr int kCells = kTracks * kSteps;
bool inRange(int track, int step) { return track >= 0 && track < kTracks && step >= 0 && step < kSteps; }
}  // namespace

Sequencer::Sequencer() { loadStarter(); }

bool Sequencer::active(int track, int step) const {
  return inRange(track, step) && ((pattern_[track] >> step) & 1u) != 0;
}

void Sequencer::toggle(int track, int step) {
  if (!inRange(track, step)) return;
  pattern_[track] = static_cast<uint16_t>(pattern_[track] ^ (1u << step));
  ++revision_;
}

void Sequencer::clear() {
  for (uint16_t& p : pattern_) p = 0;
  ++revision_;
}

void Sequencer::loadStarter() {
  clear();
  pattern_[0] = (1u << 0) | (1u << 8);   // kick: beats 1 and 3
  pattern_[1] = (1u << 4) | (1u << 12);  // snare: beats 2 and 4
  pattern_[2] = 0x5555;                  // hi-hat: every eighth note
  pattern_[3] = (1u << 14);              // bleep: end of the bar
  ++revision_;
}

void Sequencer::moveCursor(int32_t steps) {
  int32_t next = (cursor_ + steps) % kCells;
  if (next < 0) next += kCells;
  cursor_ = static_cast<int>(next);
  ++revision_;
}

void Sequencer::toggleAtCursor() { toggle(cursorTrack(), cursorStep()); }

void Sequencer::setBpm(int32_t bpm) {
  if (bpm < kMinBpm) bpm = kMinBpm;
  if (bpm > kMaxBpm) bpm = kMaxBpm;
  bpm_ = static_cast<uint16_t>(bpm);
  ++revision_;
}

void Sequencer::changeBpm(int32_t delta) { setBpm(static_cast<int32_t>(bpm_) + delta); }

uint32_t Sequencer::stepIntervalMs() const { return 60000u / (static_cast<uint32_t>(bpm_) * 4u); }

void Sequencer::start(uint32_t now_ms) {
  playing_ = true;
  first_step_pending_ = true;
  current_ = 0;
  step_start_ms_ = now_ms;
  ++revision_;
}

void Sequencer::stop() {
  playing_ = false;
  ++revision_;
}

int Sequencer::update(uint32_t now_ms) {
  if (!playing_) return -1;
  if (first_step_pending_) {
    first_step_pending_ = false;
    ++revision_;
    return current_;
  }
  const uint32_t interval = stepIntervalMs();
  const uint32_t elapsed = now_ms - step_start_ms_;
  if (elapsed < interval) return -1;
  // Advance by whole steps so a late call stays on the beat grid.
  const uint32_t steps = elapsed / interval;
  step_start_ms_ += steps * interval;
  current_ = static_cast<int>((static_cast<uint32_t>(current_) + steps) % kSteps);
  ++revision_;
  return current_;
}

int Sequencer::trackToPlay(int step) const {
  for (int t = 0; t < kTracks; ++t) {
    if (active(t, step)) return t;
  }
  return -1;
}

}  // namespace beat
