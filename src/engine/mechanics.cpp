#include "../../include/engine/mechanics.h"
#include "../../include/engine/rack.h"
#include "../../include/heuristics.h"
#include "../../include/spectre/move_generator.h"
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

namespace Mechanics {
    void applyMove(GameState& state, const Move& move, int score) {
        int dr = move.horizontal ? 0 : 1;
        int dc = move.horizontal ? 1 : 0;
        int r = move.row;
        int c = move.col;

        TileRack& rack = state.players[state.currentPlayerIndex].rack;
        string word = move.word;

        for (char letter : word) {
            while (r < BOARD_SIZE && c < BOARD_SIZE && state.board[r][c] != ' ') {
                r += dr; c += dc;
            }

            state.board[r][c] = toupper(letter);
            state.blanks[r][c] = (letter >= 'a' && letter <= 'z');

            // Remove used tile
            for (auto it = rack.begin(); it != rack.end(); ++it) {
                bool match = (it->letter == '?') || (toupper(it->letter) == toupper(letter));
                if (match) {
                    rack.erase(it);
                    break;
                }
            }
            r += dr; c += dc;
        }

        state.players[state.currentPlayerIndex].score += score;
        state.players[state.currentPlayerIndex].passCount = 0; // Valid move resets pass count

        if (rack.size() < 7 && !state.bag.empty()) {
            drawTiles(state.bag, rack, static_cast<int>(7 - rack.size()));
        }
    }

    void commitSnapshot(GameState& backup, const GameState& current) {
        backup = current;
    }

    void restoreSnapshot(GameState& current, const GameState& backup) {
        current = backup;
    }

    bool attemptExchange(GameState& state, const Move& move) {
        Player& p = state.players[state.currentPlayerIndex];
        if (exchangeRack(p.rack, move.exchangeLetters, state.bag)) {
            // RULE: Exchanges increase the pass count (effectively using a turn)
            p.passCount++;
            return true;
        }
        return false;
    }

    void applySixPassPenalty(GameState& state) {
        // RULE: Value of tiles in each player get doubled and reduced from their own rack
        for (int i = 0; i < 2; i++) {
            int rackVal = 0;
            for (const auto &tile : state.players[i].rack) {
                rackVal += tile.points;
            }
            state.players[i].score -= (rackVal * 2);
        }
    }

    void applyEmptyRackBonus(GameState& state, int winnerIdx) {
        // RULE: Player who finishes first gets bonus (twice the value of remaining tiles of opponent)
        int loserIdx = 1 - winnerIdx;
        int loserRackVal = 0;
        for (const auto& tile : state.players[loserIdx].rack) {
            loserRackVal += tile.points;
        }

        state.players[winnerIdx].score += (loserRackVal * 2);
    }

    // Helper to calculate score for a specific move
    int calculateTrueScore(const spectre::MoveCandidate &move, const LetterBoard& letters, const Board &bonusBoard) {
    int totalScore = 0;
    int mainWordScore = 0;
    int mainWordMultiplier = 1;
    int tilesPlacedCount = 0;

    // OPTIMIZATION: Track length during iteration (Removes strlen)
    int wordLen = 0;

    int r = move.row;
    int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    for (char letter : move.word) {
        if (letter == '\0') break;
        wordLen++; // Count length here

        // Fast bounds check (unlikely to fail in valid generation, but keeps safety)
        if (static_cast<unsigned>(r) >= 15 || static_cast<unsigned>(c) >= 15) return -1000;

        // OPTIMIZATION: Fast ASCII conversion (A=65 -> 0)
        // We assume generator provides uppercase. ? is 63.
        int letterScore = 0;
        if (letter != '?') {
            int idx = (letter & 31) - 1;
            if (idx >= 0 && idx < 26) letterScore = spectre::Heuristics::TILE_VALUES[idx];
        }

        bool isNewlyPlaced = (letters[r][c] == ' ');

        if (isNewlyPlaced) {
            tilesPlacedCount++;
            CellType bonus = bonusBoard[r][c];

            // Branchless-ish bonus application
            if (bonus != CellType::Normal) {
                if (bonus == CellType::DLS) letterScore <<= 1;      // * 2
                else if (bonus == CellType::TLS) letterScore *= 3;  // * 3
                else if (bonus == CellType::DWS) mainWordMultiplier <<= 1;
                else if (bonus == CellType::TWS) mainWordMultiplier *= 3;
            }
        }

        mainWordScore += letterScore;

        if (isNewlyPlaced) {
            // Cross-word checks (Logic remains same, but heavily hit)
            // ... (keep your existing neighbor check logic here) ...

            // [Copying your neighbor logic for completeness context]
            int pdr = move.isHorizontal ? 1 : 0;
            int pdc = move.isHorizontal ? 0 : 1;
            bool hasNeighbour = false;

            // ... [Keep existing Cross-Word Logic] ...
            // Just ensure inside the cross-word loop you use the fast heuristic too:
            // int crossLetterScore = (letter != '?') ? spectre::Heuristics::TILE_VALUES[(letter & 31) - 1] : 0;

            // Simplified Check for "hasNeighbour":
            if ((r-pdr >= 0 && letters[r-pdr][c-pdc] != ' ') ||
                (r+pdr < 15 && letters[r+pdr][c+pdc] != ' ')) {

                int currR = r, currC = c;
                while (currR - pdr >= 0 && currC - pdc >= 0 && letters[currR-pdr][currC-pdc] != ' ') {
                    currR -= pdr; currC -= pdc;
                }

                int crossScore = 0;
                int crossMult = 1;

                while (currR < 15 && currC < 15) {
                    char cellLetter = letters[currR][currC];
                    int pts = 0;

                    if (currR == r && currC == c) {
                        pts = (letter != '?') ? spectre::Heuristics::TILE_VALUES[(letter & 31) - 1] : 0;
                        CellType crossBonus = bonusBoard[currR][currC];
                        if (crossBonus == CellType::DLS) pts <<= 1;
                        else if (crossBonus == CellType::TLS) pts *= 3;

                        if (crossBonus == CellType::DWS) crossMult <<= 1;
                        else if (crossBonus == CellType::TWS) crossMult *= 3;
                    } else if (cellLetter != ' ') {
                        pts = spectre::Heuristics::TILE_VALUES[(cellLetter & 31) - 1];
                    } else {
                        break;
                    }
                    crossScore += pts;
                    currR += pdr; currC += pdc;
                }
                totalScore += (crossScore * crossMult);
            }
        }
        r += dr; c += dc;
    }

    // OPTIMIZATION: No strlen call
    if (wordLen > 1) {
        totalScore += (mainWordScore * mainWordMultiplier);
    }
    if (tilesPlacedCount == 7) {
        totalScore += 50;
    }

    return totalScore;
}
}
