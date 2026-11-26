# Architecture

## Layout

```
Makefile                 the only entry point (build, test, flash, monitor)
platformio.ini           [base] shared settings, one [env:<project>] per project, [env:native] for tests
boards/genesis_mini.json PlatformIO board definition (4 MB flash, QSPI PSRAM, USB CDC)
variants/axiometa_genesis_mini/pins_arduino.h
                         Axiometa's board variant, vendored verbatim (port aliases P{N}_IO{0,1,2})
include/board_config.h   which module is in which AX22 port (edit this for your wiring)
include/board_configs/   alternative wirings, selected per project with BOARD_CONFIG_FILE
include/modules.h        the module types the repo supports (MOD_*)
include/pins.h           checks the config, derives every module's pins from its port, HAS_<MODULE> flags
include/secrets.example.h  template for the gitignored include/secrets.h
lib/                     shared code, one library per responsibility
  periodic/              non-blocking interval timer        (pure, host-tested)
  heartbeat/             status-LED blink pattern            (pure, host-tested)
  debounce/              button press edges with lockout     (pure, host-tested)
  threshold_alarm/       alarm state machine, hysteresis     (pure, host-tested)
  tone_sequence/         non-blocking beep sequencer         (pure, host-tested)
  conversions/           C->F, BGR panel colours             (pure, host-tested)
  tap_hold/              tap vs hold on one button           (pure, host-tested)
  rng/                   deterministic random numbers        (pure, host-tested)
  menu_core/             menu selection and scrolling        (pure, host-tested)
  blockfall_core/        blockfall rules: well, pieces, score (pure, host-tested)
  snake_core/            snake rules                         (pure, host-tested)
  shooter_core/          asteroid shooter rules              (pure, host-tested)
  beat_core/             4x16 step sequencer                 (pure, host-tested)
  hideseek_core/         two-player hide & seek rules        (pure, host-tested)
  chameleon_core/        camouflage-vs-seekers rules         (pure, host-tested)
  penalty_core/          penalty kick vs goalie rules        (pure, host-tested)
  pin_probe/             classify a pin from pull-up/down reads (pure, host-tested)
  led_button/            Tactile LED Button driver           (hardware, AX22-0050)
  panel/                 shared ST7735 + landscape/portrait canvases (hardware, AX22-0034)
  arcade_app/            arcade App interface, host services, drawing helpers (hardware)
  arcade_menu/           the arcade's game menu              (hardware)
  snake_app/  shooter_app/  beat_app/  blockfall_app/
  hideseek_app/  chameleon_app/  penalty_app/
                         each game's input, drawing and sound (hardware)
  blockfall_screen/      blockfall renderer, portrait        (hardware)
  status_led/            NeoPixel driver using heartbeat     (hardware)
  board_info/            boot report: chip, flash, PSRAM     (hardware)
  i2c_bus/               shared I2C bus setup and scan       (hardware)
  thermostat_screen/     kit_demo renderer, landscape        (hardware)
  dht_sensor/            DHT11 wrapper                       (hardware, AX22-0011)
  buzzer/                passive buzzer tone()/noTone()      (hardware, AX22-0018)
  encoder_input/         rotary encoder + button events      (hardware, AX22-0003)
src/<project>/main.cpp   one folder per project; thin setup()/loop() wiring libraries together
test/test_<lib>/         Unity tests, run on the host by `make test`
docs/modules.md          which module is in which port (owner-maintained)
```

## Rules of the design

- **Projects are thin.** `src/<project>/` wires libraries together. Reusable
  logic goes in `lib/`.
- **Pure logic is separate from hardware.** Code that doesn't touch hardware
  (timers, patterns, parsing, state machines) lives in libraries with no
  Arduino dependency and gets host unit tests. Hardware libraries declare
  `"platforms": "espressif32"` in `library.json`, so the native test build
  never pulls them in.
- **Pins come from one place.** The variant defines what each port pin *is*.
  `include/board_config.h` says which module is in which port, and
  `include/pins.h` derives each module's pins from that. Libraries take pins
  as constructor or function arguments and never hard-code them.
- **Builds follow the wiring.**
  - `pins.h` rejects unknown or duplicated modules with `#error`, and defines
    `HAS_TFT`, `HAS_BUZZER`, `HAS_DHT11`, `HAS_ENCODER` and `HAS_LED_BUTTON`
    for use in `#if`.
  - Projects guard module code with these. The arcade's game list
    (`src/arcade/apps.cpp`) includes a game only when its modules are present.
    Without the buzzer, games run silently.
  - A project missing an essential module builds a stub that names the
    missing module, instead of failing, so `make verify` stays green for any
    wiring.
  - A project can use another wiring with
    `build_flags = '-DBOARD_CONFIG_FILE="board_configs/<file>.h"'`, as
    `arcade_led_button` does. That also keeps button-only games compiling on
    every `make verify`.
- **Detection is advisory.** Only some modules can be detected at all. The
  screen and buzzer only receive. The `probe` project measures which pins
  show a module's pull resistors. Any boot-time detection built on it may
  warn, but never overrides `board_config.h`.
- **No blocking in `loop()`.** Timing uses `Periodic` with `millis()`.
  `delay()` is allowed only in a bounded wait in `setup()`.
- **Errors are logged.** Every hardware call whose result can be checked is
  checked, and failures are logged to serial with a `[module] ERROR:` prefix.

## The arcade

- `src/arcade/main.cpp` is the **host**. It owns the screen, buzzer,
  NeoPixel, knob, on-board button, Tactile LED Button and flash storage, and
  runs one `arcade::App` at a time: the menu or a game.
- **Required vs optional:** the screen and the knob are required. The buzzer
  and the LED button are optional. When they aren't configured, sounds are
  silent and `led_button_pressed` stays false.
- Each loop, it turns knob and button activity into an `arcade::Input`, calls
  the app's `update()`, plays the current sound, and calls `draw()` and
  `panel::push()` when `needsRedraw()` says something changed (at most
  ~30 fps).
- **Apps** (`lib/*_app`) implement `arcade::App`: name, best-score key,
  orientation, `enter`/`update`/`needsRedraw`/`draw`/`exit`. They reach the
  hardware only through the host's services in `arcade_app.h`: `playSound`,
  `stopSound`, `setLed`, `setButtonLed`, `loadBest`, `saveBest` and
  `newSeed`.
- Each game's rules live in a separate pure `*_core` library with host tests.
  The app adds only input mapping, drawing and sound.
- **Switching:** a 1.5 s knob hold (the `long_press` field of
  `EncoderInput::Event`) returns
  to the menu from any game. The host calls `exit()` on the old app,
  silences the buzzer, sets the panel orientation for the new app, then calls
  `enter()`.
- **Game list:** `src/arcade/apps.cpp`. Adding a game is one instance plus
  one entry there.
- **Memory:** game objects are static, created once at boot. The panel keeps
  one landscape and, after Blockfall first runs, one portrait canvas
  (25.6 KB each), both allocated once and never freed. Nothing allocates in
  `loop()`.

## Toolchain

- PlatformIO Core 6.2.0 lives in `.venv/`. Its packages live in `.pio-core/`
  (`PLATFORMIO_CORE_DIR`, set by the Makefile).
- Platform: pioarduino `platform-espressif32` 55.03.36-1, which brings
  Arduino-ESP32 core 3.3.6 and ESP-IDF 5.5.2. That is the core version Axiometa
  builds against.
- The stock core has no Genesis Mini variant (it has the Genesis One), so
  Axiometa's variant is vendored in `variants/` and selected by
  `boards/genesis_mini.json` together with `board_build.variants_dir`.
- Warnings are errors for `src/` (`build_src_flags`), for each library (its
  `library.json` `build.flags`) and for the native tests. Framework code is
  compiled with its own flags.
- Third-party libraries go in each project's `lib_deps`, pinned to exact
  versions (the versions the Axiometa catalog lists for each module), and so
  do the transitive dependencies that are actually compiled (e.g. Adafruit
  BusIO). Dependencies a library declares but this repo never uses go in
  `lib_ignore`.
- The screen is owned by `lib/panel` (SPI bus, panel driver, canvases).
  Renderers draw into `panel::canvas()`, and `panel::push()` sends it.
  `panel::setOrientation()` switches between landscape, which follows the
  catalog (`setRotation(3)`), and portrait (`setRotation(0)`, used by
  Blockfall). Portrait is a departure from the catalog.
- Module code follows the catalog's coding notes: the ST7735 draws into a
  canvas pushed once per frame, only when the view changed and at most
  ~30 fps (this also keeps SPI from disturbing tone()); the DHT11 is read at
  most every 2 s; the encoder uses LatchMode::TWO03 and an active-high
  button with a 150 ms lockout.
- `axiometa-cli` (also in `.venv/`) is used only as a read-only reference for
  board and module facts, through the Claude Code MCP server in `.mcp.json`
  (gitignored; see `.mcp.json.example`). Its compile, upload and serial tools
  are not used: builds and flashing go through the Makefile.

## Flash layout

Default 4 MB partition table (`default.csv`): 1.25 MB app ×2 (OTA) plus
SPIFFS. No eFuse, secure boot or flash-encryption options are used.
Persistent settings use the `nvs` partition through Preferences, one
namespace per game, each with the key `best`: `snake`, `shooter`,
`blockfall`, `chameleon` (seconds survived), `penalty` (goals). Hide+Seek is
two-player and Beats has no score, so neither saves anything. The `blockfall` namespace is the one the old standalone project
used, so its best score carries over.
