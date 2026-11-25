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
| `kit_demo`  | Over-temperature alarm using every Starter Kit module: the screen shows temperature, humidity and a threshold; turn the knob to set the threshold; above it, the buzzer beeps and the NeoPixel turns red; press the knob to silence. |
| `arcade`    | Boots to a game menu: **Snake**, **Shooter**, **Beats** (a beat maker) and **Blockfall**. Turn the knob to choose, press it to start; hold the knob for 1.5 s in any game to return to the menu. Best scores are saved in flash. |

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
| 1 | ST7735 160×80 TFT screen (SPI), AX22-0034 |
| 2 | Passive buzzer, AX22-0018 |
| 3 | DHT11 temperature & humidity sensor, AX22-0011 |
| 4 | Rotary encoder with push button, AX22-0003 |

Each port carries 3.3 V, GND, three unique I/O pins, and the shared SPI and I2C
buses. All logic is **3.3 V**: never connect a 5 V signal to a port or pad. The
STEMMA QT connector on the underside is the same I2C bus as the ports. Exact
pins are in `include/pins.h`.

## Checking `hello` on the board

After `make upload PROJECT=hello ...`, run `make monitor ...`. The report is
printed whenever a monitor connects, so there's no need to press RESET.
Check that:

1. Serial shows `[board] ESP32-S3 ...`, `flash 4096 KB` and `PSRAM 2048 KB`
   (the PSRAM size may read a little under 2048).
2. It shows an I2C scan result. The Starter Kit modules don't use I2C, so
   `0 device(s)` is normal. A STEMMA QT breakout would show up here.
3. It ends with `[hello] all checks passed`.
4. The NeoPixel blinks a green double-pulse about once a second. Red means a
   check failed, so read the log.
5. `[hello] uptime ...` lines appear every 10 seconds.

Quit the monitor (Ctrl+C) before the next upload: it holds the port.

## Checking `kit_demo` on the board

After `make upload PROJECT=kit_demo ...` and `make monitor ...`:

1. **Screen:** shows `TEMP` and `HUMIDITY` with values (`--.-F` until the
   first reading, within 2 s), `SET 79F` in amber and `OK` in green. Text is
   upright and nothing is cut off at the edges. Colours look right: green
   `OK`, amber `SET`. If red and blue look swapped, the BGR handling is wrong.
2. **Sensor:** serial prints a `[dht] ... F, ... % RH` line every 2 s, and the
   temperature looks plausible for the room. Breathing on the sensor should
   raise both values within a few readings.
3. **Knob, turning:** each click changes `SET` by 1 °F, and serial prints
   `[knob] threshold ...`. Clockwise should increase it. If it decreases,
   flip the sign of `kDirection` in `lib/encoder_input/encoder_input.cpp`.
4. **Alarm:** turn `SET` below the current temperature. The status becomes
   red `ALARM!`, the buzzer chirps twice about once a second, the NeoPixel
   heartbeat turns red, and serial prints `[alarm] idle -> ALARM`.
5. **Knob, pressing:** the buzzer stops, the status becomes amber
   `SILENCED`, and the NeoPixel goes back to green.
6. **Re-arm:** turn `SET` at least 2 °F above the temperature. The status
   returns to `OK`. Turn it back below and the alarm sounds again.
7. **Sensor unplugged** (optional): within 2 s the status shows `NO SENSR`
   and serial logs `[dht] ERROR: read failed`.

## The arcade

`make upload PROJECT=arcade ...` boots to the menu. The "on-board button" is
the unlabelled one, not BOOT or RESET.

| Where | Knob turn | Knob press | On-board button |
|-------|-----------|------------|-----------------|
| Menu | choose a game | start it | |
| Snake | steer: each click turns left or right | | tap: pause / resume |
| Shooter | move the ship (clockwise = up) | fire | hold: rapid fire |
| Beats | move the cursor over the grid | switch the step on / off | tap: play / stop; hold + turn: tempo (2 BPM per click) |
| Blockfall *(hold the board tall)* | move the piece | rotate | tap: drop; hold: fall faster |

- **Back to the menu:** in any game, hold the knob for 1.5 s. The press itself
  still counts once first (one rotation, one shot, one step toggled).
- **Game over:** Snake, Shooter and Blockfall show a panel with your score and
  best. Press the knob or tap the button to play again. Presses in the first
  0.6 s are ignored.
- **Best scores** are kept in flash, one per game, and shown in the menu. The
  beat pattern is kept while the board stays powered, but not across power-off.

### Checking the arcade on the board

1. **Menu:** `ARCADE` and `HOLD KNOB: MENU` on top, four games listed,
   Blockfall tagged `[TALL]`. Turning moves the highlight and wraps around.
   Best scores appear after you've set one.
2. **Back gesture:** from each game, holding the knob 1.5 s returns to the
   menu, with the screen back in landscape after Blockfall.
3. **Snake:** the snake moves right at the start; clockwise turns it right.
   Food (red) makes it grow, beep and speed up. Walls and its own body end the
   game. A tap pauses it.
4. **Shooter:** asteroids drift in from the right within a couple of seconds.
   Clockwise moves the ship up. A press fires one shot; holding the button
   fires continuously. A hit asteroid vanishes with a low thud and scores.
   Being hit costs a life (the ship icons top right), flashes the NeoPixel red
   and makes the ship blink for a second. Three hits end the game.
5. **Beats:** a starter beat is loaded and `STOPPED` shows. A tap starts it.
   The playhead (white frame) moves across in time and the buzzer plays kick,
   snare, hi-hat and bleep sounds. Turning moves the amber cursor; a press
   toggles a step (it previews its sound when stopped). Holding the button
   while turning changes the tempo shown top left (60-200 BPM).
6. **Blockfall:** as before, now inside the arcade. Score and `LV 1` top left,
   the next piece top right, the well below. Clockwise moves right, a press
   rotates, a tap drops, a hold falls fast. Line clears flash the NeoPixel
   white. If the picture is upside down, change `kPortrait` to
   `kPortraitFlipped` in `BlockfallApp::orientation()`
   (`lib/blockfall_app/blockfall_app.h`).
7. **Best scores survive power-off:** set a score in a game, unplug, replug.
   The menu still shows it. Blockfall keeps the best from the old standalone
   version.
8. **Serial:** `make log PORT=... SECONDS=60` while switching games shows
   `[arcade] -> <game> (free heap ...)` lines with a steady free-heap figure,
   and no `ERROR` lines.

### Adding a game

1. Put the rules in `lib/<game>_core/` as pure logic with host tests in
   `test/test_<game>_core/`.
2. Put the input, drawing and sound in `lib/<game>_app/`, as a class that
   implements `arcade::App` (`lib/arcade_app/arcade_app.h`).
3. Add one instance and one entry to the list in `src/arcade/apps.cpp`.

## Layout

See [ARCHITECTURE.md](ARCHITECTURE.md). To add a standalone project (rather
than an arcade game), create `src/<name>/main.cpp` and add this to
`platformio.ini`:

```ini
[env:<name>]
extends = base
build_src_filter = +<<name>/>
```

## Secrets

Wi-Fi credentials go in `include/secrets.h`, which is gitignored. Copy
`include/secrets.example.h` to start. Secrets are never printed to serial.
