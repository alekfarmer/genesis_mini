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
| `arcade`    | Boots to a game menu: **Snake**, **Shooter**, **Beats** (a beat maker), **Blockfall**, **Hide+Seek** (2 players), **Penalty**, plus **Chameleon** when the LED button is plugged in. Only games your modules support are built. Turn the knob to choose, press it to start; hold the knob for 1.5 s in any game to return to the menu. Best scores are saved in flash. |
| `arcade_led_button` | The same arcade for a board with the Tactile LED Button in port 3 instead of the DHT11, so Chameleon is included. |
| `probe`     | Experiment: reads every port pin with pull-up and pull-down (read-only) and prints what it sees next to the configured module, to learn which modules can be detected. |

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

Modules plug into the AX22 ports, so no loose wires are needed. **Tell the
firmware what's where in `include/board_config.h`**, then rebuild:

```c
#define PORT1_MODULE MOD_TFT       // ST7735 160x80 TFT screen (SPI), AX22-0034
#define PORT2_MODULE MOD_BUZZER    // Passive buzzer, AX22-0018
#define PORT3_MODULE MOD_DHT11     // DHT11 temperature & humidity, AX22-0011
#define PORT4_MODULE MOD_ENCODER   // Rotary encoder with push button, AX22-0003
```

That's the Starter Kit as wired here. The other known module is
`MOD_LED_BUTTON` (Tactile LED Button, AX22-0050); `MOD_NONE` marks an empty
port. The full list is in `include/modules.h`.

- **Pins:** every module's pins are worked out from its port.
- **Bad configs:** the build stops with a clear error if a module is unknown or
  appears twice.
- **What gets built:**
  - The arcade includes only games whose modules are configured.
  - `kit_demo` and the arcade build a stub that names the missing modules if
    their essentials aren't there.
- **Swapping modules:** to build for a different wiring without editing the
  file, add a config under `include/board_configs/` and a project that points
  at it, as `arcade_led_button` does.

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
| Hide+Seek *(2 players)* | move between hiding spots | hide / search / continue | tap: same as knob press |
| Chameleon *(needs LED button)* | steer | stop / walk | **LED button:** camouflage |
| Penalty | aim | lock aim, then stop the power meter to shoot | tap: same as knob press |

- **Hide+Seek:** player 1 picks one of 12 spots while player 2 looks away,
  then hands over the board. Player 2 has 6 searches. Each miss says **HOT**,
  **WARM** or **COLD** (on screen, as a beep pitch and as the NeoPixel colour).
  Searches used count as points (7 if not found). Players swap roles each
  round, and the fewest points after 4 rounds wins.
- **Chameleon:** seekers wander with sight cones (the red lines); if one sees
  you, you're caught.
  - **Hiding:** next to a coloured object, the LED button's light comes on.
    Press it to take that object's colour for 3 s. The NeoPixel shows it, and
    seekers can't see you while you stay by that object.
  - **Recharge:** after that, camouflage takes 3 s to recharge.
  - **Getting harder:** a new seeker joins every 20 s.
  - **Score:** seconds survived.
- **Penalty:** turn to aim, press to lock, and press again to stop the
  swinging power meter.
  - **Power:** more power shoots higher. Past the red mark it goes over the bar,
    and above ~70 it drifts off target.
  - **Rounds:** 5 shots per round. Score 3 to go through to a harder round.
  - **Score:** total goals.
- **Back to the menu:** in any game, hold the knob for 1.5 s. The press itself
  still counts once first (one rotation, one shot, one step toggled).
- **Game over:** Snake, Shooter, Blockfall, Chameleon and Penalty show a panel
  with your score and best. Press the knob or tap the button to play again.
  Presses in the first 0.6 s are ignored.
- **Best scores** are kept in flash, one per game, and shown in the menu. The
  beat pattern is kept while the board stays powered, but not across power-off.

### Checking the arcade on the board

1. **Menu:** `ARCADE` and `HOLD KNOB: MENU` on top. Six games with the
   Starter Kit wiring, seven with `arcade_led_button`, scrolling past five.
   Blockfall is tagged `[TALL]`. Turning moves the highlight and wraps around.
   Best scores appear after you've set one. The start-up log lists what's
   configured in each port.
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
7. **Hide+Seek:** the map shows 12 spots (trees, bushes, rocks, a shed, logs,
   a well) and the amber cursor sweeps left to right. Hiding shows a "take the
   board / look away" screen. Each miss marks the spot with an X and shows
   HOT/WARM/COLD with a matching beep and NeoPixel colour. Finding them plays a
   rising jingle and reveals the spot in pink. After 4 rounds the totals and
   the winner show.
8. **Chameleon** (`arcade_led_button` wiring): you're the white dot with an
   amber nose; seekers are red with cone lines. Next to an object the LED
   button lights up and the HUD says `PRESS TO HIDE`. Pressing it turns you,
   and the NeoPixel, the object's colour for 3 s (`HIDDEN 2.4s`), then
   `RECHARGE`. Walking away ends it early. A seeker that sees you ends the
   game with a falling sound.
9. **Penalty:** goal with net, an orange goalie, the ball on the spot. Turning
   moves the yellow aim cross; a press locks it and starts the power meter
   (green / amber / red with a red over-the-bar line); a second press shoots
   with a thud. `GOAL!`, `SAVED`, `WIDE` or `OVER` shows with a sound, and the
   shot dots top centre fill in. After 5 shots a round panel shows; 3 goals go
   through, fewer end the game.
10. **Best scores survive power-off:** set a score in a game, unplug, replug.
   The menu still shows it. Blockfall keeps the best from the old standalone
   version.
11. **Serial:** `make log PORT=... SECONDS=60` while switching games shows
   `[arcade] -> <game> (free heap ...)` lines with a steady free-heap figure,
   and no `ERROR` lines.

### Adding a game

1. Put the rules in `lib/<game>_core/` as pure logic with host tests in
   `test/test_<game>_core/`.
2. Put the input, drawing and sound in `lib/<game>_app/`, as a class that
   implements `arcade::App` (`lib/arcade_app/arcade_app.h`).
3. Add one instance and one entry to the list in `src/arcade/apps.cpp`. If
   the game needs a module beyond the screen and knob, wrap both in
   `#if HAS_<MODULE>` so it's only built when that module is configured.

## Detecting modules (experiment)

The screen and buzzer can't be detected: they only receive. The `probe`
project tests what *can* be: it reads every port pin with the chip's weak
pull-up, then pull-down, and never drives a pin.

```sh
make upload PROJECT=probe PORT=/dev/cu.usbmodem1101
make log PORT=/dev/cu.usbmodem1101 SECONDS=10
```

It prints one row per port (the values here are illustrative, not measured):

```
[probe] port  configured            IO0          IO1          IO2
[probe] P3    DHT11 AX22-0011       floating     pulled HIGH  floating
```

Run it with each module plugged in, then with that port empty, and keep both
logs. A pin that reads "pulled HIGH" or "pulled LOW" with a module and
"floating" without it tells the two apart. Those results decide what boot-time
detection can warn about. It will only warn, never override
`board_config.h`.

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
