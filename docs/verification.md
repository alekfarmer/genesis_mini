# Still to verify on the board

What has been checked on real hardware so far, and what hasn't. Tick items
off as you go. Where the README has detailed steps, the item points there.

Before checking a project, flash it:
`make upload PROJECT=<name> PORT=/dev/cu.usbmodem101`.
To capture serial output for later reading:
`make log PORT=/dev/cu.usbmodem101 SECONDS=120`.

## blockfall

Flashed 2026-11-24. Start-up log clean (screen and flash storage came up).
You reported that it mostly works. Note anything that doesn't here.

- [ ] Anything that didn't work: _______________________
- [ ] **Orientation:** with the screen tall, the picture is the right way up
      (else `kPortrait` → `kPortraitFlipped` in
      `lib/blockfall_screen/blockfall_screen.cpp`).
- [ ] **Layout:** score and `LV 1` top left, next piece top right, well below
      with its border; nothing cut off at any edge.
- [ ] **Knob, turn:** clockwise moves right, one column per click; stops at
      both walls.
- [ ] **Knob, press:** rotates clockwise; next to a wall the piece shifts over
      instead of refusing.
- [ ] **Tap vs hold:** a quick tap of the on-board button drops instantly; a
      hold falls fast until released. The 220 ms split feels right
      (`kHoldMs` in `src/blockfall/main.cpp`).
- [ ] **Speed:** level 1 (a step every 0.8 s) isn't too fast or slow; after 10
      lines `LV 2` appears and pieces fall noticeably faster.
- [ ] **Responsiveness:** moving quickly while pieces fall never loses knob
      clicks or stutters.
- [ ] **Sounds:** a blip on landing; a two-note figure for 1-3 lines; a
      longer four-note figure for 4 lines at once; no crackle while the
      screen redraws.
- [ ] **NeoPixel:** green heartbeat while playing, white flash on a line
      clear, red at game over, green again on a new game.
- [ ] **Game over:** panel shows `GAME OVER`, `BEST` and `PRESS KNOB`;
      either button starts a new game (presses in the first 0.6 s are
      ignored).
- [ ] **Best score persists:** finish a game with a score, unplug, replug,
      lose quickly: the panel still shows the earlier best. Serial says
      `(new best)` only when it was beaten.
- [ ] **Serial log:** `make log` during play shows `cleared N line(s)` and
      `game over` lines with sensible scores, and no `ERROR` lines.

## kit_demo

Most of it was checked on 2026-11-24 (screen, sensor, buzzer, silence). It
has changed since then, so re-flash it first.

- [ ] **Screen after the shared-panel refactor:** looks exactly as before
      (landscape, colours right, nothing cut off). The version checked
      earlier predates `lib/panel`.
- [ ] **Knob direction after the fix:** clockwise now *raises* `SET`.
- [ ] **No buzzer errors during an alarm:** trigger the alarm while running
      `make log ... SECONDS=20`; no `noTone(): Tone is not running` lines.
      (Only start-up was checked after the fix.)
- [ ] **Sensor unplugged** (README step 7): `NO SENSR` within 2 s and a
      `[dht] ERROR` line; plugging it back recovers. Never checked.

## hello

- [x] Fully checked on 2026-11-24: boot report, all checks passed, green
      heartbeat, report repeats when a monitor connects.

## Backup and restore

- [x] Factory demo backed up and checked against the board:
      `backups/genesis_mini-20261124-170119.bin`, SHA-256
      `4f94de38843078963c276095c0d5eb4f0251bba30069598bee0a4d779f604fba`.
- [ ] **Copy the backup off this Mac** (cloud storage or another drive). It's
      gitignored, so it exists only here.
- [ ] **Restore works** (optional, but it is the only proof the backup is
      usable): `make restore PORT=/dev/cu.usbmodem101
      BACKUP=backups/genesis_mini-20261124-170119.bin`, check the factory
      demo runs, then re-flash a project. This overwrites the whole flash,
      including the bootloader, partition table and saved settings
      (blockfall's best score is lost).

## Repository

- [ ] **`make verify` from a fresh clone** has not been run: everything so
      far was built in this working copy. Clone the repo elsewhere, then run
      `make setup` (downloads about 2.3 GB) and `make verify`.
- [ ] **Merge to `main`:** all work is on the `scaffold` branch, and nothing
      has reached `main` yet. Open a pull request when you're happy.
