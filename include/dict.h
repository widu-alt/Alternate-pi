#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../include/board.h"
#include "../include/tiles.h"
#include "../include/move.h"

using namespace std;

// Snapshot of game state before last word move
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
};

/* for detecting the EXE directory
string getExecutableDirectory();
*/

// Load the dictionary.
bool loadDictionary(const string &filename);

// Check if the word is available in the dictionary.
bool isValidWord(const string &word);

// returns the list of crosswords formed from main word, to be judged.
vector<string> crossWordList(const LetterBoard &letters, const LetterBoard &oldLetters,
                             int row, int col, bool mainHorizontal);

// Get the full word from the board (for challenging).
string extractMainWord(const LetterBoard &letters, int row, int col, bool horizontal);