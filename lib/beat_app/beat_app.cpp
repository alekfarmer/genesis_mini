#include "beat_app.h"

#include <Arduino.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;

namespace {

constexpr int32_t kBpmPerClick = 2;

// Layout, 160 x 80: header at y 1 (size 1); grid of 4 rows x 16 steps.
// Track letters at x 2; cells 8 x 14 px on a 9 x 16 px pitch from (12, 12),
// so the grid ends at x 155 and y 75.
constexpr int16_t kGridX = 12;
constexpr int16_t kGridY = 12;
constexpr int16_t kPitchX = 9;
constexpr int16_t kPitchY = 16;
constexpr int16_t kCellW = 8;
constexpr int16_t kCellH = 14;

// Drum sounds for a single buzzer. Each is shorter than the shortest step
// (75 ms at 200 BPM), so one never runs into the next.
const ToneSequence::Step kKick[] = {{160, 12}, {110, 12}, {70, 20}};
const ToneSequence::Step kSnare[] = {{1900, 6}, {700, 6}, {1600, 6}, {600, 6}, {1300, 8}};
const ToneSequence::Step kHat[] = {{7000, 8}};
const ToneSequence::Step kBleep[] = {{1319, 50}};

struct TrackSound {
  const ToneSequence::Step* steps;
  size_t count;
  const char* letter;
};
const TrackSound kTracks[beat::kTracks] = {
    {kKick, 3, "K"}, {kSnare, 5, "S"}, {kHat, 1, "H"}, {kBleep, 1, "B"}};

uint16_t trackColor(int track) {
  switch (track) {
    case 0:  return conversions::panelColor(255, 110, 80);   // kick: coral
    case 1:  return conversions::panelColor(255, 200, 70);   // snare: amber
    case 2:  return conversions::panelColor(90, 210, 255);   // hat: sky
    default: return conversions::panelColor(190, 120, 255);  // bleep: violet
  }
}

}  // namespace

void BeatApp::enter(uint32_t now_ms) {
  (void)now_ms;
  seq_.stop();
  arcade::setLed(arcade::kLedNormal);
  drawn_revision_ = UINT32_MAX;
  Serial.printf("[beats] ready: %u BPM, tap the button to play\n", static_cast<unsigned>(seq_.bpm()));
}

void BeatApp::exit() { seq_.stop(); }

void BeatApp::playTrack(int track, uint32_t now_ms) {
  if (track < 0 || track >= beat::kTracks) return;
  arcade::playSound(kTracks[track].steps, kTracks[track].count, now_ms);
}

void BeatApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (in.knob_steps != 0) {
    if (in.button_held) {
      seq_.changeBpm(in.knob_steps * kBpmPerClick);
    } else {
      seq_.moveCursor(in.knob_steps);
    }
  }
  if (in.knob_pressed) {
    seq_.toggleAtCursor();
    if (seq_.active(seq_.cursorTrack(), seq_.cursorStep()) && !seq_.playing()) {
      playTrack(seq_.cursorTrack(), now_ms);  // preview the sound just switched on
    }
  }
  if (in.button_tap) {
    if (seq_.playing()) {
      seq_.stop();
      arcade::stopSound();
    } else {
      seq_.start(now_ms);
    }
    Serial.printf("[beats] %s at %u BPM\n", seq_.playing() ? "play" : "stop", static_cast<unsigned>(seq_.bpm()));
  }
  if (in.button_hold_end) {
    Serial.printf("[beats] tempo %u BPM\n", static_cast<unsigned>(seq_.bpm()));
  }

  const int step = seq_.update(now_ms);
  if (step >= 0) {
    playTrack(seq_.trackToPlay(step), now_ms);
  }
}

void BeatApp::draw() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());

  char line[16];
  snprintf(line, sizeof(line), "%u BPM", static_cast<unsigned>(seq_.bpm()));
  ui::text(2, 1, 1, ui::white(), line);
  ui::textRight(158, 1, 1, seq_.playing() ? ui::good() : ui::dim(), seq_.playing() ? "PLAYING" : "STOPPED");

  const uint16_t empty = conversions::panelColor(35, 35, 50);
  const uint16_t empty_beat = conversions::panelColor(55, 55, 75);  // first step of each beat
  const int playing_step = seq_.currentStep();
  for (int t = 0; t < beat::kTracks; ++t) {
    const int16_t y = static_cast<int16_t>(kGridY + t * kPitchY);
    ui::text(2, static_cast<int16_t>(y + 3), 1, trackColor(t), kTracks[t].letter);
    for (int s = 0; s < beat::kSteps; ++s) {
      const int16_t x = static_cast<int16_t>(kGridX + s * kPitchX);
      const uint16_t fill = seq_.active(t, s) ? trackColor(t) : (s % 4 == 0 ? empty_beat : empty);
      c.fillRect(x, y, kCellW, kCellH, fill);
      if (s == playing_step) {
        c.drawRect(x, y, kCellW, kCellH, ui::white());
      }
    }
  }
  // Cursor: a bright frame just outside the cell.
  const int16_t cx = static_cast<int16_t>(kGridX + seq_.cursorStep() * kPitchX - 1);
  const int16_t cy = static_cast<int16_t>(kGridY + seq_.cursorTrack() * kPitchY - 1);
  c.drawRect(cx, cy, kCellW + 2, kCellH + 2, ui::accent());

  drawn_revision_ = seq_.revision();
}
