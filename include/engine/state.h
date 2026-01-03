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