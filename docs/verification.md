# Still to verify on the board

What has been checked on real hardware so far, and what hasn't. Tick items
off as you go. Where the README has detailed steps, the item points there.

Before checking a project, flash it:
`make upload PROJECT=<name> PORT=/dev/cu.usbmodem101`.
To capture serial output for later reading:
`make log PORT=/dev/cu.usbmodem101 SECONDS=120`.

## arcade

Built 2026-11-25; Hide+Seek, Chameleon, Penalty and the module config added
2026-11-26. It has **never been flashed**. It builds warning-free and
all its game logic passes the host tests, but nothing below has been seen on
the board. Flash it with `make upload PROJECT=arcade PORT=...`. The README's
"Checking the arcade" section has the full steps.

- [ ] **Boots to the menu:** six games listed (Snake, Shooter, Beats,
      Blockfall, Hide+Seek, Penalty), scrolling past five; Blockfall tagged
      `[TALL]`; highlight wraps around. Start-up log shows
      `[arcade] starting, 6 games`, one `port N:` line per port matching
      `board_config.h`, and no `ERROR`.
- [ ] **Config filtering:** flash `arcade_led_button` with the LED button in
      port 3: Chameleon appears, 7 games. Then set port 2 to `MOD_NONE` in
      `board_config.h`, rebuild and flash `arcade`: Beats is gone and the
      other games run silently. (Put the buzzer back afterwards.)
- [ ] **Back gesture:** holding the knob 1.5 s returns to the menu from every
      game; after Blockfall the screen is landscape again.
- [ ] **Switching doesn't leak memory:** `make log` while switching games
      several times shows a steady `free heap` in the `[arcade] ->` lines.
      The first Blockfall visit drops it by about 25 KB, once.

### Snake
- [ ] Starts moving right; clockwise turns right; quick double clicks make a
      U-turn over two steps.
- [ ] Food makes it grow, beep and speed up; walls and its own body end the
      game.
- [ ] A tap pauses (`PAUSED` banner) and resumes.
- [ ] Speed: 180 ms per step at the start feels right.

### Shooter
- [ ] Asteroids drift in from the right; the background stars scroll.
- [ ] Clockwise moves the ship up (or does down feel more natural? The sign
      is in `ShooterApp::update`: `moveShip(in.knob_steps)`).
- [ ] A press fires one shot; holding the on-board button fires continuously.
- [ ] Hitting a rock: it vanishes, a low thud plays, the score rises
      (smaller rocks score more).
- [ ] Being hit: a life icon disappears, the NeoPixel flashes red, the ship
      blinks for a second, and other rocks pass through meanwhile.
- [ ] Difficulty: rocks get more frequent and faster as the score rises, but
      it doesn't feel unfair early on.

### Beats
- [ ] Starter beat loaded, `STOPPED` shown; a tap starts it, and the white
      playhead moves in time.
- [ ] The four sounds are distinguishable on the buzzer (kick low, snare
      rough, hi-hat high click, bleep a note). This is the most uncertain part:
      a single buzzer can only approximate drums.
- [ ] Turning moves the amber cursor row by row; a press toggles a step and
      previews its sound when stopped.
- [ ] Holding the button and turning changes the tempo between 60 and 200 BPM,
      without moving the cursor (after the first ~0.2 s of the hold).
- [ ] Stopping silences it at once. Leaving and coming back keeps the
      pattern.

### Hide+Seek (two players)
- [ ] 12 spots drawn and recognisable (trees, bushes, rocks, shed, logs,
      well); the cursor sweeps left to right.
- [ ] The "take the board / look away" screens appear between turns, and the
      hiding spot isn't given away (the cursor starts at the left for the
      seeker).
- [ ] Misses: X on the spot, HOT/WARM/COLD on the status line with a
      matching beep and NeoPixel colour. Do the hints feel fair (hot under
      30 px, warm under 60 px)?
- [ ] After 4 rounds the totals and the right winner show; a press starts a
      new match.

### Chameleon (needs the LED button: flash `arcade_led_button`)
- [ ] The LED button in port 3 works: its light is on exactly when the HUD
      says `PRESS TO HIDE`, and a press camouflages you.
- [ ] Camouflaged: you take the object's colour, the NeoPixel shows it, and
      a seeker looking straight at you doesn't catch you. It wears off after
      3 s, or sooner if you walk away. Then `RECHARGE` counts down.
- [ ] Seekers' cone lines match what actually catches you: in front within
      ~34 px, or touching from any side.
- [ ] Clockwise turns you clockwise; a knob press stops and starts you.
- [ ] Difficulty: a seeker joins every 20 s. Is it survivable for a while?

### Penalty
- [ ] Goal, net, goalie and ball look right; the aim cross moves with the
      knob, clockwise = right.
- [ ] The power meter swings up and down; power past the red line goes over
      the bar, and very hard shots drift.
- [ ] The goalie dives (sometimes the right way) and saves some shots, but
      round 1 isn't too hard.
- [ ] Shot dots fill green/red; after 5 shots the round panel says THROUGH!
      or NOT ENOUGH GOALS; game over saves the best.

### Blockfall (inside the arcade)
- [ ] Anything that didn't work in the standalone version you tried
      earlier: _______________________
- [ ] **Orientation:** with the screen tall, the picture is the right way up
      (else `kPortrait` → `kPortraitFlipped` in `BlockfallApp::orientation()`,
      `lib/blockfall_app/blockfall_app.h`).
- [ ] **Layout:** score and `LV 1` top left, next piece top right, well below
      with its border; nothing cut off at any edge.
- [ ] **Knob, turn:** clockwise moves right, one column per click; stops at
      both walls.
- [ ] **Knob, press:** rotates clockwise; next to a wall the piece shifts over
      instead of refusing.
- [ ] **Tap vs hold:** a quick tap of the on-board button drops instantly; a
      hold falls fast until released. The 220 ms split feels right
      (`kButtonHoldMs` in `src/arcade/main.cpp`, shared by every game).
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

## probe (module detection experiment)

Never flashed. It reads pins only; it never drives them.

- [ ] Flash `probe` and run `make log PORT=... SECONDS=10` with the Starter
      Kit plugged in as configured. Save the table.
- [ ] Unplug each module in turn and log again (the screen may go blank:
      that's its reset pin being read). Save each table.
- [ ] Bring me the logs: they decide which modules boot-time detection can
      warn about.

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
