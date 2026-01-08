#include "../../include/spectre/judge.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/heuristics.h"
#include "../../include/engine/mechanics.h"
#include <algorithm>
#include <cstring>
#include <iostream>

using namespace std;
using namespace std::chrono;

namespace spectre {

// --- INTERNAL LIGHTWEIGHT HELPERS ---

// Convert move to standard format for return
static Move candidateToMove(const MoveCandidate& cand) {
    Move m;
    m.type = MoveType::PLAY;
    m.row = cand.row;
    m.col = cand.col;
    m.horizontal = cand.isHorizontal;
    int i=0;
    while(i < 15 && cand.word[i] != '\0') {
        m.word += cand.word[i];
        i++;
    }
    return m;
}

// Convert histogram back to TileRack for the Generator
static TileRack countsToRack(const int* counts) {
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

// --- MAIN SOLVER ---

Move Judge::solveEndgame(const LetterBoard& board, const Board& bonusBoard,
                         const TileRack& myRack, const TileRack& oppRack, Dictionary& dict) {

    // 1. Setup Lightweight Racks (Histograms)
    int myRackCounts[27] = {0};
    int oppRackCounts[27] = {0};

    for (const auto& t : myRack) {
        if (t.letter == '?') myRackCounts[26]++;
        else if (isalpha(t.letter)) myRackCounts[toupper(t.letter) - 'A']++;
    }
    for (const auto& t : oppRack) {
        if (t.letter == '?') oppRackCounts[26]++;
        else if (isalpha(t.letter)) oppRackCounts[toupper(t.letter) - 'A']++;
    }

    // 2. Generate Root Moves (Maximizer)
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, myRack, dict, true);

    if (candidates.empty()) return Move(MoveType::PASS);

    // Sort moves to optimize pruning
    for(auto& c : candidates) {
        c.score = Mechanics::calculateTrueScore(c, board, bonusBoard);
    }
    sort(candidates.begin(), candidates.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
        return a.score > b.score;
    });

    MoveCandidate bestMove = candidates[0];
    int bestVal = -999999;
    int alpha = -999999;
    int beta = 999999;

    auto startTime = steady_clock::now();
    int timeBudgetMs = 4000; // 4 seconds for endgame is reasonable

    // 3. Search Loop
    for (const auto& move : candidates) {
        // Create Next State
        LetterBoard nextBoard = board;
        int nextMyRack[27];
        memcpy(nextMyRack, myRackCounts, sizeof(nextMyRack));

        // Lightweight Apply
        applyMove(nextBoard, move, nextMyRack);

        int moveScore = move.score;

        // CHECK: Did I go out?
        bool rackEmpty = true;
        for(int i=0; i<27; i++) if(nextMyRack[i]>0) { rackEmpty=false; break; }

        if (rackEmpty) {
            // BONUS RULE: Winner gets 2 * Sum of Opponent's Remaining Tiles
            int bonus = 0;
            for(int i=0; i<26; i++) bonus += oppRackCounts[i] * Heuristics::getTileValue((char)('A'+i));

            // Return immediately - going out on turn 1 of endgame is almost always optimal
            return candidateToMove(move);
        }

        // Recurse (Opponent's Turn)
        // Passes = 0 because a play resets the pass count
        int val = moveScore - minimax(nextBoard, bonusBoard, oppRackCounts, nextMyRack, dict,
                                      -beta, -alpha, false, 0, 0, startTime, timeBudgetMs);

        if (val > bestVal) {
            bestVal = val;
            bestMove = move;
        }
        alpha = std::max(alpha, val);
        if (alpha >= beta) break; // Prune

        if (duration_cast<milliseconds>(steady_clock::now() - startTime).count() > timeBudgetMs) break;
    }

    return candidateToMove(bestMove);
}

// --- MINIMAX RECURSION ---

int Judge::minimax(LetterBoard board, const Board& bonusBoard,
                   int* currentRackCounts, int* otherRackCounts,
                   Dictionary& dict,
                   int alpha, int beta,
                   bool maximizingPlayer,
                   int passesInARow,
                   int depth,
                   const steady_clock::time_point& startTime,
                   int timeBudgetMs) {

    // 1. Time Limit Check (every 4th depth to save calls)
    if ((depth & 3) == 0 && duration_cast<milliseconds>(steady_clock::now() - startTime).count() > timeBudgetMs) {
        // Heuristic: Current Score differential (simple fallback)
        return 0;
    }

    // 2. Generate Moves
    TileRack rack = countsToRack(currentRackCounts);
    vector<MoveCandidate> moves = MoveGenerator::generate(board, rack, dict, false); // No threading

    // 3. Handle PASS logic
    if (moves.empty()) {
        passesInARow++;

        // TERMINATION RULE: 6 Passes ends game (3 per player)
        // If we hit the limit, calculate the deduction penalty.
        if (passesInARow >= 6) {
             // Deduction Rule: Subtract value of remaining tiles from self.
             // Net Score = -(MyRem) - -(OppRem) = OppRem - MyRem
            int myRem = 0;
            for(int i=0; i<26; i++) myRem += currentRackCounts[i] * Heuristics::getTileValue((char)('A'+i));

            int oppRem = 0;
            for(int i=0; i<26; i++) oppRem += otherRackCounts[i] * Heuristics::getTileValue((char)('A'+i));

            return (oppRem - myRem);
        }

        // Pass continues game -> Switch turn
        return -minimax(board, bonusBoard, otherRackCounts, currentRackCounts, dict,
                        -beta, -alpha, !maximizingPlayer, passesInARow, depth+1, startTime, timeBudgetMs);
    }

    // 4. Sort (Max Score First)
    for(auto& m : moves) m.score = calculateMoveScore(board, bonusBoard, m);
    sort(moves.begin(), moves.end(), [](const MoveCandidate& a, const MoveCandidate& b){ return a.score > b.score; });

    int bestVal = -999999;

    // 5. Iterate Moves
    for (const auto& move : moves) {
        LetterBoard nextBoard = board;
        int nextRack[27];
        memcpy(nextRack, currentRackCounts, sizeof(nextRack));

        applyMove(nextBoard, move, nextRack);
        int moveScore = move.score;

        // Check Empty Rack Bonus
        bool rackEmpty = true;
        for(int i=0; i<27; i++) if(nextRack[i]>0) { rackEmpty=false; break; }

        if (rackEmpty) {
            // Apply 2x Opponent Sum Bonus
            int bonus = 0;
            for(int i=0; i<26; i++) bonus += otherRackCounts[i] * Heuristics::getTileValue((char)('A'+i));

            int total = moveScore + (2 * bonus);

            if (total > bestVal) bestVal = total;
            alpha = std::max(alpha, bestVal);
            if (alpha >= beta) break;
            continue; // Game over, no recursion
        }

        // Recurse (Passes reset to 0 on play)
        int val = moveScore - minimax(nextBoard, bonusBoard, otherRackCounts, nextRack, dict,
                                      -beta, -alpha, !maximizingPlayer, 0, depth+1, startTime, timeBudgetMs);

        if (val > bestVal) bestVal = val;
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) break;
    }

    return bestVal;
}

// Wrapper for consistency with Mechanics
int Judge::calculateMoveScore(const LetterBoard& board, const Board& bonusBoard, const MoveCandidate& move) {
    return Mechanics::calculateTrueScore(move, board, bonusBoard);
}

// Lightweight Apply (No GameState)
void Judge::applyMove(LetterBoard& board, const MoveCandidate& move, int* rackCounts) {
    int r = move.row;
    int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    for (int i=0; move.word[i] != '\0'; i++) {
        if (board[r][c] == ' ') {
            board[r][c] = move.word[i];

            // Remove from histogram
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

} // namespace