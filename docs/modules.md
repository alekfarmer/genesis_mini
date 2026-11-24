# Modules and ports

The Genesis Mini Starter Kit modules, and the AX22 port each one is plugged
into. This file is the source of truth for wiring: if a module or port is not
listed here, it is not connected. Pin numbers live in `include/pins.h`.

| AX22 port | Module                       | Interface | Catalog id | Notes |
|-----------|------------------------------|-----------|------------|-------|
| 1         | Screen                       | TBD       | TBD        |       |
| 2         | Buzzer                       | TBD       | TBD        |       |
| 3         | Temperature/humidity sensor  | TBD       | TBD        |       |
| 4         | Rotary encoder               | TBD       | TBD        |       |

On-board (no port): NeoPixel, STEMMA QT (I²C) connector, USB-C.

"TBD" entries get filled in from the Axiometa catalog (`get_module`), never
from memory.
