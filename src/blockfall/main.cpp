// blockfall: a falling-block puzzle game on the kit. Hold the board in
// portrait (screen tall, a quarter turn from the usual landscape).
//
//   Knob, turn              move the piece left / right
//   Knob, press             rotate
//   On-board button, tap    drop the piece to the bottom
//   On-board button, hold   fall faster while held
//   Either button           new game, from the game-over screen
//
// Buzzer: short sounds on landing, line clears and game over. NeoPixel: green
// heartbeat while playing, a white flash on line clears, red at game over.
// The best score is kept in flash (Preferences) and survives power-off.

#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>

#include "blockfall_core.h"
#include "blockfall_screen.h"
#include "buzzer.h"
#include "encoder_input.h"
#include "periodic.h"
#include "pins.h"
#include "status_led.h"
#include "tap_hold.h"
#include "tone_sequence.h"

namespace {

using blockfall::Game;

constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kSerialWaitMs = 2000;
constexpr uint32_t kFramePeriodMs = 33;       // redraw cap, ~30 fps
constexpr uint32_t kSoftDropMs = 40;          // fall interval while the button is held
constexpr uint32_t kHoldMs = 220;             // longer than this = hold, not tap
constexpr uint32_t kSettleMs = 15;            // button debounce
constexpr uint32_t kRestartGuardMs = 600;     // ignore presses right after game over
constexpr uint32_t kFlashMs = 300;            // NeoPixel flash on a line clear
constexpr const char* kPrefsNamespace = "blockfall";
constexpr const char* kPrefsBestKey = "best";

// Sound effects (original short figures).
const ToneSequence::Step kLandSteps[] = {{220, 25}};
const ToneSequence::Step kClearSteps[] = {{880, 60}, {1175, 90}};
const ToneSequence::Step kBigClearSteps[] = {{880, 60}, {1175, 60}, {1480, 60}, {1760, 160}};
const ToneSequence::Step kGameOverSteps[] = {{660, 150}, {0, 40}, {520, 150}, {0, 40}, {390, 320}};

ToneSequence g_landSound(kLandSteps, 1, false);
ToneSequence g_clearSound(kClearSteps, 2, false);
ToneSequence g_bigClearSound(kBigClearSteps, 4, false);
ToneSequence g_gameOverSound(kGameOverSteps, 5, false);
ToneSequence* g_sound = nullptr;  // the effect currently playing, if any

constexpr StatusLed::Color kLedPlaying = {0, 12, 4};
constexpr StatusLed::Color kLedFlash = {24, 24, 24};
constexpr StatusLed::Color kLedGameOver = {16, 0, 0};

BlockfallScreen g_screen(pins::kTftCs, pins::kTftRst, pins::kTftDc);
EncoderInput g_encoder(pins::kEncoderButton, pins::kEncoderClk, pins::kEncoderDt);
TapHold g_dropButton(false, kHoldMs, kSettleMs);  // on-board button is active-low
Buzzer g_buzzer(pins::kBuzzer);
StatusLed g_led(pins::kNeoPixel, kLedPlaying);
Periodic g_frameTimer(kFramePeriodMs);
Preferences g_prefs;

Game g_game(1);  // reseeded from the hardware RNG in newGame()
bool g_prefsOk = false;
uint32_t g_best = 0;
uint32_t g_lastFallMs = 0;
uint32_t g_gameOverMs = 0;
uint32_t g_flashUntilMs = 0;
bool g_flashing = false;
uint32_t g_drawnRevision = UINT32_MAX;
uint32_t g_drawnBest = UINT32_MAX;

void playSound(ToneSequence& sound, uint32_t now) {
  if (g_sound != nullptr && g_sound != &sound) {
    g_sound->stop();
    uint16_t ignored = 0;
    g_sound->update(now, &ignored);  // consume its "stopped" change
  }
  g_sound = &sound;
  g_sound->start(now);
}

void newGame(uint32_t now) {
  const uint32_t seed = esp_random();
  g_game.reset(seed);
  g_lastFallMs = now;
  g_led.setColor(kLedPlaying);
  Serial.printf("[blockfall] new game (best %" PRIu32 ")\n", g_best);
}

void saveBest() {
  if (!g_prefsOk) {
    Serial.println("[blockfall] ERROR: flash storage unavailable, best score not saved");
    return;
  }
  if (g_prefs.putUInt(kPrefsBestKey, g_best) == 0) {
    Serial.println("[blockfall] ERROR: could not save best score to flash");
  }
}

/// After a piece locks: sound and light for line clears, game-over handling.
void onLock(uint32_t now) {
  const uint8_t cleared = g_game.lastCleared();
  if (g_game.state() == Game::State::kGameOver) {
    g_gameOverMs = now;
    playSound(g_gameOverSound, now);
    g_led.setColor(kLedGameOver);
    const bool record = g_game.score() > g_best;
    if (record) {
      g_best = g_game.score();
      saveBest();
    }
    Serial.printf("[blockfall] game over: score %" PRIu32 ", lines %" PRIu32 "%s\n", g_game.score(),
                  g_game.lines(), record ? " (new best)" : "");
    return;
  }
  if (cleared > 0) {
    playSound(cleared == 4 ? g_bigClearSound : g_clearSound, now);
    g_led.setColor(kLedFlash);
    g_flashing = true;
    g_flashUntilMs = now + kFlashMs;
    Serial.printf("[blockfall] cleared %u line(s): score %" PRIu32 ", level %" PRIu32 "\n",
                  static_cast<unsigned>(cleared), g_game.score(), g_game.level());
  } else {
    playSound(g_landSound, now);
  }
}

void handleInputWhilePlaying(const EncoderInput::Event& in, TapHold::Event button, uint32_t now) {
  for (int32_t i = 0; i < in.steps; ++i) g_game.moveRight();
  for (int32_t i = 0; i > in.steps; --i) g_game.moveLeft();
  if (in.pressed) {
    g_game.rotate();
  }
  if (button == TapHold::Event::kTap) {
    g_game.hardDrop();
    g_lastFallMs = now;
    onLock(now);
    return;
  }
  const bool soft = g_dropButton.held();
  const uint32_t interval = soft ? kSoftDropMs : g_game.fallIntervalMs();
  if (now - g_lastFallMs >= interval) {
    g_lastFallMs = now;
    if (g_game.tick(soft)) {
      onLock(now);
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  const uint32_t start = millis();
  while (!Serial && millis() - start < kSerialWaitMs) {
    delay(10);  // bounded wait for a monitor; setup only
  }
  Serial.println("\n[blockfall] starting");

  if (!g_screen.begin()) {
    Serial.println("[screen] ERROR: no memory for the frame buffer, or SPI failed to start");
  }
  g_encoder.begin();
  pinMode(pins::kUserButton, INPUT_PULLUP);  // catalog: on-board button, active-low
  g_buzzer.play(0);
  g_led.begin();

  g_prefsOk = g_prefs.begin(kPrefsNamespace, false);
  if (g_prefsOk) {
    g_best = g_prefs.getUInt(kPrefsBestKey, 0);
  } else {
    Serial.println("[blockfall] ERROR: could not open flash storage; best score won't persist");
  }
  newGame(millis());
}

void loop() {
  const uint32_t now = millis();

  // Inputs first, every pass: the encoder is sampled here.
  const EncoderInput::Event in = g_encoder.poll(now);
  const TapHold::Event button = g_dropButton.update(digitalRead(pins::kUserButton) == HIGH, now);

  if (g_game.state() == Game::State::kPlaying) {
    handleInputWhilePlaying(in, button, now);
  } else if ((in.pressed || button == TapHold::Event::kTap) && now - g_gameOverMs >= kRestartGuardMs) {
    newGame(now);
  }

  if (g_sound != nullptr) {
    uint16_t freq = 0;
    if (g_sound->update(now, &freq)) {
      g_buzzer.play(freq);
    }
  }
  if (g_flashing && static_cast<int32_t>(now - g_flashUntilMs) >= 0) {
    g_flashing = false;
    if (g_game.state() == Game::State::kPlaying) {
      g_led.setColor(kLedPlaying);
    }
  }

  // Redraw only when something visible changed, at most ~30 times a second.
  const bool changed = g_game.revision() != g_drawnRevision || g_best != g_drawnBest;
  if (changed && g_frameTimer.due(now)) {
    g_screen.draw(g_game, g_best);
    g_drawnRevision = g_game.revision();
    g_drawnBest = g_best;
  }

  g_led.update(now);
}
