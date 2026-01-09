#include "../../include/spectre/vanguard.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h" // Source of Truth
#include "../../include/spectre/logger.h"
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
                               Dictionary& dict, int timeLimitMs) {

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

    {
        ScopedLogger log;
        std::cout << "[VANGUARD] Generated " << candidates.size() << " moves." << std::endl;
        std::cout << "[VANGUARD] Top Static: " << candidates[0].word << " (" << candidates[0].score << ")" << std::endl;
    }

    // 3. MONTE CARLO SIMULATION (UNLEASHED)
    int candidateCount = min((int)candidates.size(), 12);

    int myRackCounts[27] = {0};
    for (const Tile& t : rack) {
        if (t.letter == '?') myRackCounts[26]++;
        else if (isalpha(t.letter)) myRackCounts[toupper(t.letter) - 'A']++;
    }

    vector<long long> totalNetScore(candidateCount, 0);
    vector<int> simCounts(candidateCount, 0);

    auto startTime = chrono::high_resolution_clock::now();
    int totalSims = 0;

    // Concurrency Check
    unsigned int nThreads = std::thread::hardware_concurrency();
    if(nThreads == 0) nThreads = 2;

    // SPRINT LOOP: Run until time runs out
    bool timeUp = false;
    while (!timeUp) {
        vector<future<long long>> futures;
        int batchSize = 50;

        for (int k = 0; k < candidateCount; k++) {
             // Stop dispatching if time is critical (buffer 10ms)
             if (chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() >= (timeLimitMs - 10)) {
                 timeUp = true;
                 break;
             }

             futures.push_back(async(launch::async, [k, batchSize, &candidates, board, bonusBoard, &spy, myRackCounts, &dict]() {
                long long batchScore = 0;
                for(int b=0; b<batchSize; b++) {
                    // A. ASK SPY
                    vector<char> oppTiles = spy.generateWeightedRack();
                    int oppRackCounts[27] = {0};
                    for(char c : oppTiles) {
                        if (c == '?') oppRackCounts[26]++;
                        else oppRackCounts[c - 'A']++;
                    }

                    // B. SETUP SIM
                    LetterBoard simBoard = board;
                    int mySimRack[27];
                    memcpy(mySimRack, myRackCounts, sizeof(mySimRack));

                    // C. APPLY MOVE
                    int myMoveScore = candidates[k].score;
                    simApplyMove(simBoard, candidates[k], mySimRack);

                    // D. OPPONENT RESPONSE
                    TileRack oppTileRack = countsToRack(oppRackCounts);
                    vector<MoveCandidate> responses = MoveGenerator::generate(simBoard, oppTileRack, dict, false);

                    int bestOppScore = 0;
                    if(!responses.empty()) {
                        for(const auto& resp : responses) {
                            int s = Mechanics::calculateTrueScore(resp, simBoard, bonusBoard);
                            if(s > bestOppScore) bestOppScore = s;
                        }
                    }

                    // Net Score = My Score - Their Best Response
                    batchScore += (myMoveScore - bestOppScore);
                }
                return batchScore;
             }));
        }

        if (timeUp && futures.empty()) break;

        // Collect Results
        for(size_t k=0; k<futures.size(); k++) {
            totalNetScore[k] += futures[k].get();
            simCounts[k] += batchSize;
            totalSims += batchSize;
        }
    }

    {
        ScopedLogger log;
        std::cout << "[VANGUARD] Total Sims: " << totalSims << " in " << timeLimitMs << "ms" << std::endl;
        std::cout << "[VANGUARD] Sim Results:" << std::endl;
        std::cout << left << setw(15) << "Word" << setw(10) << "Static" << setw(10) << "NetScore" << std::endl;
        std::cout << "-----------------------------------" << std::endl;
    }

    // 4. SELECTION
    int bestIdx = 0;
    double bestVal = -999999.0;

    for (int i = 0; i < candidateCount; i++) {
        if (simCounts[i] == 0) continue;
        double avgNet = (double)totalNetScore[i] / simCounts[i];

        // HEURISTIC: 60% Strategy (Net), 40% Aggression (Raw)
        double heuristic = (avgNet * 0.6) + (candidates[i].score * 0.4);

        {
            ScopedLogger log;
            std::cout << left << setw(15) << candidates[i].word
                      << setw(10) << candidates[i].score
                      << setw(10) << avgNet << std::endl;
        }

        if (heuristic > bestVal) {
            bestVal = heuristic;
            bestIdx = i;
        }
    }

    {
        ScopedLogger log;
        std::cout << "[VANGUARD] Selected: " << candidates[bestIdx].word
                  << " (Sims: " << simCounts[bestIdx] << ")" << std::endl;
    }

    return candidates[bestIdx];
}
}