#include "thermostat_screen.h"

#include <stdio.h>

#include "conversions.h"
#include "panel.h"

namespace {

// Layout (catalog rule: everything inside 160x80; size 1 = 6x8 px, size 2 =
// 12x16 px per character). Two columns on top, two full-width rows below.
//   y  2: "TEMP" / "HUMIDITY" labels, size 1        (ends y 10)
//   y 12: temperature / humidity values, size 2     (ends y 28, max 6 chars = 72 px)
//   y 36: "SET 79F", size 2                         (ends y 52)
//   y 60: status word, size 2                       (ends y 76, max 8 chars = 96 px)
constexpr int16_t kLeftX = 4;
constexpr int16_t kRightX = 84;
constexpr int16_t kLabelY = 2;
constexpr int16_t kValueY = 12;
constexpr int16_t kSetY = 36;
constexpr int16_t kStatusY = 60;

// Colours go through conversions::panelColor because the panel is BGR.
const uint16_t kBlack = conversions::panelColor(0, 0, 0);
const uint16_t kWhite = conversions::panelColor(255, 255, 255);
const uint16_t kGrey = conversions::panelColor(140, 140, 140);
const uint16_t kGreen = conversions::panelColor(0, 220, 80);
const uint16_t kRed = conversions::panelColor(255, 40, 40);
const uint16_t kAmber = conversions::panelColor(255, 180, 0);

void printAt(int16_t x, int16_t y, uint8_t size, uint16_t color, const char* text) {
  GFXcanvas16& c = panel::canvas();
  c.setTextSize(size);
  c.setTextColor(color);
  c.setCursor(x, y);
  c.print(text);
}

}  // namespace

ThermostatScreen::ThermostatScreen(uint8_t cs, uint8_t rst, uint8_t dc) : cs_(cs), rst_(rst), dc_(dc) {}

bool ThermostatScreen::begin() {
  ready_ = panel::begin(cs_, rst_, dc_, panel::Orientation::kLandscape);
  return ready_;
}

void ThermostatScreen::draw(const ThermostatView& view) {
  if (!ready_) {
    return;
  }
  char text[12];
  panel::canvas().fillScreen(kBlack);

  printAt(kLeftX, kLabelY, 1, kGrey, "TEMP");
  printAt(kRightX, kLabelY, 1, kGrey, "HUMIDITY");
  if (view.has_reading) {
    snprintf(text, sizeof(text), "%.1fF", static_cast<double>(view.temp_f));
    printAt(kLeftX, kValueY, 2, kWhite, text);
    snprintf(text, sizeof(text), "%.0f%%", static_cast<double>(view.humidity_pct));
    printAt(kRightX, kValueY, 2, kWhite, text);
  } else {
    printAt(kLeftX, kValueY, 2, kGrey, "--.-F");
    printAt(kRightX, kValueY, 2, kGrey, "--%");
  }

  snprintf(text, sizeof(text), "SET %dF", view.threshold_f);
  printAt(kLeftX, kSetY, 2, kAmber, text);

  switch (view.status) {
    case ThermostatView::Status::kWaiting:     printAt(kLeftX, kStatusY, 2, kGrey, "WAITING"); break;
    case ThermostatView::Status::kOk:          printAt(kLeftX, kStatusY, 2, kGreen, "OK"); break;
    case ThermostatView::Status::kAlarm:       printAt(kLeftX, kStatusY, 2, kRed, "ALARM!"); break;
    case ThermostatView::Status::kSilenced:    printAt(kLeftX, kStatusY, 2, kAmber, "SILENCED"); break;
    case ThermostatView::Status::kSensorError: printAt(kLeftX, kStatusY, 2, kRed, "NO SENSR"); break;
  }

  panel::push();
}
