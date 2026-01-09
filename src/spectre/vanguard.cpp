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
                                   int scoreDiff, // (MyScore - OppScore)
                                   OpponentType oppType)
{
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, false);

    if (candidates.empty()) {
        MoveCandidate pass; pass.word[0] = '\0'; return pass;
    }

    MoveCandidate bestMove;
    bestMove.score = -10000;
    bestMove.word[0] = '\0';

    // PANIC MODE CHECK
    // If we are losing by > 40 points, disable defense. We need to catch up.
    bool panicMode = (scoreDiff < -40);

    for (auto& cand : candidates) {

        // A. Base Score
        int logicScore = Mechanics::calculateTrueScore(cand, board, bonusBoard);

        // B. Rack Equity (Keep Good Tiles)
        float leaveVal = 0.0f;
        for(char c : cand.leave) {
            if(c == '\0') break;
            leaveVal += Heuristics::getLeaveValue(c);
        }

        // C. Tower Defense
        int penalty = 0;

        // Only apply penalty if opponent is Greedy AND we are NOT panicking.
        if (oppType == OpponentType::GREEDY && !panicMode) {
            if (leavesTWSOpen(cand, board)) {
                penalty = 25;
            }
        }

        cand.score = logicScore + (int)leaveVal - penalty;

        if (cand.score > bestMove.score) {
            bestMove = cand;
        }
    }

    return bestMove;
}

}