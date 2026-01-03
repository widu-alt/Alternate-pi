#pragma once

#include <vector>
#include <array>
#include <string>

using namespace std;

// Board Constants
constexpr int BOARD_SIZE = 15; //15 x 15 scrabble board
constexpr int CELL_INNER_WIDTH = 5; //Characters inside each cell

enum class CellType {
    Normal,
    DLS, //Double Letter Score
    TLS, //Triple Letter Score
    DWS, //Double Word Score
    TWS, //Triple Word Score
};

enum class MoveType {
    PLAY, // Placing a word
    PASS, // Passing a turn
    EXCHANGE, // Swap tiles with bag
    CHALLENGE, // Challenge previous move
    QUIT, // Resign
    NONE // (Internal Use)
};

// Represents a single scrabble Tile
struct Tile{
    char letter; //"A to Z" or "?" for blank
    int points; //score value
};

// A rack is the 7 tiles a player holds
using TileRack = vector<Tile>;

// Tile bag
using TileBag = vector<Tile>;

// 2D board type
using Board = array < array < CellType, BOARD_SIZE >, BOARD_SIZE>;

//Holds the actual letters placed on the board
using LetterBoard = array < array < char, BOARD_SIZE >, BOARD_SIZE >;

// Track whether a letter on the board came from a blank tile.
// true = this square is a blank (score 0)
// false = normal tile
using BlankBoard = array < array < bool, BOARD_SIZE >, BOARD_SIZE >;

// Game can have 2-4 players
struct Player {
    TileRack rack;
    int score = 0;
    int passCount = 0;
};

// To hold the move data
struct Move {
    MoveType type = MoveType::NONE;

    // For playing
    string word; // Playing word/exchanging tiles
    int row = -1;
    int col = -1;
    bool horizontal = true;

    // For exchange
    string exchangeLetters;

    // [CRITICAL FIX] Added tiles vector to match AIPlayer usage
    std::vector<Tile> tiles;

    static Move Pass() { return {MoveType::PASS}; }
    static Move Quit() { return {MoveType::QUIT}; }
    static Move Challenge() { return {MoveType::CHALLENGE}; }
    static Move Play(int r, int c, bool h, string w) {
        return {MoveType::PLAY, w, r, c, h};
    }
    static Move Exchange(string tiles) {
        return {MoveType::EXCHANGE, " ", -1, -1, true, tiles};
    }
};

// Result of trying to play a word
struct MoveResult {
    bool success;
    int score;
    string message; // Error message or Success
    vector<string> wordsFormed; // Main + Cross words
};