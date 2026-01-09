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

    // 2. STATIC SCORING
    for (auto& cand : candidates) {
        cand.score = Mechanics::calculateTrueScore(cand, board, bonusBoard);
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

                // 1. Calculate MY Equity (Static for this move)
                int myLeaveCounts[27];
                memcpy(myLeaveCounts, myRackCounts, sizeof(myLeaveCounts));

                const auto& move = candidates[k]; // <--- Capture reference for convenience

                for (int i=0; move.word[i] != '\0'; i++) {
                    int r = move.row + (move.isHorizontal?0:i);
                    int c = move.col + (move.isHorizontal?i:0);
                    if (board[r][c] == ' ') {
                        char letter = move.word[i];
                        if (letter >= 'a' && letter <= 'z') { if (myLeaveCounts[26]>0) myLeaveCounts[26]--; }
                        else {
                            int idx = letter-'A';
                            if (myLeaveCounts[idx]>0) myLeaveCounts[idx]--;
                            else if (myLeaveCounts[26]>0) myLeaveCounts[26]--;
                        }
                    }
                }
                TileRack myLeave = countsToRack(myLeaveCounts);
                double myEquity = Treasurer::evaluateEquity(myLeave, scoreDiff, bagSize);

                // 2. Run Simulations
                for(int b=0; b<batchSize; b++) {
                    // Simulation Setup
                    vector<char> oppTiles = spy.generateWeightedRack();
                    int oppRackCounts[27] = {0};
                    for(char c : oppTiles) { if (c == '?') oppRackCounts[26]++; else oppRackCounts[c - 'A']++; }

                    LetterBoard simBoard = board;
                    int mySimRack[27];
                    memcpy(mySimRack, myRackCounts, sizeof(mySimRack));
                    simApplyMove(simBoard, candidates[k], mySimRack);

                    TileRack oppTileRack = countsToRack(oppRackCounts);
                    vector<MoveCandidate> responses = MoveGenerator::generate(simBoard, oppTileRack, dict, false);

                    double bestOppTotal = 0.0;
                    if(!responses.empty()) {
                        for(const auto& resp : responses) {
                            int s = Mechanics::calculateTrueScore(resp, simBoard, bonusBoard);

                            // [FIX 1] Manual string length calculation for char array
                            int wordLen = 0;
                            while(wordLen < 15 && resp.word[wordLen] != '\0') wordLen++;

                            // Quick Equity Estimate for Opponent
                            double estimatedOppEquity = 0.0;
                            if (s < 25 && wordLen <= 3) estimatedOppEquity = 12.0;

                            if ((s + estimatedOppEquity) > bestOppTotal) bestOppTotal = s + estimatedOppEquity;
                        }
                    }

                    // [FIX 2] Use candidates[k].score instead of undefined 'myMoveScore'
                    batchNAV += (long long)((candidates[k].score + myEquity) - bestOppTotal);
                }
                return batchNAV;
             }));
        }

        if (timeUp && futures.empty()) break;

        // [ROOT FIX] Thread Throttle
        // If simulations finish instantly (low branching factor), yield CPU.
        // This ensures the main thread can actually check the clock.
        if (futures.size() < candidateCount && totalSims > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

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