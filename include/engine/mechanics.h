#pragma once
#include "state.h"
#include "types.h"
#include "../move.h"

namespace Mechanics {

    // Applies a validated move to the state (Updates Board, Rack, Score, Bag)
    void applyMove(GameState& state, const Move& move, int score);

    // Saves the current state into a backup
    void commitSnapshot(GameState& backup, const GameState& current);

    // Restores the game state from a backup (Used in challenges)
    void restoreSnapshot(GameState& current, const GameState& backup);

    // Attempts an exchange (Updates Rack, Bag, PassCount) - Returns true if successful
    bool attemptExchange(GameState& state, const Move& move);

    // Value of tiles in each player get doubled and reduced from their own rack
    void applySixPassPenalty(GameState& state);

    // Player who finishes first gets bonus (twice the value of remaining tiles of opponent)
    void applyEmptyRackBonus(GameState& state, int winnerIdx);
}