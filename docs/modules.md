# Modules and ports

**The source of truth is `include/board_config.h`.** It says which module is
in which AX22 port, and the firmware derives every module's pins from it.
Edit that file when you move a module, then rebuild. This page explains the
modules; it doesn't decide anything.

Current wiring (the Genesis Mini Starter Kit):

| AX22 port | Module | `board_config.h` name | Interface | Pins used |
|-----------|--------|-----------------------|-----------|-----------|
| 1 | ST7735 160×80 IPS TFT (AX22-0034) | `MOD_TFT` | SPI (shared bus) | IO0 = CS, IO1 = RST, IO2 = DC |
| 2 | Passive buzzer (AX22-0018) | `MOD_BUZZER` | PWM | IO1 |
| 3 | DHT11 temperature & humidity (AX22-0011) | `MOD_DHT11` | single-wire (pull-up on module) | IO1 = data |
| 4 | Rotary encoder + button (AX22-0003) | `MOD_ENCODER` | GPIO | IO0 = button (active HIGH), IO1 = CLK, IO2 = DT |

Other supported modules:

| Module | `board_config.h` name | Pins used |
|--------|-----------------------|-----------|
| Tactile LED Button (AX22-0050) | `MOD_LED_BUTTON` | IO1 = button (LOW when pressed; pull-up and RC debounce on the module), IO2 = white LED (HIGH = on) |

On-board (no port): NeoPixel (GPIO21), user button (GPIO45), STEMMA QT
(I²C: SDA GPIO10, SCL GPIO11), USB-C.

Pin roles come from the Axiometa catalog (`get_module`), never from memory.
