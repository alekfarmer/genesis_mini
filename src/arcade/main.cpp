// arcade: boots to a menu of games; pick one with the knob.
//
//   Menu:  turn the knob to choose, press it to start.
//   Any game: hold the knob for 1.5 s to return to the menu.
//
// This file is the host: it owns the screen, buzzer, NeoPixel, knob, on-board
// button and flash storage, turns knob and button activity into
// arcade::Input, and runs whichever app is active. Games live in lib/*_app
// and are listed in apps.cpp.

#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>

#include "apps.h"
#include "arcade_app.h"
#include "buzzer.h"
#include "encoder_input.h"
#include "menu_app.h"
#include "panel.h"
#include "periodic.h"
#include "pins.h"
#include "status_led.h"
#include "tap_hold.h"
#include "tone_sequence.h"

namespace {

constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kSerialWaitMs = 2000;
constexpr uint32_t kFramePeriodMs = 33;  // redraw cap, ~30 fps
constexpr uint32_t kButtonHoldMs = 220;  // on-board button: longer = hold, not tap
constexpr uint32_t kButtonSettleMs = 15;
constexpr const char* kBestKey = "best";

EncoderInput g_encoder(pins::kEncoderButton, pins::kEncoderClk, pins::kEncoderDt);
TapHold g_button(false, kButtonHoldMs, kButtonSettleMs);  // on-board button is active-low
Buzzer g_buzzer(pins::kBuzzer);
StatusLed g_led(pins::kNeoPixel, arcade::kLedNormal);
Periodic g_frameTimer(kFramePeriodMs);

const ToneSequence::Step kSilence[] = {{0, 1}};
ToneSequence g_sound(kSilence, 1, false);

MenuApp g_menu(kGames, kGameCount);
arcade::App* g_current = nullptr;
bool g_forceRedraw = true;
bool g_screenOk = false;

/// Leave the current app and start `next`, switching screen orientation.
void switchTo(arcade::App* next, uint32_t now) {
  if (g_current != nullptr) {
    g_current->exit();
  }
  arcade::stopSound();
  if (g_screenOk && !panel::setOrientation(next->orientation())) {
    Serial.printf("[arcade] ERROR: no memory to switch the screen for %s, staying in the menu\n", next->name());
    next = &g_menu;
    panel::setOrientation(g_menu.orientation());
  }
  g_current = next;
  g_current->enter(now);
  g_forceRedraw = true;
  Serial.printf("[arcade] -> %s (free heap %" PRIu32 " B)\n", g_current->name(), ESP.getFreeHeap());
}

}  // namespace

// ---- Services for apps (declared in arcade_app.h) ---------------------------

namespace arcade {

void playSound(const ToneSequence::Step* steps, size_t count, uint32_t now_ms) {
  g_sound = ToneSequence(steps, count, false);
  g_sound.start(now_ms);
}

void stopSound() {
  g_sound.stop();
  g_buzzer.play(0);
}

void setLed(StatusLed::Color color) { g_led.setColor(color); }

uint32_t loadBest(const char* key) {
  // Opened read-write so a first visit creates the namespace instead of
  // logging "not found"; nothing is written until saveBest().
  Preferences prefs;
  if (!prefs.begin(key, false)) {
    Serial.printf("[arcade] ERROR: flash storage unavailable, no best score for %s\n", key);
    return 0;
  }
  const uint32_t best = prefs.getUInt(kBestKey, 0);
  prefs.end();
  return best;
}

void saveBest(const char* key, uint32_t score) {
  Preferences prefs;
  if (!prefs.begin(key, false)) {
    Serial.printf("[arcade] ERROR: flash storage unavailable, best score for %s not saved\n", key);
    return;
  }
  if (prefs.putUInt(kBestKey, score) == 0) {
    Serial.printf("[arcade] ERROR: could not save best score for %s\n", key);
  }
  prefs.end();
}

uint32_t newSeed() { return esp_random(); }

}  // namespace arcade

// ---- Arduino entry points ---------------------------------------------------

void setup() {
  Serial.begin(kSerialBaud);
  const uint32_t start = millis();
  while (!Serial && millis() - start < kSerialWaitMs) {
    delay(10);  // bounded wait for a monitor; setup only
  }
  Serial.printf("\n[arcade] starting, %u games\n", static_cast<unsigned>(kGameCount));

  g_screenOk = panel::begin(pins::kTftCs, pins::kTftRst, pins::kTftDc, panel::Orientation::kLandscape);
  if (!g_screenOk) {
    Serial.println("[screen] ERROR: no memory for the frame buffer, or SPI failed to start");
  }
  g_encoder.begin();
  pinMode(pins::kUserButton, INPUT_PULLUP);  // catalog: on-board button, active-low
  g_buzzer.play(0);
  g_led.begin();
  switchTo(&g_menu, millis());
}

void loop() {
  const uint32_t now = millis();

  // Inputs first, every pass: the encoder is sampled here.
  const EncoderInput::Event knob = g_encoder.poll(now);
  const TapHold::Event button = g_button.update(digitalRead(pins::kUserButton) == HIGH, now);

  if (knob.long_press && g_current != &g_menu) {
    switchTo(&g_menu, now);
  } else {
    const arcade::Input in{
        knob.steps,
        knob.pressed,
        button == TapHold::Event::kTap,
        button == TapHold::Event::kHoldStart,
        g_button.held(),
        button == TapHold::Event::kHoldEnd,
    };
    g_current->update(in, now);
    if (g_current == &g_menu) {
      if (arcade::App* chosen = g_menu.takeLaunch()) {
        switchTo(chosen, now);
      }
    }
  }

  uint16_t freq = 0;
  if (g_sound.update(now, &freq)) {
    g_buzzer.play(freq);
  }

  // Redraw only when something visible changed, at most ~30 times a second,
  // so SPI traffic doesn't starve the knob or disturb the buzzer.
  if (g_screenOk && (g_forceRedraw || g_current->needsRedraw()) && g_frameTimer.due(now)) {
    g_current->draw();
    panel::push();
    g_forceRedraw = false;
  }

  g_led.update(now);
}
