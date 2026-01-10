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
        int wordLen = 0;

        int r = move.row;
        int c = move.col;
        int dr = move.isHorizontal ? 0 : 1;
        int dc = move.isHorizontal ? 1 : 0;

        for (char letter : move.word) {
            if (letter == '\0') break;
            wordLen++;

            if (r < 0 || r > 14 || c < 0 || c > 14) return -1000;

            int letterScore = spectre::Heuristics::getTileValue(letter);
            bool isNewlyPlaced = (letters[r][c] == ' ');

            if (isNewlyPlaced) {
                tilesPlacedCount++;
                CellType bonus = bonusBoard[r][c];

                if (bonus == CellType::DLS) letterScore *= 2;
                else if (bonus == CellType::TLS) letterScore *= 3;

                if (bonus == CellType::DWS) mainWordMultiplier *= 2;
                else if (bonus == CellType::TWS) mainWordMultiplier *= 3;
            }

            mainWordScore += letterScore;

            if (isNewlyPlaced) {
                int pdr = move.isHorizontal ? 1 : 0;
                int pdc = move.isHorizontal ? 0 : 1;
                bool hasNeighbour = false;

                int checkR1 = r - pdr;
                int checkC1 = c - pdc;
                if (checkR1 >= 0 && checkR1 < 15 && checkC1 >= 0 && checkC1 < 15 && letters[checkR1][checkC1] != ' ') hasNeighbour = true;

                int checkR2 = r + pdr;
                int checkC2 = c + pdc;
                if (checkR2 >= 0 && checkR2 < 15 && checkC2 >= 0 && checkC2 < 15 && letters[checkR2][checkC2] != ' ') hasNeighbour = true;

                if (hasNeighbour) {
                    int currR = r, currC = c;
                    while (currR - pdr >= 0 && currC - pdc >= 0 && letters[currR-pdr][currC-pdc] != ' ') {
                        currR -= pdr;
                        currC -= pdc;
                    }

                    int crossScore = 0;
                    int crossMult = 1;

                    while (currR < 15 && currC < 15) {
                        char cellLetter = letters[currR][currC];
                        if (currR == r && currC == c) {
                            int crossLetterScore = spectre::Heuristics::getTileValue(letter);
                            CellType crossBonus = bonusBoard[currR][currC];
                            if (crossBonus == CellType::DLS) crossLetterScore *= 2;
                            else if (crossBonus == CellType::TLS) crossLetterScore *= 3;
                            if (crossBonus == CellType::DWS) crossMult *= 2;
                            else if (crossBonus == CellType::TWS) crossMult *= 3;
                            crossScore += crossLetterScore;
                        } else if (cellLetter != ' ') {
                            crossScore += spectre::Heuristics::getTileValue(cellLetter);
                        } else {
                            break;
                        }
                        currR += pdr;
                        currC += pdc;
                    }
                    totalScore += (crossScore * crossMult);
                }
            }
            r += dr;
            c += dc;
        }

        if (wordLen > 1) {
            totalScore += (mainWordScore * mainWordMultiplier);
        }
        if (tilesPlacedCount == 7) {
            totalScore += 50;
        }

        return totalScore;
    }
}
