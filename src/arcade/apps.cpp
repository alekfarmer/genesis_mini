// The arcade's game list. To add a game: implement arcade::App in
// lib/<game>_app, include its header here, create one instance and add it to
// kGames. The menu picks it up automatically.

#include "apps.h"

#include "beat_app.h"
#include "blockfall_app.h"
#include "shooter_app.h"
#include "snake_app.h"

namespace {
SnakeApp g_snake;
ShooterApp g_shooter;
BeatApp g_beats;
BlockfallApp g_blockfall;
}  // namespace

arcade::App* const kGames[] = {
    &g_snake,
    &g_shooter,
    &g_beats,
    &g_blockfall,
};

const uint8_t kGameCount = sizeof(kGames) / sizeof(kGames[0]);
