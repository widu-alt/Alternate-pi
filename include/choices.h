#pragma once

#include "../include/move.h"
#include "engine/board.h"
#include "engine/tiles.h"
#include "../include/engine/rack.h"
#include "engine/dictionary.h"
#include "../include/player_controller.h"
#include "../include/engine/referee.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// ================================
//         UI HELPERS
// ================================

// Show tile set from current player's perspective
// Unseen tiles are bag + opponent rack.
// Reveals opponent rack only when bag <= 7 (Tile tracking rule)
void showTileSet(const TileBag &bag, const Player players[2], int currentPlayer);

// Note: All Game Logic (Play, Exchange, Pass, Challenge, EndGame)
// has been moved to src/engine/game_director.cpp




















