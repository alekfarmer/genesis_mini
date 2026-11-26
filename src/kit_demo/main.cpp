// kit_demo: an over-temperature alarm using every Starter Kit module.
//
//   Port 1  ST7735 screen   temperature, humidity, threshold, alarm status
//   Port 2  passive buzzer  beeps while the alarm is active
//   Port 3  DHT11           read every 2 s
//   Port 4  rotary encoder  turn: threshold +/- 1 °F; press: silence alarm
//   On-board NeoPixel       green heartbeat, red while alarming
//
// The alarm sounds when the temperature rises above the threshold and clears
// once it falls 1 °F below it. A silenced alarm stays quiet until it clears.
//
// Ports come from include/board_config.h. If any of the four modules isn't
// configured, a stub is built instead that says which one is missing.

#include <Arduino.h>

#include "pins.h"

#define KIT_DEMO_MODULES_OK (HAS_TFT && HAS_BUZZER && HAS_DHT11 && HAS_ENCODER)

#if KIT_DEMO_MODULES_OK

#include <algorithm>

#include "buzzer.h"
#include "conversions.h"
#include "dht_sensor.h"
#include "encoder_input.h"
#include "periodic.h"
#include "status_led.h"
#include "thermostat_screen.h"
#include "threshold_alarm.h"
#include "tone_sequence.h"

namespace {

using AlarmState = ThresholdAlarm::State;
using Status = ThermostatView::Status;

constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kSerialWaitMs = 2000;
constexpr uint32_t kSensorPeriodMs = 2000;  // DHT11 allows at most 1 Hz
constexpr uint32_t kFramePeriodMs = 33;     // redraw cap, ~30 fps
constexpr int kThresholdDefaultF = 79;      // ~26 °C
constexpr int kThresholdMinF = 32;          // DHT11 range: 0-50 °C
constexpr int kThresholdMaxF = 122;
constexpr float kHysteresisF = 1.0f;

// Alarm beep: two short chirps, then a pause; repeats while alarming.
const ToneSequence::Step kAlarmBeep[] = {{2700, 120}, {0, 80}, {2700, 120}, {0, 680}};

constexpr StatusLed::Color kLedOk = {0, 12, 4};
constexpr StatusLed::Color kLedAlarm = {16, 0, 0};

ThermostatScreen g_screen(pins::kTftCs, pins::kTftRst, pins::kTftDc);
DhtSensor g_dht(pins::kDht);
EncoderInput g_encoder(pins::kEncoderButton, pins::kEncoderClk, pins::kEncoderDt);
Buzzer g_buzzer(pins::kBuzzer);
StatusLed g_led(pins::kNeoPixel, kLedOk);
ThresholdAlarm g_alarm(static_cast<float>(kThresholdDefaultF), kHysteresisF);
ToneSequence g_beeps(kAlarmBeep, sizeof(kAlarmBeep) / sizeof(kAlarmBeep[0]), true);
Periodic g_sensorTimer(kSensorPeriodMs);
Periodic g_frameTimer(kFramePeriodMs);

ThermostatView g_view;
bool g_dirty = true;  // view changed since the last redraw
AlarmState g_lastAlarm = AlarmState::kIdle;
uint32_t g_sensorFailures = 0;

const char* alarmName(AlarmState s) {
  switch (s) {
    case AlarmState::kIdle:     return "idle";
    case AlarmState::kAlarming: return "ALARM";
    case AlarmState::kSilenced: return "silenced";
  }
  return "?";
}

void readSensor() {
  const DhtSensor::Reading r = g_dht.read();
  if (!r.ok) {
    ++g_sensorFailures;
    Serial.printf("[dht] ERROR: read failed (%" PRIu32 " so far), check the module in port 3\n",
                  g_sensorFailures);
    g_view.status = Status::kSensorError;
    g_dirty = true;
    return;
  }
  g_view.has_reading = true;
  g_view.temp_f = conversions::celsiusToFahrenheit(r.temp_c);
  g_view.humidity_pct = r.humidity_pct;
  g_alarm.update(g_view.temp_f);
  Serial.printf("[dht] %.1f F, %.0f %% RH, threshold %d F, alarm %s\n",
                static_cast<double>(g_view.temp_f), static_cast<double>(g_view.humidity_pct),
                g_view.threshold_f, alarmName(g_alarm.state()));
  g_dirty = true;
}

/// React to alarm state changes: buzzer, LED, log, screen status.
void applyAlarmState(uint32_t now) {
  const AlarmState s = g_alarm.state();
  if (g_view.has_reading && g_view.status != Status::kSensorError) {
    const Status wanted = (s == AlarmState::kAlarming)   ? Status::kAlarm
                          : (s == AlarmState::kSilenced) ? Status::kSilenced
                                                         : Status::kOk;
    if (g_view.status != wanted) {
      g_view.status = wanted;
      g_dirty = true;
    }
  }
  if (s == g_lastAlarm) {
    return;
  }
  Serial.printf("[alarm] %s -> %s\n", alarmName(g_lastAlarm), alarmName(s));
  if (s == AlarmState::kAlarming) {
    g_beeps.start(now);
  } else {
    g_beeps.stop();
  }
  g_led.setColor(s == AlarmState::kAlarming ? kLedAlarm : kLedOk);
  g_lastAlarm = s;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  const uint32_t start = millis();
  while (!Serial && millis() - start < kSerialWaitMs) {
    delay(10);  // bounded wait for a monitor; setup only
  }
  Serial.println("\n[kit_demo] starting");

  if (!g_screen.begin()) {
    Serial.println("[screen] ERROR: no memory for the frame buffer, or SPI failed to start");
  }
  g_dht.begin();
  g_encoder.begin();
  g_buzzer.play(0);
  g_led.begin();

  g_view.threshold_f = kThresholdDefaultF;
  g_view.status = Status::kWaiting;
  g_sensorTimer.reset(millis() - kSensorPeriodMs);  // first read right away
  Serial.printf("[kit_demo] threshold %d F; turn the knob to change, press to silence\n",
                g_view.threshold_f);
}

void loop() {
  const uint32_t now = millis();

  // Input first: the encoder is sampled here, so this runs every pass.
  const EncoderInput::Event in = g_encoder.poll(now);
  if (in.steps != 0) {
    g_view.threshold_f =
        std::clamp(g_view.threshold_f + static_cast<int>(in.steps), kThresholdMinF, kThresholdMaxF);
    g_alarm.setThreshold(static_cast<float>(g_view.threshold_f));
    if (g_view.has_reading) {
      g_alarm.update(g_view.temp_f);  // re-evaluate against the new threshold
    }
    Serial.printf("[knob] threshold %d F\n", g_view.threshold_f);
    g_dirty = true;
  }
  if (in.pressed) {
    Serial.println("[knob] pressed");
    g_alarm.silence();
  }

  if (g_sensorTimer.due(now)) {
    readSensor();
  }
  applyAlarmState(now);

  uint16_t freq = 0;
  if (g_beeps.update(now, &freq)) {
    g_buzzer.play(freq);
  }

  // Redraw only when something changed, and at most ~30 times a second, so
  // SPI traffic doesn't starve the encoder or disturb the buzzer.
  if (g_dirty && g_frameTimer.due(now)) {
    g_screen.draw(g_view);
    g_dirty = false;
  }

  g_led.update(now);
}

#else  // !KIT_DEMO_MODULES_OK

// Stub: the board config lacks a module kit_demo needs. It reports which one
// once a second (so a monitor attached late still sees it) and shows a red
// heartbeat.
#include "periodic.h"
#include "status_led.h"

namespace {
StatusLed g_led(pins::kNeoPixel, {16, 0, 0});
Periodic g_report(1000);

void reportMissing() {
  Serial.println("[kit_demo] ERROR: needs these modules in include/board_config.h:");
  if (!HAS_TFT) Serial.println("[kit_demo]   - screen (MOD_TFT, AX22-0034)");
  if (!HAS_BUZZER) Serial.println("[kit_demo]   - buzzer (MOD_BUZZER, AX22-0018)");
  if (!HAS_DHT11) Serial.println("[kit_demo]   - DHT11 sensor (MOD_DHT11, AX22-0011)");
  if (!HAS_ENCODER) Serial.println("[kit_demo]   - rotary encoder (MOD_ENCODER, AX22-0003)");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  g_led.begin();
}

void loop() {
  const uint32_t now = millis();
  if (g_report.due(now)) reportMissing();
  g_led.update(now);
}

#endif  // KIT_DEMO_MODULES_OK
