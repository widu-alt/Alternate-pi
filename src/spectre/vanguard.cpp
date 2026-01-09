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

    MoveCandidate Vanguard::search(const LetterBoard& board, const Board& bonusBoard,
                                   const TileRack& rack, const Spy& spy,
                                   Dictionary& dict, int timeLimitMs,
                                   int bagSize, int scoreDiff) {

    // 1. GENERATE
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, true);
    if (candidates.empty()) {
        MoveCandidate pass; pass.word[0] = '\0'; pass.score = 0; return pass;
    }

    // 2. STATIC SCORING + EQUITY FILTER
    // We can run a fast static Equity check here to prune garbage moves
    for (auto& cand : candidates) {
        cand.score = Mechanics::calculateTrueScore(cand, board, bonusBoard);
    }

    sort(candidates.begin(), candidates.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
        return a.score > b.score;
    });

    // 3. MONTE CARLO SIMULATION (UNLEASHED)
    int candidateCount = min((int)candidates.size(), 12);

    int myRackCounts[27] = {0};
    for (const Tile& t : rack) {
        if (t.letter == '?') myRackCounts[26]++;
        else if (isalpha(t.letter)) myRackCounts[toupper(t.letter) - 'A']++;
    }

    vector<long long> totalNav(candidateCount, 0); // Net Asset Value accumulator
    vector<int> simCounts(candidateCount, 0);

    auto startTime = chrono::high_resolution_clock::now();
    int totalSims = 0;
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

            futures.push_back(async(launch::async,
                [k, batchSize, &candidates, board, bonusBoard, &spy, myRackCounts, &dict, bagSize, scoreDiff]() {

                long long batchNAV = 0;

                // [NEW] Pre-calculate My Leave Equity once (it's static for the move)
                // We need to reconstruct the leave rack from the move
                int tempRack[27];
                memcpy(tempRack, myRackCounts, sizeof(tempRack));
                simApplyMove((LetterBoard&)board, candidates[k], tempRack); // (Hack: cast const away for temp calc or use helper logic)
                // Actually simApplyMove modifies board, we need just rack logic.
                // Let's rely on the simulation loop to do it cleanly.

                for(int b=0; b<batchSize; b++) {
                    // A. SIMULATION SETUP
                    vector<char> oppTiles = spy.generateWeightedRack();
                    int oppRackCounts[27] = {0};
                    for(char c : oppTiles) {
                        if (c == '?') oppRackCounts[26]++;
                        else oppRackCounts[c - 'A']++;
                    }

                    LetterBoard simBoard = board;
                    int mySimRack[27];
                    memcpy(mySimRack, myRackCounts, sizeof(mySimRack));

                    // B. APPLY MY MOVE
                    int myMoveScore = candidates[k].score;
                    simApplyMove(simBoard, candidates[k], mySimRack);

                    // [NEW] C. EVALUATE MY EQUITY (The Treasurer)
                    TileRack myLeave = countsToRack(mySimRack);
                    // Assume 0 score diff for static check or pass actual diff?
                    // For simulation, we assume parity or use current game state if passed in.
                    // Assuming scoreDiff = 0 for V1 simplicity in loop.
                    double myEquity = Treasurer::evaluateEquity(myLeave, scoreDiff, bagSize);

                    // D. OPPONENT RESPONSE
                    TileRack oppTileRack = countsToRack(oppRackCounts);
                    vector<MoveCandidate> responses = MoveGenerator::generate(simBoard, oppTileRack, dict, false);

                    int bestOppScore = 0;
                    double bestOppEquity = 0.0;

                    if(!responses.empty()) {
                        // Opponent plays optimally for NAV (Score + Equity)
                        // But finding Equity for every opp move is too slow.
                        // Approximation: Opponent plays for Score.
                        for(const auto& resp : responses) {
                            int s = Mechanics::calculateTrueScore(resp, simBoard, bonusBoard);
                            if(s > bestOppScore) bestOppScore = s;
                        }
                    }

                    // E. CALCULATE NET ASSET VALUE (NAV)
                    // NAV = (MyScore + MyEquity) - (OppScore)
                    // Note: We subtract OppScore because it hurts our margin.
                    // We DO NOT subtract OppEquity because we can't control their luck next turn easily.

                    batchNAV += (long long)((myMoveScore + myEquity) - bestOppScore);
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

    {
        ScopedLogger log;
        std::cout << "[VANGUARD] Total Sims: " << totalSims << std::endl;
        std::cout << "[VANGUARD] NAV Analysis (Treasurer Enabled):" << std::endl;
        std::cout << left << setw(15) << "Word" << setw(10) << "Score" << setw(10) << "AvgNAV" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
    }

    // 4. SELECTION
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