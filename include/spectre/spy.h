#pragma once

#include "../engine/board.h"
#include "../../include/move.h"
#include "../../include/engine/rack.h"
#include "../engine/tiles.h"
#include <vector>
#include <random>

namespace spectre {

    // A single reality in the multiverse
    struct Particle {
        std::vector<char> rack; // The 7 tiles this version of the opponent holds
        double weight;          // Probability weight (0.0 to 1.0)
    };

    class Spy {
    public:
        Spy();

        // [UPDATE STEP]
        // The opponent moved. We kill particles that couldn't make this move,
        // and penalize particles that *could* have made a much better move.
        void observeOpponentMove(const Move& move, const LetterBoard& board);

        // [PREDICTION STEP]
        // Called before we search. We refill our particles to 7 tiles
        // using the remaining unseen pool.
        void updateGroundTruth(const LetterBoard& board, const TileRack& myRack, const TileBag& bag);

        // [OUTPUT]
        // Returns one likely rack by sampling the particle cloud.
        std::vector<char> generateWeightedRack() const;

    private:
        std::vector<char> unseenPool;
        std::vector<Particle> particles;
        const int PARTICLE_COUNT = 1000;

        // Helper: The "Mentalist" Logic
        // Returns a score 0.0-1.0 based on how "rational" the move was for this rack.
        double evaluateRationality(const std::vector<char>& rack, const Move& move, const LetterBoard& board);

        // Helper: Find best score for a specific rack (The Simulation)
        int findBestPossibleScore(const std::vector<char>& rack, const LetterBoard& board);

        // Helper: Manage the cloud
        void initParticles();
        void resampleParticles();
    };

}