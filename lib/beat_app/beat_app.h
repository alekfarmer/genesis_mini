#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "beat_core.h"

/// The beat maker for the arcade: a 4-track x 16-step pattern played on the
/// buzzer. Landscape. The pattern is kept while the board stays powered.
///   Knob, turn                         move the cursor across the grid
///   Knob, press                        switch the step under the cursor on/off
///   On-board button, tap               play / stop
///   On-board button held + knob turn   change the tempo (2 BPM per click)
class BeatApp : public arcade::App {
 public:
  const char* name() const override { return "BEATS"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override { return seq_.revision() != drawn_revision_; }
  void draw() override;
  void exit() override;

 private:
  void playTrack(int track, uint32_t now_ms);

  beat::Sequencer seq_;
  uint32_t drawn_revision_ = UINT32_MAX;
};
