#pragma once

#include "../include/move.h"
#include "../include/board.h"
#include "../include/tiles.h"
#include "../include/rack.h"
#include "engine/dictionary.h"
#include "../include/player_controller.h"
#include "../include/engine/referee.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// ================================
//         GAME ACTION
// ================================

bool executePlayMove(GameState& state,
                     const Move &move,
                     const Board &bonusBoard);

bool executeExchangeMove(GameState& state, const Move &move);

void passTurn(GameState& state, bool &canChallenge, LastMoveInfo &lastMove);

// Show tile set from current player's prespective
// unseen tiles are bag + opponent rack, reveal opponenet rack when bag <= 7 (Tile tracking)
void showTileSet(const TileBag &bag, const Player players[2], int currentPlayer);

// Handle a CHALLENGE command;
// Uses lastSnapshot to undo the last move on successful challenge.
// Does NOT change current player.
void challengeMove(GameState& state,
                   const Board &bonusBoard,
                   const GameState &lastSnapshot,
                   LastMoveInfo &lastMove,
                   bool &canChallenge);

// ================================
//         END GAME RULES
// ================================

// Returns true if game ended and scores are printed.
bool handleSixPassEndGame(GameState& state);

bool handleEmptyRackEndGame(GameState& state,
                            const Board &bonusBoard,
                            GameState &lastSnapshot,
                            LastMoveInfo &lastMove,
                            bool &canChallenge,
                            PlayerController* controller);

// Handle resignation
bool handleQuit(GameState& state);




















