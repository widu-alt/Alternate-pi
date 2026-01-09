#include "../../include/spectre/spy.h"
#include "../../include/tile_tracker.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h"
#include "../../include/engine/dictionary.h"
#include "../../include/spectre/logger.h"
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

// Helper to generate a cache key from a rack
std::string getRackKey(const std::vector<char>& rack) {
    std::string s(rack.begin(), rack.end());
    std::sort(s.begin(), s.end());
    return s;
}

void Spy::observeOpponentMove(const Move& move, const LetterBoard& preMoveBoard) {
    if (unseenPool.empty()) return;

    {
        ScopedLogger log;
        std::cout << "[SPY] Observing Move: " << move.word << std::endl;
    }

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

    // 2. SCORE THE MOVE
    Board bonusBoard = createBoard();
    MoveCandidate mc;
    mc.row = move.row; mc.col = move.col; mc.isHorizontal = move.horizontal;
    int len = 0; while (len < 15 && len < (int)move.word.size()) { mc.word[len] = move.word[len]; len++; } mc.word[len] = '\0';
    int actualScore = Mechanics::calculateTrueScore(mc, preMoveBoard, bonusBoard);

    // 3. UPDATE PARTICLES (With Caching)
    double totalWeight = 0.0;
    std::map<std::string, int> scoreCache; // <--- THE CACHE

    for (auto& p : particles) {
        // A. HARD FILTER
        std::vector<char> rackCopy = p.rack;
        bool possible = true;
        for (char t : tilesPlayed) {
            auto it = std::find(rackCopy.begin(), rackCopy.end(), t);
            if (it != rackCopy.end()) *it = ' ';
            else {
                auto bit = std::find(rackCopy.begin(), rackCopy.end(), '?');
                if (bit != rackCopy.end()) *bit = ' ';
                else { possible = false; break; }
            }
        }

        if (!possible) {
            p.weight = 0.0;
            continue;
        }

        // B. SOFT FILTER (Optimized)
        // Check cache first
        std::string key = getRackKey(p.rack);
        int bestPossible = 0;

        if (scoreCache.count(key)) {
            bestPossible = scoreCache[key];
        } else {
            // Expensive calculation happens only once per unique rack
            bestPossible = findBestPossibleScore(p.rack, preMoveBoard);
            scoreCache[key] = bestPossible;
        }

        // Calculate Rationality inline to use cached bestPossible
        double rationality = 0.0;
        if (actualScore <= bestPossible) {
            int delta = bestPossible - actualScore;
            if (delta == 0) rationality = 1.0;
            else if (delta < 5) rationality = 0.9;
            else if (delta < 15) rationality = 0.5;
            else if (delta < 30) rationality = 0.1;
            else rationality = 0.01;
        }

        p.weight *= rationality;
        totalWeight += p.weight;
    }

    {
        ScopedLogger log;
        std::cout << "[SPY] Filter Stats: " << totalWeight << " mass. Unique Racks Analyzed: " << scoreCache.size() << std::endl;
    }

    // 4. RESAMPLE
    if (totalWeight < 0.0001) {
        ScopedLogger log;
        std::cout << "[SPY] PANIC: Model collapsed. Resetting." << std::endl;
        initParticles();
    } else {
        resampleParticles(totalWeight);
    }

    // 5. TRANSITION
    for(auto& p : particles) {
        for(char t : tilesPlayed) {
            auto it = std::find(p.rack.begin(), p.rack.end(), t);
            if(it != p.rack.end()) p.rack.erase(it);
            else {
                auto bit = std::find(p.rack.begin(), p.rack.end(), '?');
                if(bit != p.rack.end()) p.rack.erase(bit);
            }
        }
    }
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
    // 1. Rebuild Unseen Pool
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

    {
        ScopedLogger log;
        std::cout << "[SPY] Ground Truth: " << unseenPool.size() << " tiles unseen." << std::endl;
    }

    // 2. REFILL PARTICLES
    static thread_local std::mt19937 rng(std::random_device{}());

    if(particles.empty()) { initParticles(); return; }

    for (auto& p : particles) {
        // Sanity check size
        if (p.rack.size() > 7) p.rack.resize(7);

        // CREATE A LOCAL COPY of available tiles for this specific particle refill
        // This is expensive but necessary for correctness.
        // Optimization: Only copy if we actually need to draw.
        if (p.rack.size() < 7 && !unseenPool.empty()) {
            std::vector<char> localPool = unseenPool;
            // Remove tiles that are ALREADY in this particle's rack
            // (Because unseenPool includes the opponent's current tiles)
            for(char existing : p.rack) {
                auto it = std::find(localPool.begin(), localPool.end(), existing);
                if(it != localPool.end()) localPool.erase(it);
            }

            // Now draw from the remaining valid pool
            std::shuffle(localPool.begin(), localPool.end(), rng);
            while(p.rack.size() < 7 && !localPool.empty()) {
                p.rack.push_back(localPool.back());
                localPool.pop_back();
            }
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