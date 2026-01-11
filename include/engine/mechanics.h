#pragma once
#include "state.h"
#include "types.h"
#include "../heuristics.h"
#include "../spectre/move_generator.h"
#include "../move.h"

namespace Mechanics {

    // --- TEMPLATED SCORER (Inlined for Speed) ---
    // Generates optimized assembly for Horizontal vs Vertical
    template <bool IS_HORIZONTAL>
    static inline int calculateTrueScoreFast(const spectre::MoveCandidate &move,
                                             const LetterBoard& letters,
                                             const Board &bonusBoard)
    {
        int totalScore = 0;
        int mainWordScore = 0;
        int mainWordMultiplier = 1;
        int tilesPlacedCount = 0;
        int wordLen = 0;

        int r = move.row;
        int c = move.col;

        // COMPILE-TIME CONSTANTS (Compiler optimizes these away)
        constexpr int dr = IS_HORIZONTAL ? 0 : 1;
        constexpr int dc = IS_HORIZONTAL ? 1 : 0;
        constexpr int pdr = IS_HORIZONTAL ? 1 : 0; // Perpendicular dr
        constexpr int pdc = IS_HORIZONTAL ? 0 : 1; // Perpendicular dc

        for (char letter : move.word) {
            if (letter == '\0') break;
            wordLen++;

            // UNSAFE: Direct Array Access (No safety checks)
            // Assumes generator provides valid 'A'-'Z' or '?'
            int letterScore = 0;
            if (letter != '?') {
                 letterScore = spectre::Heuristics::TILE_VALUES[(letter & 31) - 1];
            }

            // Fast Check: Is square empty?
            if (letters[r][c] == ' ') {
                tilesPlacedCount++;
                CellType bonus = bonusBoard[r][c];

                if (bonus != CellType::Normal) {
                    if (bonus == CellType::DLS) letterScore <<= 1;
                    else if (bonus == CellType::TLS) letterScore *= 3;
                    else if (bonus == CellType::DWS) mainWordMultiplier <<= 1;
                    else if (bonus == CellType::TWS) mainWordMultiplier *= 3;
                }

                // --- CROSS WORD CHECK ---
                // Check immediate neighbors in perpendicular direction
                bool hasNeighbor = false;
                if (r-pdr >= 0 && letters[r-pdr][c-pdc] != ' ') hasNeighbor = true;
                else if (r+pdr < 15 && letters[r+pdr][c+pdc] != ' ') hasNeighbor = true;

                if (hasNeighbor) {
                    // Backtrack to start of cross-word
                    int currR = r; int currC = c;
                    while (currR - pdr >= 0 && currC - pdc >= 0 && letters[currR-pdr][currC-pdc] != ' ') {
                        currR -= pdr; currC -= pdc;
                    }

                    int crossScore = 0;
                    int crossMult = 1;

                    // Forward Scan
                    while (currR < 15 && currC < 15) {
                        char cellLetter = letters[currR][currC];

                        // Stop if empty AND not the tile we are placing
                        if (cellLetter == ' ' && (currR != r || currC != c)) break;

                        int pts = 0;
                        if (currR == r && currC == c) {
                            // The tile we are placing
                             pts = (letter != '?') ? spectre::Heuristics::TILE_VALUES[(letter & 31) - 1] : 0;

                             CellType crossBonus = bonusBoard[currR][currC];
                             if (crossBonus == CellType::DLS) pts <<= 1;
                             else if (crossBonus == CellType::TLS) pts *= 3;

                             if (crossBonus == CellType::DWS) crossMult <<= 1;
                             else if (crossBonus == CellType::TWS) crossMult *= 3;
                        } else {
                            // Existing tile
                            pts = spectre::Heuristics::TILE_VALUES[(cellLetter & 31) - 1];
                        }

                        crossScore += pts;
                        currR += pdr; currC += pdc;
                    }
                    totalScore += (crossScore * crossMult);
                }
            }

            mainWordScore += letterScore;
            r += dr; c += dc;
        }

        if (wordLen > 1) totalScore += (mainWordScore * mainWordMultiplier);
        if (tilesPlacedCount == 7) totalScore += 50;

        return totalScore;
    }

    // Applies a validated move to the state (Updates Board, Rack, Score, Bag)
    void applyMove(GameState& state, const Move& move, int score);

    // Saves the current state into a backup
    void commitSnapshot(GameState& backup, const GameState& current);

    // Restores the game state from a backup (Used in challenges)
    void restoreSnapshot(GameState& current, const GameState& backup);

    // Attempts an exchange (Updates Rack, Bag, PassCount) - Returns true if successful
    bool attemptExchange(GameState& state, const Move& move);

    // Value of tiles in each player get doubled and reduced from their own rack
    void applySixPassPenalty(GameState& state);

    // Player who finishes first gets bonus (twice the value of remaining tiles of opponent)
    void applyEmptyRackBonus(GameState& state, int winnerIdx);

    int calculateTrueScore(const spectre::MoveCandidate &move, const LetterBoard& letters, const Board &bonusBoard);
}