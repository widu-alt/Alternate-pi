#include "../../include/spectre/spy.h"
#include "../../include/tile_tracker.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h"
#include "../../include/engine/dictionary.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>
#include <random>

using namespace std;

namespace spectre {

Spy::Spy() {
    particles.resize(PARTICLE_COUNT);
    for(auto& p : particles) p.weight = 1.0;
}

void Spy::observeOpponentMove(const Move& move, const LetterBoard& preMoveBoard) {
    if (unseenPool.empty()) return;

    // 1. DEDUCE PLAYED TILES
    std::vector<char> tilesPlayed;
    int r = move.row; int c = move.col;
    int dr = move.horizontal ? 0 : 1;
    int dc = move.horizontal ? 1 : 0;

    for (char letter : move.word) {
        if (preMoveBoard[r][c] == ' ') {
            tilesPlayed.push_back(letter);
        }
        r += dr; c += dc;
    }

    // 2. SCORE THE MOVE (The Truth)
    Board bonusBoard = createBoard();
    MoveCandidate mc;
    mc.row = move.row;
    mc.col = move.col;
    mc.isHorizontal = move.horizontal;

    int len = 0;
    while (len < 15 && len < (int)move.word.size()) {
        mc.word[len] = move.word[len];
        len++;
    }
    mc.word[len] = '\0';

    int actualScore = Mechanics::calculateTrueScore(mc, preMoveBoard, bonusBoard);

    // 3. UPDATE PARTICLES (The Filter)
    double totalWeight = 0.0;

    for (auto& p : particles) {
        // A. HARD FILTER (Do they have the tiles?)
        std::vector<char> rackCopy = p.rack;
        bool possible = true;

        for (char t : tilesPlayed) {
            auto it = std::find(rackCopy.begin(), rackCopy.end(), t);
            if (it != rackCopy.end()) {
                *it = ' ';
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

        // B. SOFT FILTER (Rationality Check)
        double rationality = evaluateRationality(p.rack, actualScore, preMoveBoard);
        p.weight *= rationality;
        totalWeight += p.weight;
    }

    // 4. RESAMPLE (Survival of the Fittest)
    if (totalWeight < 0.0001) {
        initParticles(); // Panic Reset: Our model was wrong, reboot.
    } else {
        resampleParticles(totalWeight);
    }

    // 5. TRANSITION STATE (The Memory Fix)
    // Remove the played tiles from the surviving particles.
    // They now represent the opponent's rack *after* the move, but *before* drawing.
    for(auto& p : particles) {
        for(char t : tilesPlayed) {
            auto it = std::find(p.rack.begin(), p.rack.end(), t);
            if(it != p.rack.end()) {
                p.rack.erase(it);
            } else {
                // Must be a blank
                auto bit = std::find(p.rack.begin(), p.rack.end(), '?');
                if(bit != p.rack.end()) p.rack.erase(bit);
            }
        }
    }
}

double Spy::evaluateRationality(const std::vector<char>& rack, int actualScore, const LetterBoard& board) {
    int bestPossible = findBestPossibleScore(rack, board);
    if (actualScore > bestPossible) return 0.0;

    int delta = bestPossible - actualScore;
    if (delta == 0) return 1.0;
    if (delta < 5)  return 0.9;
    if (delta < 15) return 0.5;
    if (delta < 30) return 0.1;
    return 0.01;
}

int Spy::findBestPossibleScore(const std::vector<char>& rack, const LetterBoard& board) {
    TileRack tRack;
    for(char c : rack) {
        Tile t; t.letter = c; t.points = 0;
        tRack.push_back(t);
    }

    vector<MoveCandidate> moves = MoveGenerator::generate(board, tRack, gDictionary, false);
    if (moves.empty()) return 0;

    int maxScore = 0;
    Board bonusBoard = createBoard();

    for (const auto& m : moves) {
        int s = Mechanics::calculateTrueScore(m, board, bonusBoard);
        if (s > maxScore) maxScore = s;
    }
    return maxScore;
}

void Spy::updateGroundTruth(const LetterBoard& board, const TileRack& myRack, const TileBag& bag) {
    // 1. Recalculate the Unseen Pool (Global Truth)
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

    // 2. REFILL PARTICLES (Evolution)
    // Instead of initParticles() (which erases memory), we just top up the racks.
    static thread_local std::mt19937 rng(std::random_device{}());

    if(particles.empty()) {
        initParticles();
        return;
    }

    for (auto& p : particles) {
        // Sanity Check: If rack is too big (shouldn't happen), trim
        if (p.rack.size() > 7) p.rack.resize(7);

        // Refill to 7 tiles from the new unseen pool
        while (p.rack.size() < 7 && !unseenPool.empty()) {
            std::uniform_int_distribution<int> dist(0, unseenPool.size() - 1);
            int idx = dist(rng);
            p.rack.push_back(unseenPool[idx]);
        }
    }
}

void Spy::initParticles() {
    static thread_local std::mt19937 rng(std::random_device{}());
    for(int i=0; i<PARTICLE_COUNT; i++) {
        particles[i].rack.clear();
        particles[i].weight = 1.0;

        // Safety: If pool is empty, we can't fill.
        if (unseenPool.empty()) continue;

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

    // Normalize weights after resampling
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