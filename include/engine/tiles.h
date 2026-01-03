#pragma once

#include <vector>
#include "types.h"

using namespace std;

// Create the standard 100-tile english scrabble bag
TileBag createStandardTileBag();

// Print the tile bag
void printTileBag(const TileBag &bag, const vector<Tile> &opponentRack, bool revealOpponentRack);

// Shuffle the tile bag
void shuffleTileBag(TileBag &bag);




