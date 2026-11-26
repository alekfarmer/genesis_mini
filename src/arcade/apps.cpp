// The arcade's game list, filtered by the modules in include/board_config.h.
//
// To add a game: implement arcade::App in lib/<game>_app, then add a block
// below guarded by the modules it can't run without. Games that are left out
// are not in the menu and their code is not linked into the firmware.
//
// Every game gets the screen and the knob (the arcade itself needs them).
// Sound is optional everywhere: without a buzzer, games run silently.
//
//   Game        Needs beyond screen + knob
//   Snake       -
//   Shooter     -
//   Beats       buzzer (it is a sound toy)
//   Blockfall   -
//   Hide+Seek   -
//   Chameleon   Tactile LED Button (AX22-0050)
//   Penalty     -

#include "apps.h"

#include "pins.h"

#if HAS_TFT && HAS_ENCODER

#include "blockfall_app.h"
#include "hideseek_app.h"
#include "penalty_app.h"
#include "shooter_app.h"
#include "snake_app.h"
#if HAS_BUZZER
#include "beat_app.h"
#endif
#if HAS_LED_BUTTON
#include "chameleon_app.h"
#endif

namespace {
SnakeApp g_snake;
ShooterApp g_shooter;
#if HAS_BUZZER
BeatApp g_beats;
#endif
BlockfallApp g_blockfall;
HideSeekApp g_hideseek;
#if HAS_LED_BUTTON
ChameleonApp g_chameleon;
#endif
PenaltyApp g_penalty;
}  // namespace

arcade::App* const kGames[] = {
    &g_snake,
    &g_shooter,
#if HAS_BUZZER
    &g_beats,
#endif
    &g_blockfall,
    &g_hideseek,
#if HAS_LED_BUTTON
    &g_chameleon,
#endif
    &g_penalty,
};

const uint8_t kGameCount = sizeof(kGames) / sizeof(kGames[0]);

#endif
