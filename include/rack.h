#pragma once

#include <vector>
#include <string>
#include "../include/tiles.h"
#include "engine/types.h"

using namespace std;

// Draw up to 'count' tiles from the bag into the rack
// returns how many tiles were actually drawn
int drawTiles(TileBag &bag, TileRack &rack, int count);

//Print the rack like 1:A(1) 2:T(1) 3:Q(10)
void printRack(const TileRack &rack);

//Swap two tiles in the rack (based on player view)
bool handleSwapCommand(TileRack &rack, int index1, int index2);

// Parse a player command for the rack.
// - "4-7" → swap tile 4 and 7
// - "0"   → shuffle rack
// Returns true if command was valid and applied.
bool applyRackCommand(TileBag &bag, TileRack &rack, const string &command);

//Shuffle the rack play order randomly
void shuffleRack(TileRack &rack);

// Parse a player command for the rack.
// - "4-7" → swap tile 4 and 7
// - "0"   → shuffle rack
// Returns true if command was valid and applied.
bool exchangeRack(TileRack &rack, const string& lettersStr, TileBag &bag);

