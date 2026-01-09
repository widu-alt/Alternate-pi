#include "../../include/spectre/vanguard.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h" // Source of Truth
#include "../../include/spectre/logger.h"
#include "../../include/spectre/treasurer.h"
#include "../../include/heuristics.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <future>
#include <iomanip>

using namespace std;

namespace spectre {

// Thread-local RNG
thread_local mt19937 rng(random_device{}());

// --- INTERNAL HELPERS ---

// Fast simulation of applying a move (updates board char array only)
void simApplyMove(LetterBoard& board, const MoveCandidate& move, int* rackCounts) {
    int r = move.row;
    int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    for (int i=0; move.word[i] != '\0'; i++) {
        if (board[r][c] == ' ') {
            board[r][c] = move.word[i];

            // Remove from rack histogram
            char letter = move.word[i];
            if (letter >= 'a' && letter <= 'z') {
                if (rackCounts[26] > 0) rackCounts[26]--;
            } else {
                int idx = letter - 'A';
                if (rackCounts[idx] > 0) rackCounts[idx]--;
                else if (rackCounts[26] > 0) rackCounts[26]--;
            }
        }
        r += dr; c += dc;
    }
}

// Convert histogram back to TileRack
TileRack countsToRack(const int* counts) {
    TileRack rack;
    rack.reserve(7);
    for(int i=0; i<26; i++) {
        for(int k=0; k<counts[i]; k++) {
            Tile t; t.letter = (char)('A' + i); t.points = 0;
            rack.push_back(t);
        }
    }
    for(int k=0; k<counts[26]; k++) {
        Tile t; t.letter = '?'; t.points = 0;
        rack.push_back(t);
    }
    return rack;
}

// --- MAIN SEARCH ---

MoveCandidate Vanguard::search(const LetterBoard& board,
                               const Board& bonusBoard,
                               const TileRack& rack,
                               Spy& spy,
                               Dictionary& dict,
                               int timeLimitMs,
                               int bagSize,
                               int scoreDiff)
{
    // 1. Generate ALL legal moves
    // (We use the raw generator to get everything)
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, false);

    if (candidates.empty()) {
        MoveCandidate pass;
        pass.word[0] = '\0';
        return pass;
    }

    // 2. PHASE 1 LOBOTOMY: Remove Hard Pruning Loops
    // Previously, we called general->approve() and treasurer->approve() here.
    [cite_start]// The report confirms this was "The Hard Pruning Trap"[cite: 113].
    // We now keep ALL moves that are legally generated.

    // 3. Simulation Prep (MCTS / Rollout)
    MoveCandidate bestMove;
    bestMove.score = -10000;
    bestMove.word[0] = '\0';

    // Simple heuristic fallback for now (Prevent "Random Play" behavior until MCTS is tuned)
    // This ensures Cutie_Pi is at least as smart as Speedi_Pi while we fix the brain.
    for (auto& cand : candidates) {

        // A. Static Evaluation (Score)
        int logicScore = Mechanics::calculateTrueScore(cand, board, bonusBoard);

        // B. Apply Soft Bias (Future Home of The General/Treasurer)
        // For now, we purely rely on score + equity to establish a baseline.
        // float utility = logicScore + Treasurer::getUtilityAdjustment(...)

        cand.score = logicScore; // Store raw score for now

        if (cand.score > bestMove.score) {
            bestMove = cand;
        }
    }

    // TODO: Phase 2 - Re-enable MCTS here, but ONLY on the top X candidates (Soft Pruning),
    // and using The General's new "Profiler" logic to bias the results.

    return bestMove;
}

}