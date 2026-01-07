#include <iostream>
#include <limits>
#include <string>

#include "../include/engine/board.h"
#include "../include/move.h"
#include "../include/engine/tiles.h"
#include "../include/engine/dictionary.h"
#include "../include/engine/mechanics.h"
#include "../include/choices.h"

using namespace std;

// Helper to print the tile distribution (Bag + Opponent Rack)
// This is used by the 't' command in HumanPlayer
void showTileSet(const TileBag &bag, const Player players[2], int currentPlayer) {
    int opponent = 1 - currentPlayer;
    bool revealOpponent = (static_cast<int>(bag.size()) <= 7);

    // We reuse the existing printTileBag helper which should be available
    // via tiles.h or we implement the logic here if it was legacy.
    // Assuming printTileBag(bag, rack, reveal) exists in tiles.cpp/h
    printTileBag(bag, players[opponent].rack, revealOpponent);
}