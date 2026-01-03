#include <iostream>

#include "../include/board.h"
#include "../include/interface/renderer.h"

using namespace std;

// Building the board and setting up all the bonus squares
Board createBoard() {
    Board board{};

    //filling everything with normal cells.
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            board[row][col] = CellType::Normal;
        }
    }

    // --------------------------------------------------------------
    // Official Scrabble Bonus Layout
    // (Row = A–O → 0–14, Col = 1–15 → 0–14)
    // --------------------------------------------------------------

    // --- Triple Word Score (TWS) ---
    int tws[][2] = {
        {0,0},{0,7},{0,14},
        {7,0},{7,14},
        {14,0},{14,7},{14,14}
    };

    for (auto &p : tws) board[p[0]][p[1]] = CellType::TWS;

    //C++ learning note
    /*
     * in "for(auto &p : tws)" the p becomes a reference to each element in the list.
     * each element looks like {row, column}
     * :tws means for each element inside tws
     * so p is effectively, first iteration p = {0,0}, second iteration p = {0,7} etc...
     */

    // --- Double Word Score (DWS) ---
    // (Note: H7 was already set by you, but including it does no harm)
    int dws[][2] = {
        {1,1},{2,2},{3,3},{4,4},{7,7},
        {10,10},{11,11},{12,12},{13,13},
        {1,13},{2,12},{3,11},{4,10},
        {10,4},{11,3},{12,2},{13,1}
    };
    for (auto &p : dws) board[p[0]][p[1]] = CellType::DWS;

    // --- Triple Letter Score (TLS) ---
    int tls[][2] = {
        {1,5},{1,9},
        {5,1},{5,5},{5,9},{5,13},
        {9,1},{9,5},{9,9},{9,13},
        {13,5},{13,9}
    };
    for (auto &p : tls) board[p[0]][p[1]] = CellType::TLS;

    // --- Double Letter Score (DLS) ---
    int dls[][2] = {
        {0,3},{0,11},
        {2,6},{2,8},
        {3,0},{3,7},{3,14},
        {6,2},{6,6},{6,8},{6,12},
        {7,3},{7,11},
        {8,2},{8,6},{8,8},{8,12},
        {11,0},{11,7},{11,14},
        {12,6},{12,8},
        {14,3},{14,11}
    };
    for (auto &p : dls) board[p[0]][p[1]] = CellType::DLS;

    return board;
}

// clearing the letter board
void clearLetterBoard(LetterBoard &letters) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            letters[row][col] = ' '; //empty
        }
    }
}

// clearing the blank board
void clearBlankBoard(BlankBoard &blanks) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            blanks[row][col] = false;
        }
    }
}

// placing the word on letter board
bool placeWordOnBoard(LetterBoard &letters, int row, int col, bool horizontal, const string &word) {
    int dr = horizontal ? 0 : 1; //goes row by row if its vertical (row step)
    int dc = horizontal ? 1 : 0; //goes column by column if its horizontal (column step)

    // 1) bounds check
    int r = row;
    int c = col;

    // C++ learning
    // for (char ch: word) means loop through each character inside the string word
    for (char ch: word) {
        if (r<0 || r >= BOARD_SIZE || c<0 || c >= BOARD_SIZE) {
            return false;
        }
        r += dr;
        c += dc;
    }

    // 2) Checking for conflicts
    r = row;
    c = col;

    for (char ch: word) {

        // C++ learning
        // std::toupper converts letter strings to uppercase

        char upper = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        char existing = letters[r][c];

        if (existing != ' ' && existing != upper) {
            return false; // trying to overwrite a different letter
        }
        r += dr;
        c += dc;
    }

    // 3) Actually place the letters
    r = row;
    c = col;
    for (char ch: word) {
        char upper = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        letters[r][c] = upper;
        r += dr;
        c += dc;
    }

    return true;
}