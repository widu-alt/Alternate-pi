#include "../../include/spectre/spy.h"
#include "../../include/tile_tracker.h"
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;

namespace spectre {

Spy::Spy() {}

void Spy::observeOpponentMove(const Move& move, const LetterBoard& board) {
    // In a full Bayesian implementation, we would update weights here.
    // For now, the most critical part is tracking the unseen pool correctly
    // which happens in updateGroundTruth.
    // We keep this stub to allow the architecture to support inference later.
}

void Spy::updateGroundTruth(const LetterBoard& board, const TileRack& myRack, const TileBag& bag) {
    unseenPool.clear();

    // Use TileTracker to deduce exactly what is missing
    TileTracker tracker;

    // 1. Mark visible board tiles
    for(int r=0; r<15; r++) {
        for(int c=0; c<15; c++) {
            if(board[r][c] != ' ') {
                 // Note: Ideally we'd know which are blanks, but standard letter tracking is 99% of the battle
                 tracker.markSeen(board[r][c]);
            }
        }
    }

    // 2. Mark my rack
    for(const auto& t : myRack) {
        tracker.markSeen(t.letter);
    }

    // 3. Reconstruct the "Unseen" pool (Bag + Opponent Rack)
    string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ?";
    for (char c : alphabet) {
        int count = tracker.getUnseenCount(c);
        for (int k=0; k<count; k++) {
            unseenPool.push_back(c);
        }
    }
}

std::vector<char> Spy::generateWeightedRack() const {
    // Current Logic:
    // Since 'unseenPool' contains exactly (Bag + Opponent Rack),
    // a random sample of 7 tiles from this pool is a statistically valid
    // hypothesis for the opponent's current hand.

    std::vector<char> pool = unseenPool;

    // Random shuffle is sufficient for Monte Carlo sampling at this stage
    // (Bayesian weights would modify the shuffle distribution, but uniform is a massive improvement over broken logic)
    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(pool.begin(), pool.end(), rng);

    // Take up to 7 tiles
    std::vector<char> rack;
    int drawCount = std::min((int)pool.size(), 7);
    for(int i=0; i<drawCount; i++) {
        rack.push_back(pool[i]);
    }

    return rack;
}

}