# Standards for this repository

## What this project is

Firmware for an Axiometa Genesis Mini: an ESP32-S3 board with 4 MB flash,
2 MB PSRAM, Wi-Fi, Bluetooth LE, a NeoPixel, a STEMMA QT (I²C) connector and
AX22 module ports, programmed over USB-C. The code is written here, built
locally, and flashed to the board over USB.

These rules apply to every task. Read ARCHITECTURE.md before creating or
moving files, adding a library, or changing the build configuration, and keep
it true when you change the design.

## Boundaries (these override everything else)

- **Nothing is installed globally.** The whole toolchain (build tool,
  ESP32-S3 platform, compilers, frameworks, libraries, flashing tools) lives
  inside this repository:
  - PlatformIO is installed into a project-local Python virtual environment
    (`.venv/`).
  - `PLATFORMIO_CORE_DIR` points to `.pio-core/` inside the repo, so
    platforms, toolchains and packages never land in `~/.platformio`.
  - Libraries are declared in `platformio.ini` (`lib_deps`), pinned to exact
    versions, and fetched into the repo.
  - No Homebrew, no global pip/npm, no Arduino IDE, no USB drivers, no
    changes to shell profiles or system settings. The ESP32-S3's native USB
    needs no driver on macOS.
  If you believe something truly has to be installed outside the repo, stop,
  explain why, and wait for my explicit approval of that specific install.
- Every change on the host happens inside this repository. Nothing is
  written outside this directory tree.
- **The Makefile is the only way to build, test, flash, or talk to the
  board.** See "Running anything" below. Never call pio, esptool,
  arduino-cli, python, pip, curl, gh, or similar tools directly, even "just
  to check", and never tuck them inside a script or a longer command.
- Secrets (Wi-Fi credentials, API keys) live in a gitignored file (`.env` or
  `include/secrets.h`); commit an example version. Secrets never appear in
  logs or serial output.

## The board (hardware safety)

- **Flashing needs my go-ahead.** Build and test freely, but ask before every
  upload to the board, and say what is being flashed.
- **Never touch eFuses, secure boot, or flash encryption.** These changes
  are permanent and can lock the board for good. That means no
  `espefuse`, and no build options that enable them.
- Never erase the whole flash, or write to the bootloader or partition table,
  without asking first.
- The serial port is chosen explicitly (`PORT=...`), never guessed, and only
  the Genesis Mini's port is ever used.
- Pins are 3.3 V logic. Before wiring advice or pin changes, check the pin
  against the board's pinout and the AX22 module's documentation. Never
  suggest a wiring that puts 5 V on a GPIO.
- Pin assignments and module mappings live in one place (`include/pins.h`)
  so they are easy to check.
- I have the Genesis Mini Starter Kit. Its modules and the ports they are
  plugged into are listed in `docs/modules.md`. If a module or port isn't
  listed there, ask me. Never guess which module is in which port.

## Git

- A new repository starts with one commit on `main`, made by me. Branch
  from it.
- Work on your feature branch. Commit often, in small steps, each with a
  clear message. You may push it.
- `main` changes only through a pull request that I review and merge.
- Merging into any other branch needs my explicit approval, every time.
- Never force-push, rewrite published history, or change git configuration.
- `.venv/`, `.pio-core/`, `.pio/` and secrets are gitignored.

## Before you start

For any task that creates or changes more than one file, reply first with
your plan and the files you will touch, in under 40 lines, followed by your
assumptions and your questions. Wait for approval.

## Running anything

Through `make` targets, and nothing else.

- **I own the Makefile.** Do not create, edit, or delete it, or any script
  it calls, without my explicit approval of that specific change.
- Only run targets that exist. `make help` lists them. Never guess a target
  or work around a missing one.
- If a task needs a target that does not exist yet, or one that does not
  work, stop. Say what the target should do, propose its exact recipe, and
  wait for me to add or approve it. Until then, write the code and leave it
  unbuilt and unflashed, and say so.
- Never pass extra flags or variables to a target beyond what `make help`
  documents.

The targets I plan to add (not all exist yet):

- `setup` creates `.venv/` and the toolchain in `.pio-core/`.
- `build` compiles the firmware.
- `test` runs the native unit tests.
- `upload PORT=...` flashes the board; ask me first.
- `monitor PORT=...` opens the serial monitor.
- `clean` removes build output.
- `verify` is the gate.

## Quality

The code is C++ on the Arduino framework for ESP32 (via PlatformIO). Follow
current embedded C++ best practice, and say in the plan where you depart from
it. Most of the code will be written by an agent, so write it for me to read
and review: after each change, explain briefly what changed and why.

- Readable code, split into small modules by responsibility, with comments on
  anything public.
- No blocking `delay()` in the main loop when the code does more than one
  thing.
- Check every hardware call's return value, and log errors to serial instead
  of failing silently.
- Watch memory: no unbounded allocations or growing strings in the loop.
- Add a library only when it earns its place, and say why.

## Done means

- `make verify` is green from a fresh clone: it builds without warnings
  and the tests pass.
- Logic that does not need hardware (parsing, state machines, calculations)
  has native unit tests. Show each test failing once before you make it pass.
- Anything that needs the board is listed with the exact steps for me to
  check by hand.
- The README explains to a stranger how to set up, build, flash, and wire the
  modules used.
- Say what you did not verify, especially anything only tested off-device.
