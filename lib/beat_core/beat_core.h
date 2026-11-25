#pragma once

#include <stdint.h>

/// Step sequencer for the beat maker: 4 tracks x 16 steps (one bar of
/// sixteenth notes). A cursor moves over the grid to toggle steps; while
/// playing, update() reports each new step on time. The buzzer can sound only
/// one thing at once, so on a step where several tracks are on, the topmost
/// track wins. Pure logic (no Arduino), host-tested.
namespace beat {

constexpr int kTracks = 4;  // 0 kick, 1 snare, 2 hi-hat, 3 bleep
constexpr int kSteps = 16;
constexpr uint16_t kMinBpm = 60;
constexpr uint16_t kMaxBpm = 200;
constexpr uint16_t kDefaultBpm = 110;

class Sequencer {
 public:
  /// Starts stopped, at the default tempo, with a starter beat loaded.
  Sequencer();

  bool active(int track, int step) const;
  void toggle(int track, int step);
  void clear();

  /// Load the starter beat: kick on 1 and 3, snare on 2 and 4, hi-hat on
  /// every eighth note, a bleep at the end of the bar.
  void loadStarter();

  /// Cursor over the grid, read row by row: steps 0-15 of track 0, then
  /// track 1, and so on. + moves right; it wraps at both ends.
  void moveCursor(int32_t steps);
  int cursorTrack() const { return cursor_ / kSteps; }
  int cursorStep() const { return cursor_ % kSteps; }
  void toggleAtCursor();

  uint16_t bpm() const { return bpm_; }
  void setBpm(int32_t bpm);  // clamped to kMinBpm..kMaxBpm
  void changeBpm(int32_t delta);

  /// Length of one step (a sixteenth note): 60000 / (bpm * 4) ms.
  uint32_t stepIntervalMs() const;

  void start(uint32_t now_ms);
  void stop();
  bool playing() const { return playing_; }

  /// Call every loop(). Returns the step (0-15) that begins now, or -1. The
  /// first call after start() returns step 0. A late call skips steps but
  /// stays on the beat grid.
  int update(uint32_t now_ms);

  /// The step being played, or -1 when stopped.
  int currentStep() const { return playing_ ? current_ : -1; }

  /// The track to sound on `step`: the topmost active track, or -1.
  int trackToPlay(int step) const;

  /// Increases whenever anything visible changes.
  uint32_t revision() const { return revision_; }

 private:
  uint16_t pattern_[kTracks];  // bit s = step s is on
  int cursor_ = 0;
  uint16_t bpm_ = kDefaultBpm;
  bool playing_ = false;
  bool first_step_pending_ = false;
  int current_ = 0;
  uint32_t step_start_ms_ = 0;
  uint32_t revision_ = 0;
};

}  // namespace beat
