#include "../../include/spectre/vanguard.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h"
#include "../../include/spectre/logger.h"
#include "../../include/spectre/treasurer.h"
#include "../../include/heuristics.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace spectre {

// --- HELPER: Danger Check ---
// Returns TRUE if the move places a tile adjacent to an empty TWS
bool leavesTWSOpen(const MoveCandidate& move, const LetterBoard& board) {
    int r = move.row;
    int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    // Iterate through the word being placed
    for (int i=0; move.word[i] != '\0'; i++) {
        // Check 4 neighbors of the current tile
        int nr[] = {r+1, r-1, r, r};
        int nc[] = {c, c, c+1, c-1};

        for(int k=0; k<4; k++) {
            int checkR = nr[k];
            int checkC = nc[k];

            // Bounds check
            if (checkR >= 0 && checkR < 15 && checkC >= 0 && checkC < 15) {
                // Is this neighbor a TWS? (0,0), (0,7), (0,14), (7,0), (7,14), (14,0), (14,7), (14,14)
                bool isTWS = ((checkR==0 || checkR==7 || checkR==14) && (checkC==0 || checkC==7 || checkC==14) && !(checkR==7 && checkC==7));

                // If it IS a TWS and it is EMPTY (and not being filled by this move)
                if (isTWS && board[checkR][checkC] == ' ') {
                    return true; // DANGER: We just put a tile next to an open TWS
                }
            }
        }
        r += dr; c += dc;
    }
    return false;
}

// --- MAIN SEARCH ---

MoveCandidate Vanguard::search(const LetterBoard& board,
                               const Board& bonusBoard,
                               const TileRack& rack,
                               Spy& spy,
                               Dictionary& dict,
                               int timeLimitMs,
                               int bagSize,
                               int scoreDiff,
                               OpponentType oppType)
{
    // 1. Generate ALL legal moves
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, false);

    if (candidates.empty()) {
        MoveCandidate pass;
        pass.word[0] = '\0';
        return pass;
    }

    MoveCandidate bestMove;
    bestMove.score = -10000;
    bestMove.word[0] = '\0';

    // 2. SCORING LOOP
    for (auto& cand : candidates) {

        // A. Base Score (Points + Equity)
        int logicScore = Mechanics::calculateTrueScore(cand, board, bonusBoard);

        // B. STRATEGIC ADJUSTMENT (The General's Orders)
        int penalty = 0;

        if (oppType == OpponentType::GREEDY) {
            // TOWER DEFENSE:
            // If the opponent is Greedy, they WILL take the Triple Word Score if we offer it.
            // So we heavily penalize moves that open it.
            if (leavesTWSOpen(cand, board)) {
                penalty = 25; // Massive penalty (equivalent to a whole turn)
                // logicScore -= penalty;
            }
        }
        else if (oppType == OpponentType::SMART) {
            // Vs Smart Opponent:
            // Maybe less penalty because they might fear our retaliation?
            // For now, standard play.
            penalty = 0;
        }

        cand.score = logicScore - penalty;

        if (cand.score > bestMove.score) {
            bestMove = cand;
        }
    }

    return bestMove;
}

}