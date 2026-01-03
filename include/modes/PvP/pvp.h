#pragma once

#include "../../engine/state.h"

// runs the full PvP game loop
void runPvP();

// NEW: Helper to save the current state into a snapshot
void takeSnapshot(GameSnapshot &lastSnapShot,
                  const LetterBoard &letters,
                  const BlankBoard &blanks,
                  const Player players[2],
                  const TileBag &bag);