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

/*
bool leavesTWSOpen(const MoveCandidate& move, const LetterBoard& board) {
    int r = move.row; int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    for (int i=0; move.word[i] != '\0'; i++) {
        int nr[] = {r+1, r-1, r, r};
        int nc[] = {c, c, c+1, c-1};
        for(int k=0; k<4; k++) {
            int checkR = nr[k]; int checkC = nc[k];
            if (checkR >= 0 && checkR < 15 && checkC >= 0 && checkC < 15) {
                // Check for TWS (0,0), (0,7)...
                bool isTWS = ((checkR==0 || checkR==7 || checkR==14) && (checkC==0 || checkC==7 || checkC==14) && !(checkR==7 && checkC==7));
                if (isTWS && board[checkR][checkC] == ' ') return true;
            }
        }
        r += dr; c += dc;
    }
    return false;
}
*/

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

    MoveCandidate Vanguard::search(const LetterBoard& board, const Board& bonusBoard,
                               const TileRack& rack, const Spy& spy,
                               Dictionary& dict, int timeLimitMs,
                               int bagSize, int scoreDiff) {

    // 1. GENERATE
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, true);
    if (candidates.empty()) {
        MoveCandidate pass; pass.word[0] = '\0'; pass.score = 0; return pass;
    }

    // 2. STATIC SCORING
    for (auto& cand : candidates) {
        int logicScore = 0;
        if (cand.isHorizontal)
            logicScore = Mechanics::calculateTrueScoreFast<true>(cand, board, bonusBoard);
        else
            logicScore = Mechanics::calculateTrueScoreFast<false>(cand, board, bonusBoard);
    }

    sort(candidates.begin(), candidates.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
        return a.score > b.score;
    });

    // 3. MONTE CARLO SIMULATION
    int candidateCount = min((int)candidates.size(), 12);

    int myRackCounts[27] = {0};
    for (const Tile& t : rack) {
        if (t.letter == '?') myRackCounts[26]++;
        else if (isalpha(t.letter)) myRackCounts[toupper(t.letter) - 'A']++;
    }

    vector<long long> totalNav(candidateCount, 0);
    vector<int> simCounts(candidateCount, 0);

    auto startTime = chrono::high_resolution_clock::now();
    int totalSims = 0;

    // Concurrency
    unsigned int nThreads = std::thread::hardware_concurrency();
    if(nThreads == 0) nThreads = 2;

    bool timeUp = false;
    while (!timeUp) {
        vector<future<long long>> futures;
        int batchSize = 50;

        for (int k = 0; k < candidateCount; k++) {
             if (chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() >= (timeLimitMs - 10)) {
                 timeUp = true; break;
             }

             // CAPTURE 'board' BY VALUE (Creates a clean copy per thread)
             futures.push_back(async(launch::async, [k, batchSize, candidates, board, bonusBoard, &spy, myRackCounts, &dict, bagSize, scoreDiff]() {

                long long batchNAV = 0;

                // STEP A: Calculate My Equity ONCE (Static for this move)
                // Use a temporary rack to calculate the leave, DO NOT touch the board yet.
                int myLeaveCounts[27];
                memcpy(myLeaveCounts, myRackCounts, sizeof(myLeaveCounts));

                // Manual removal (Logic extracted from simApplyMove to avoid board dependency)
                const auto& move = candidates[k];
                for (int i=0; move.word[i] != '\0'; i++) {
                    // Check if tile was placed (we need board to know if it was empty)
                    int r = move.row + (move.isHorizontal ? 0 : i);
                    int c = move.col + (move.isHorizontal ? i : 0);
                    // Only remove if we placed it (board was empty)
                    if (board[r][c] == ' ') {
                        char letter = move.word[i];
                        if (letter >= 'a' && letter <= 'z') { // Blank
                            if (myLeaveCounts[26] > 0) myLeaveCounts[26]--;
                        } else {
                            int idx = letter - 'A';
                            if (myLeaveCounts[idx] > 0) myLeaveCounts[idx]--;
                            else if (myLeaveCounts[26] > 0) myLeaveCounts[26]--;
                        }
                    }
                }

                TileRack myLeave = countsToRack(myLeaveCounts);
                double myEquity = Treasurer::evaluateEquity(myLeave, scoreDiff, bagSize);

                // STEP B: Run Simulations
                for(int b=0; b<batchSize; b++) {
                    // 1. Setup World
                    vector<char> oppTiles = spy.generateWeightedRack();
                    int oppRackCounts[27] = {0};
                    for(char c : oppTiles) {
                        if (c == '?') oppRackCounts[26]++;
                        else oppRackCounts[c - 'A']++;
                    }

                    // 2. Apply My Move (On a fresh board copy)
                    LetterBoard simBoard = board;
                    int mySimRack[27]; // Dummy, we already calculated equity
                    memcpy(mySimRack, myRackCounts, sizeof(mySimRack));
                    simApplyMove(simBoard, candidates[k], mySimRack);

                    // 3. Opponent Response
                    TileRack oppTileRack = countsToRack(oppRackCounts);
                    vector<MoveCandidate> responses = MoveGenerator::generate(simBoard, oppTileRack, dict, false);

                    int bestOppScore = 0;
                    if(!responses.empty()) {
                        for(const auto& resp : responses) {
                            int s = Mechanics::calculateTrueScore(resp, simBoard, bonusBoard);
                            if(s > bestOppScore) bestOppScore = s;
                        }
                    }

                    // 4. NAV Calculation
                    batchNAV += (long long)((candidates[k].score + myEquity) - bestOppScore);
                }
                return batchNAV;
             }));
        }

        if (timeUp && futures.empty()) break;

        for(size_t k=0; k<futures.size(); k++) {
            totalNav[k] += futures[k].get();
            simCounts[k] += batchSize;
            totalSims += batchSize;
        }
    }

    // 4. SELECTION (Average NAV)
    int bestIdx = 0;
    double bestVal = -999999.0;

    for (int i = 0; i < candidateCount; i++) {
        if (simCounts[i] == 0) continue;
        double avgNAV = (double)totalNav[i] / simCounts[i];

        {
            ScopedLogger log;
            std::cout << left << setw(15) << candidates[i].word
                      << setw(10) << candidates[i].score
                      << setw(10) << avgNAV << std::endl;
        }

        if (avgNAV > bestVal) {
            bestVal = avgNAV;
            bestIdx = i;
        }
    }

    return candidates[bestIdx];
}

}