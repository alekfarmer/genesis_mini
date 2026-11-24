#pragma once

class Print;

/// One-shot boot diagnostics, printed to serial at startup so a fresh board
/// can be checked against its spec (ESP32-S3, 4 MB flash, 2 MB PSRAM).
namespace board_info {

/// Print chip model and revision, CPU frequency, flash and PSRAM size, and the
/// reason for the last reset. Returns false (and says why) if the board does
/// not match the Genesis Mini spec, e.g. PSRAM missing or the wrong flash size.
bool printBootReport(Print& out);

}  // namespace board_info
