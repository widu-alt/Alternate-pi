#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../include/board.h"
#include "../include/tiles.h"
#include "../include/move.h"
#include "engine/state.h"

using namespace std;

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