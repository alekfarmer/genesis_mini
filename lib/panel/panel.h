#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

/// The kit's 160x80 ST7735 TFT (AX22-0034) on the shared SPI bus, with an
/// off-screen canvas. Projects draw everything into canvas() and call push()
/// once per frame, which avoids flicker (catalog requirement). One panel per
/// board, so this is a single static object.
namespace panel {

/// Panel orientations. Landscape is the catalog's setRotation(3); portrait
/// turns the board a quarter turn (setRotation(0); use 2 if it's upside down).
enum class Orientation : uint8_t { kLandscape = 3, kPortrait = 0, kPortraitFlipped = 2 };

/// Start SPI, initialise the panel and allocate the canvas (once, 25.6 KB).
/// Returns false if the canvas could not be allocated or SPI failed to start.
/// The panel gives no acknowledgement over SPI, so a disconnected screen can
/// only be spotted by eye.
bool begin(uint8_t cs, uint8_t rst, uint8_t dc, Orientation orientation);

/// Canvas to draw into; sized for the orientation (160x80 or 80x160).
/// Only valid after a successful begin().
GFXcanvas16& canvas();

int16_t width();
int16_t height();

/// Send the whole canvas to the panel (~25 KB over SPI, a few ms).
void push();

}  // namespace panel
