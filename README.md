# Genesis Mini firmware

Firmware projects for the [Axiometa Genesis Mini](https://axiometa.io)
(ESP32-S3, 4 MB flash, 2 MB PSRAM, on-board NeoPixel, STEMMA QT, four AX22
module ports), written in C++ on the Arduino framework and built with
PlatformIO.

The repo holds several **projects**. Each one is a complete program for the
board, and you choose which one to flash.

| Project     | What it does |
|-------------|--------------|
| `hello`     | First flash on a new board: boot report (chip, flash, PSRAM, reset reason), I2C scan, NeoPixel heartbeat (green = OK, red = a check failed). |
| `kit_demo`  | *(coming)* Uses the Starter Kit modules: screen, buzzer, temperature/humidity sensor, rotary encoder. |

## Requirements

- macOS or Linux with `python3` (3.9+) and `make`.
- A C/C++ compiler for the host unit tests (on macOS: `xcode-select --install`).
- A USB-C **data** cable. The ESP32-S3's native USB needs no driver.

Nothing is installed globally. `make setup` puts the whole toolchain inside
this folder (`.venv/` and `.pio-core/`, about 2.3 GB).

## Set up, build, test

```sh
make setup     # once: Python venv + PlatformIO + ESP32-S3 toolchain, all local
make verify    # build every project (warnings are errors) + run unit tests
make help      # list all targets
```

- `make build`: build every project. `make build PROJECT=hello` builds just one.
- `make test`: run the host unit tests.
- `make projects`: list the projects.
- `make clean`: remove build output. The toolchain is kept.

## Flash a project

1. Plug the board in over USB-C.
2. Find its port: `make ports`. On macOS it looks like `/dev/cu.usbmodem1101`.
3. Flash a project:
   ```sh
   make upload PROJECT=hello PORT=/dev/cu.usbmodem1101
   ```
4. Watch its output (quit with Ctrl+C):
   ```sh
   make monitor PORT=/dev/cu.usbmodem1101
   ```

If the upload can't connect, put the board in download mode: hold **BOOT**,
tap **RESET**, release **BOOT**, then upload again. Press **RESET** afterwards
to run the new firmware.

## Wiring

Modules plug into the AX22 ports, so no loose wires are needed. This setup
uses (see `docs/modules.md`):

| Port | Module |
|------|--------|
| 1 | ST7735 160×80 TFT screen (SPI) |
| 2 | Passive buzzer |
| 3 | Temperature/humidity sensor |
| 4 | Rotary encoder with push button |

Each port carries 3.3 V, GND, three unique I/O pins, and the shared SPI and I2C
buses. All logic is **3.3 V**: never connect a 5 V signal to a port or pad. The
STEMMA QT connector on the underside is the same I2C bus as the ports. Exact
pins are in `include/pins.h`.

## Checking `hello` on the board

After `make upload PROJECT=hello ...` and `make monitor ...`, press RESET and
check that:

1. Serial shows `[board] ESP32-S3 ...`, `flash 4096 KB` and `PSRAM 2048 KB`
   (the PSRAM size may read a little under 2048).
2. It shows `[i2c] bus up on SDA=10 SCL=11` and a scan result. With only the
   kit plugged in, the I2C devices listed depend on which sensor module is in
   port 3.
3. It ends with `[hello] all checks passed`.
4. The NeoPixel blinks a green double-pulse about once a second. Red means a
   check failed, so read the log.
5. `[hello] uptime ...` lines appear every 10 seconds.

## Layout

See [ARCHITECTURE.md](ARCHITECTURE.md). To add a project, create
`src/<name>/main.cpp` and add this to `platformio.ini`:

```ini
[env:<name>]
extends = base
build_src_filter = +<<name>/>
```

## Secrets

Wi-Fi credentials go in `include/secrets.h`, which is gitignored. Copy
`include/secrets.example.h` to start. Secrets are never printed to serial.
