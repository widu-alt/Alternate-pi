#include "../../include/spectre/spy.h"
#include "../../include/tile_tracker.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h"
#include "../../include/engine/dictionary.h"
#include "../../include/spectre/judge.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>

using namespace std;

namespace spectre {

Spy::Spy() {
    particles.resize(PARTICLE_COUNT);
    for(auto& p : particles) p.weight = 1.0;
}

void Spy::observeOpponentMove(const Move& move, const LetterBoard& preMoveBoard) {
    if (unseenPool.empty()) return;

    // A. DETERMINE TILES PLAYED
    // Since 'preMoveBoard' is the state BEFORE the move, any tile in the word
    // that corresponds to an empty cell ' ' MUST have come from the rack.
    std::vector<char> tilesPlayed;
    int r = move.row;
    int c = move.col;
    int dr = move.horizontal ? 0 : 1;
    int dc = move.horizontal ? 1 : 0;

    for (char letter : move.word) {
        if (preMoveBoard[r][c] == ' ') {
            tilesPlayed.push_back(letter);
        }
        r += dr;
        c += dc;
    }

    // B. CALCULATE ACTUAL SCORE
    // Now we can accurately score the move because we have the pre-move board.
    // (Mechanics::calculateMoveScore is designed to work on the board *as the move is being placed*)
    // But strictly speaking, it expects the board to be the "Context".
    // We pass preMoveBoard.
    // Note: We need a BonusBoard. For now, we assume standard board or static access.
    Board bonusBoard = createBoard();
    MoveCandidate mc;
    mc.row = move.row; mc.col = move.col; mc.isHorizontal = move.horizontal;
    int len=0; while(len < move.word.size()) { mc.word[len] = move.word[len]; len++; } mc.word[len] = '\0';

    int actualScore = Mechanics::calculateTrueScore(mc, preMoveBoard, bonusBoard);

    // C. UPDATE PARTICLES
    double totalWeight = 0.0;

    for (auto& p : particles) {
        // 1. HARD FILTER (Physical Possibility)
        std::vector<char> rackCopy = p.rack;
        bool possible = true;

        for (char t : tilesPlayed) {
            auto it = std::find(rackCopy.begin(), rackCopy.end(), t);
            if (it != rackCopy.end()) {
                *it = ' '; // Mark used
            } else {
                auto blankIt = std::find(rackCopy.begin(), rackCopy.end(), '?');
                if (blankIt != rackCopy.end()) {
                    *blankIt = ' ';
                } else {
                    possible = false;
                    break;
                }
            }
        }

        if (!possible) {
            p.weight = 0.0;
            continue;
        }

        // 2. SOFT FILTER (The Judge)
        double rationality = Spy::evaluateRationality(rack, move, board);


        p.weight *= rationality;
        totalWeight += p.weight;
    }

    // D. RESAMPLE
    if (totalWeight < 0.0001) {
        initParticles(); // Panic Reset
    } else {
        resampleParticles(totalWeight);
    }
}

// ... (Rest of Spy functions: updateGroundTruth, initParticles, resampleParticles remain the same) ...
void Spy::updateGroundTruth(const LetterBoard& board, const TileRack& myRack, const TileBag& bag) {
    unseenPool.clear();
    TileTracker tracker;
    for(int r=0; r<15; r++) for(int c=0; c<15; c++)
        if(board[r][c] != ' ') tracker.markSeen(board[r][c]);
    for(const auto& t : myRack) tracker.markSeen(t.letter);

    string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ?";
    for (char c : alphabet) {
        int count = tracker.getUnseenCount(c);
        for (int k=0; k<count; k++) unseenPool.push_back(c);
    }
    initParticles(); // Reset for stability in V1
}

void Spy::initParticles() {
    static thread_local std::mt19937 rng(std::random_device{}());
    for(int i=0; i<PARTICLE_COUNT; i++) {
        particles[i].rack.clear();
        particles[i].weight = 1.0;
        std::vector<char> pool = unseenPool;
        std::shuffle(pool.begin(), pool.end(), rng);
        int draw = std::min((int)pool.size(), 7);
        for(int k=0; k<draw; k++) particles[i].rack.push_back(pool[k]);
    }
}

void Spy::resampleParticles(double totalWeight) {
    std::vector<Particle> newParticles;
    newParticles.reserve(PARTICLE_COUNT);
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, totalWeight);

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        double r = dist(rng);
        double currentSum = 0.0;
        for (const auto& p : particles) {
            currentSum += p.weight;
            if (currentSum >= r) {
                newParticles.push_back(p);
                break;
            }
        }
    }
    for(auto& p : newParticles) p.weight = 1.0;
    particles = newParticles;
}

std::vector<char> Spy::generateWeightedRack() const {
    if (particles.empty()) return {};
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, particles.size() - 1);
    return particles[dist(rng)].rack;
}

}