#include "board_info.h"

#include <Arduino.h>
#include <esp_system.h>

namespace board_info {
namespace {

constexpr uint32_t kExpectedFlashBytes = 4u * 1024u * 1024u;

const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "external pin";
    case ESP_RST_SW:        return "software restart";
    case ESP_RST_PANIC:     return "panic (crash)";
    case ESP_RST_INT_WDT:   return "interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "task watchdog";
    case ESP_RST_WDT:       return "other watchdog";
    case ESP_RST_DEEPSLEEP: return "wake from deep sleep";
    case ESP_RST_BROWNOUT:  return "brownout (low supply voltage)";
    case ESP_RST_SDIO:      return "SDIO";
    case ESP_RST_USB:       return "USB";
    case ESP_RST_JTAG:      return "JTAG";
    default:                return "unknown";
  }
}

}  // namespace

bool printBootReport(Print& out) {
  bool ok = true;
  const uint32_t flash = ESP.getFlashChipSize();
  const esp_reset_reason_t reason = esp_reset_reason();

  out.printf("[board] %s rev %u, %u cores @ %" PRIu32 " MHz\n", ESP.getChipModel(),
             static_cast<unsigned>(ESP.getChipRevision()),
             static_cast<unsigned>(ESP.getChipCores()), ESP.getCpuFreqMHz());
  out.printf("[board] flash %" PRIu32 " KB\n", flash / 1024u);
  if (flash != kExpectedFlashBytes) {
    out.printf("[board] ERROR: expected %" PRIu32 " KB flash\n", kExpectedFlashBytes / 1024u);
    ok = false;
  }

  if (psramFound()) {
    out.printf("[board] PSRAM %" PRIu32 " KB\n", ESP.getPsramSize() / 1024u);
  } else {
    out.println("[board] ERROR: PSRAM not found (expected 2 MB)");
    ok = false;
  }

  out.printf("[board] last reset: %s\n", resetReasonName(reason));
  if (reason == ESP_RST_BROWNOUT) {
    out.println("[board] WARNING: brownout - check USB cable or batteries");
  }
  return ok;
}

}  // namespace board_info
