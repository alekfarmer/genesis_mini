#include "panel.h"

#include <Adafruit_ST7735.h>
#include <SPI.h>

#include <new>

namespace panel {
namespace {

constexpr int16_t kLong = 160;
constexpr int16_t kShort = 80;

// SPIClass must be FSPI on the ESP32-S3 (catalog). The panel driver and the
// canvas are created once in begin() and live for the whole program.
SPIClass g_spi(FSPI);
Adafruit_ST7735* g_tft = nullptr;
GFXcanvas16* g_canvas = nullptr;

}  // namespace

bool begin(uint8_t cs, uint8_t rst, uint8_t dc, Orientation orientation) {
  if (g_canvas != nullptr) {
    return true;  // already started
  }
  const bool landscape = (orientation == Orientation::kLandscape);
  g_canvas = new (std::nothrow) GFXcanvas16(landscape ? kLong : kShort, landscape ? kShort : kLong);
  if (g_canvas == nullptr || g_canvas->getBuffer() == nullptr) {
    return false;
  }
  // Catalog init for AX22-0034, in order. MISO is included so the shared bus
  // also works for other SPI modules.
  static Adafruit_ST7735 tft(&g_spi, cs, dc, rst);
  g_tft = &tft;
  g_spi.begin(SCK, MISO, MOSI);
  if (g_spi.bus() == nullptr) {
    return false;
  }
  tft.initR(INITR_MINI160x80);
  tft.setRotation(static_cast<uint8_t>(orientation));
  tft.fillScreen(0);  // one-time clear, allowed outside the draw loop
  g_canvas->setTextWrap(false);
  return true;
}

GFXcanvas16& canvas() { return *g_canvas; }

int16_t width() { return g_canvas != nullptr ? g_canvas->width() : 0; }

int16_t height() { return g_canvas != nullptr ? g_canvas->height() : 0; }

void push() {
  if (g_tft == nullptr || g_canvas == nullptr) {
    return;
  }
  g_tft->drawRGBBitmap(0, 0, g_canvas->getBuffer(), g_canvas->width(), g_canvas->height());
}

}  // namespace panel
