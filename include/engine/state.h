#pragma once

#include <vector>
#include <string>
#include "board.h"
#include "tiles.h"
#include "rack.h"
#include "../player_controller.h"
#include "../fast_constraints.h" // Include this!
#include "types.h"

struct GameState {
    LetterBoard board;
    BlankBoard blanks;
    TileBag bag;
    Player players[2];

    int currentPlayerIndex = 0;
    bool dictActive = true;

    // Helper to create a deep copy ( For AI simulation )
    GameState clone() const {
        return *this;
    }
};

struct LastMoveInfo {
    bool exists = false;
    int playerIndex = -1;

    Move move;
    int score = 0;
    bool emptiedRack = false; // flag for Endgame detection

    // Context for challenges (populated by Director)
    vector<string> formedWords;

    void reset() {
        exists = false;
        playerIndex = -1;
        score = 0;
        emptiedRack = false;
        formedWords.clear();
    }
};
