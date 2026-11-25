#include "thermostat_screen.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "conversions.h"

namespace {

constexpr int16_t kWidth = 160;
constexpr int16_t kHeight = 80;

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

// One SPI peripheral, one panel, one 160x80 16-bit canvas (25.6 KB, allocated
// once at startup, never in the loop). SPIClass must be FSPI on the ESP32-S3.
SPIClass g_spi(FSPI);
GFXcanvas16 g_canvas(kWidth, kHeight);
Adafruit_ST7735* g_tft = nullptr;

void printAt(int16_t x, int16_t y, uint8_t size, uint16_t color, const char* text) {
  g_canvas.setTextSize(size);
  g_canvas.setTextColor(color);
  g_canvas.setCursor(x, y);
  g_canvas.print(text);
}

}  // namespace

ThermostatScreen::ThermostatScreen(uint8_t cs, uint8_t rst, uint8_t dc) : cs_(cs), rst_(rst), dc_(dc) {}

bool ThermostatScreen::begin() {
  // GFXcanvas16 allocates its 25.6 KB buffer at startup; drawing without it
  // would crash, so refuse to start instead.
  if (g_canvas.getBuffer() == nullptr) {
    return false;
  }
  // Catalog init for AX22-0034, in order. MISO is included so the shared bus
  // also works for other SPI modules.
  static Adafruit_ST7735 tft(&g_spi, cs_, dc_, rst_);
  g_tft = &tft;
  g_spi.begin(SCK, MISO, MOSI);
  if (g_spi.bus() == nullptr) {
    return false;
  }
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(kBlack);  // one-time clear, allowed outside the draw loop
  g_canvas.setTextWrap(false);
  ready_ = true;
  return true;
}

void ThermostatScreen::draw(const ThermostatView& view) {
  if (!ready_) {
    return;
  }
  char text[12];
  g_canvas.fillScreen(kBlack);

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

  g_tft->drawRGBBitmap(0, 0, g_canvas.getBuffer(), kWidth, kHeight);
}
