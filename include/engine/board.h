#pragma once

#include <array>
#include <string>
#include "types.h"

using namespace std;

//initialize all letters to ' '
void clearLetterBoard(LetterBoard &letters);

//initialize all blank flags to false
void clearBlankBoard(BlankBoard &blanks);

// Place a word on the board (0-based row/col
// horizontal = true -> left to right
// horizontal = false -> top to bottom
// Returns a false if the word doesn't fit or conflicts with existing letters.
bool placeWordOnBoard(LetterBoard &letters, int row, int col, bool horizontal, const string &word);

// Create and return a board with all Scrabble bonus squares set.
Board createBoard();

// Helpers for Challenges/Validation
string extractMainWord(const LetterBoard &letters,
                       int row,
                       int col,
                       bool horizontal);

vector<std::string> crossWordList(const LetterBoard &letters,
                                  const LetterBoard &oldLetters,
                                  int row,
                                  int col,
                                  bool mainHorizontal);