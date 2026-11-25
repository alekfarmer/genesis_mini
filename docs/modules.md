# Modules and ports

The Genesis Mini Starter Kit modules, and the AX22 port each one is plugged
into. This file is the source of truth for wiring: if a module or port is not
listed here, it is not connected. Pin numbers live in `include/pins.h`, and
per-module pin roles come from the Axiometa catalog (`get_module`).

| AX22 port | Module                      | Catalog id | Interface | Pins used |
|-----------|-----------------------------|------------|-----------|-----------|
| 1         | ST7735 160×80 IPS TFT       | AX22-0034  | SPI (shared bus) | IO0 = CS, IO1 = RST, IO2 = DC |
| 2         | Passive buzzer              | AX22-0018  | PWM       | IO1 |
| 3         | DHT11 temperature & humidity | AX22-0011 | single-wire (pull-up on module) | IO1 = data |
| 4         | Rotary encoder + button     | AX22-0003  | GPIO      | IO0 = button (active HIGH), IO1 = CLK, IO2 = DT |

On-board (no port): NeoPixel (GPIO21), user button (GPIO45), STEMMA QT
(I2C: SDA GPIO10, SCL GPIO11), USB-C.
