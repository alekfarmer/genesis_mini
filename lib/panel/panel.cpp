#include "panel.h"

#include <Adafruit_ST7735.h>
#include <SPI.h>

#include <new>

namespace panel {
namespace {

constexpr int16_t kLong = 160;
constexpr int16_t kShort = 80;

// SPIClass must be FSPI on the ESP32-S3 (catalog). The panel driver and the
// canvases are created on first use and live for the whole program: one
// landscape (160x80) and, if ever needed, one portrait (80x160).
SPIClass g_spi(FSPI);
Adafruit_ST7735* g_tft = nullptr;
GFXcanvas16* g_landscape = nullptr;
GFXcanvas16* g_portrait = nullptr;
GFXcanvas16* g_active = nullptr;

bool isLandscape(Orientation o) { return o == Orientation::kLandscape; }

/// The canvas for an orientation, allocating it the first time.
GFXcanvas16* canvasFor(Orientation o) {
  GFXcanvas16*& slot = isLandscape(o) ? g_landscape : g_portrait;
  if (slot == nullptr) {
    slot = new (std::nothrow) GFXcanvas16(isLandscape(o) ? kLong : kShort, isLandscape(o) ? kShort : kLong);
    if (slot != nullptr && slot->getBuffer() == nullptr) {
      delete slot;  // the canvas object exists but its pixel buffer doesn't
      slot = nullptr;
    }
    if (slot != nullptr) {
      slot->setTextWrap(false);
    }
  }
  return slot;
}

}  // namespace

bool begin(uint8_t cs, uint8_t rst, uint8_t dc, Orientation orientation) {
  if (g_tft != nullptr) {
    return setOrientation(orientation);  // already started
  }
  g_active = canvasFor(orientation);
  if (g_active == nullptr) {
    return false;
  }
  // Catalog init for AX22-0034, in order. MISO is included so the shared bus
  // also works for other SPI modules.
  static Adafruit_ST7735 tft(&g_spi, cs, dc, rst);
  g_spi.begin(SCK, MISO, MOSI);
  if (g_spi.bus() == nullptr) {
    return false;
  }
  tft.initR(INITR_MINI160x80);
  tft.setRotation(static_cast<uint8_t>(orientation));
  tft.fillScreen(0);  // one-time clear, allowed outside the draw loop
  g_tft = &tft;
  return true;
}

bool setOrientation(Orientation orientation) {
  if (g_tft == nullptr) {
    return false;
  }
  GFXcanvas16* next = canvasFor(orientation);
  if (next == nullptr) {
    return false;
  }
  g_active = next;
  g_tft->setRotation(static_cast<uint8_t>(orientation));
  return true;
}

GFXcanvas16& canvas() { return *g_active; }

int16_t width() { return g_active != nullptr ? g_active->width() : 0; }

int16_t height() { return g_active != nullptr ? g_active->height() : 0; }

void push() {
  if (g_tft == nullptr || g_active == nullptr) {
    return;
  }
  g_tft->drawRGBBitmap(0, 0, g_active->getBuffer(), g_active->width(), g_active->height());
}

}  // namespace panel
