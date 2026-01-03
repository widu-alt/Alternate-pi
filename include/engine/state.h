#pragma once

#include "types.h"

struct GameState {
    LetterBoard board;
    BlankBoard blanks;
    TileBag bag;
    Player players[2];

    int currentPlayerIndex = 0;
    bool dictActive = true;

    // Helper to create a deep copy ( For AI simulation )
    GameState close() const {
        return *this;
    }
};

// Snapshot of game state before last word move ( for Undo/Challenge )
struct GameSnapshot {
    LetterBoard letters;
    BlankBoard blanks;
    TileBag bag;
    Player players[2];
};

struct LastMoveInfo {
    bool exists = false;
    int playerIndex = -1;
    int startRow = 0;
    int startCol = 0;
    bool horizontal = false;
    std::string word;
};
