#include "../../include/engine/mechanics.h"
#include "../../include/engine/rack.h"
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
}
