#pragma once

#include "../engine/types.h"
#include <string>
#include <vector>

using namespace std;

class Renderer {
public:

    static void printTitle();

    static void printBoard(const Board& bonusBoard, const LetterBoard& letters);

    static void printRack(const TileRack& rack);

    static void showUnseenTiles(const TileBag& bag, const Player players[2], int currentPlayer);

    static void clearScreen();
    static void waitForQuitKey();

};