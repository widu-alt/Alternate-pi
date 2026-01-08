#include "../../include/spectre/vanguard.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h" // Source of Truth
#include "../../include/heuristics.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <future>

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

    // 1. GENERATE (Find all my legal moves)
    vector<MoveCandidate> candidates = MoveGenerator::generate(board, rack, dict, true);
    if (candidates.empty()) {
        MoveCandidate pass; pass.word[0] = '\0'; pass.score = 0; return pass;
    }

    // 2. STATIC SCORING (Filter garbage)
    for (auto& cand : candidates) {
        cand.score = Mechanics::calculateTrueScore(cand, board, bonusBoard);
    }

    // Sort High -> Low
    sort(candidates.begin(), candidates.end(), [](const MoveCandidate& a, const MoveCandidate& b) {
        return a.score > b.score;
    });

    // OPTIMIZATION: If we have a massive lead move (Bingo), take it.
    if (candidates.size() > 1 && candidates[0].score > candidates[1].score + 40) {
        return candidates[0];
    }
    if (candidates.size() == 1) return candidates[0];

    // 3. MONTE CARLO SIMULATION (The "Closed World" logic)
    // We only simulate the top few moves to save time
    int candidateCount = min((int)candidates.size(), 12);

    int myRackCounts[27] = {0};
    for (const Tile& t : rack) {
        if (t.letter == '?') myRackCounts[26]++;
        else if (isalpha(t.letter)) myRackCounts[toupper(t.letter) - 'A']++;
    }

    vector<long long> totalNetScore(candidateCount, 0);
    vector<int> simCounts(candidateCount, 0);

    auto startTime = chrono::high_resolution_clock::now();

    // Simulations
    while (true) {
        auto now = chrono::high_resolution_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - startTime).count() >= timeLimitMs) break;

        vector<future<long long>> futures;

        for (int k = 0; k < candidateCount; k++) {
             futures.push_back(async(launch::async, [k, &candidates, board, bonusBoard, &spy, myRackCounts, &dict]() {

                long long batchScore = 0;
                int BATCH_SIZE = 10;

                for(int b=0; b<BATCH_SIZE; b++) {
                    // A. ASK SPY (The Input)
                    vector<char> oppTiles = spy.generateWeightedRack();
                    int oppRackCounts[27] = {0};
                    for(char c : oppTiles) {
                        if (c == '?') oppRackCounts[26]++;
                        else oppRackCounts[c - 'A']++;
                    }

                    // B. SETUP SIMULATION
                    LetterBoard simBoard = board;
                    int mySimRack[27];
                    memcpy(mySimRack, myRackCounts, sizeof(mySimRack));

                    // C. APPLY MY MOVE
                    int myMoveScore = candidates[k].score;
                    simApplyMove(simBoard, candidates[k], mySimRack);

                    // D. OPPONENT RESPONSE (Closed World)
                    // We assume opponent plays their best possible move with the rack Spy guessed.
                    // We DO NOT simulate drawing new tiles.
                    TileRack oppTileRack = countsToRack(oppRackCounts);

                    vector<MoveCandidate> responses = MoveGenerator::generate(simBoard, oppTileRack, dict, false);

                    int bestOppScore = 0;
                    if(!responses.empty()) {
                        // Find the best score they can get
                        for(const auto& resp : responses) {
                            int s = Mechanics::calculateTrueScore(resp, simBoard, bonusBoard);
                            if(s > bestOppScore) bestOppScore = s;
                        }
                    }

                    // Net Score = My Points - Their Response
                    // A move that scores 30 but allows a 50-point response is worth -20.
                    batchScore += (myMoveScore - bestOppScore);
                }
                return batchScore;
             }));
        }

        for(int k=0; k<candidateCount; k++) {
            totalNetScore[k] += futures[k].get();
            simCounts[k] += 10;
        }
    }

    // 4. SELECTION
    int bestIdx = 0;
    double bestVal = -999999.0;

    for (int i = 0; i < candidateCount; i++) {
        if (simCounts[i] == 0) continue;
        double avgNet = (double)totalNetScore[i] / simCounts[i];

        // Weighted: 70% Sim (Strategy) + 30% Raw Score (Greed)
        double heuristic = (avgNet * 0.7) + (candidates[i].score * 0.3);

        if (heuristic > bestVal) {
            bestVal = heuristic;
            bestIdx = i;
        }
    }

    return candidates[bestIdx];
}

}