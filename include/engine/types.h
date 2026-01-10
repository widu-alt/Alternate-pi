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

// =========================================================
// OPTIMIZATION: SBO (Small Buffer Optimization) Move Struct
// =========================================================
// By removing std::string, this struct becomes POD (Plain Old Data).
// It can be copied via memcpy and lives entirely on the stack.
// No malloc/free overhead during simulations.
struct Move {
    MoveType type = MoveType::NONE;

    // SBO: Max Scrabble word is 15 chars + null terminator
    char word[16];

    int row = -1;
    int col = -1;
    bool horizontal = true;

    // SBO: Max exchange is 7 tiles + null terminator
    char exchangeLetters[8];

    // Note: vector<Tile> still allocates, but is rarely used in hot paths
    // compared to 'word'.
    std::vector<Tile> tiles;

    // Default Constructor
    Move() {
        type = MoveType::NONE;
        word[0] = '\0';
        exchangeLetters[0] = '\0';
        row = -1; col = -1;
    }

    Move(MoveType t) {
        type = t;
        word[0] = '\0';
        exchangeLetters[0] = '\0';
        row = -1; col = -1;
        horizontal = true;
    }

    // Static Builders
    static Move Pass() {
        Move m;
        m.type = MoveType::PASS;
        return m;
    }

    static Move Quit() {
        Move m;
        m.type = MoveType::QUIT;
        return m;
    }

    static Move Challenge() {
        Move m;
        m.type = MoveType::CHALLENGE;
        return m;
    }

    static Move Play(int r, int c, bool h, const string& w) {
        Move m;
        m.type = MoveType::PLAY;
        m.row = r;
        m.col = c;
        m.horizontal = h;

        // Fast Copy (avoiding string allocation)
        size_t len = w.length();
        if (len > 15) len = 15;
        memcpy(m.word, w.c_str(), len);
        m.word[len] = '\0'; // Null terminate

        return m;
    }

    static Move Exchange(const string& letters) {
        Move m;
        m.type = MoveType::EXCHANGE;
        m.horizontal = true; // Convention
        m.word[0] = ' '; m.word[1] = '\0'; // Space for renderer compatibility

        size_t len = letters.length();
        if (len > 7) len = 7;
        memcpy(m.exchangeLetters, letters.c_str(), len);
        m.exchangeLetters[len] = '\0';

        return m;
    }
};

// Result of trying to play a word
struct MoveResult {
    bool success;
    int score;
    string message; // Error message or Success
    vector<string> wordsFormed; // Main + Cross words
};