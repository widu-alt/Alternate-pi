#pragma once

#include "../../include/board.h"
#include "../../include/move.h"
#include "../../include/rack.h"
#include "../../include/tiles.h"
#include <vector>

namespace spectre {

    class Spy {
    public:
        Spy();

        // Called when the opponent makes a move.
        // Updates internal probability weights based on what they played.
        void observeOpponentMove(const Move& move, const LetterBoard& board);

        // Called before the AI searches.
        // Re-calculates the "Unseen Pool" (The bag + the opponent's rack).
        void updateGroundTruth(const LetterBoard& board, const TileRack& myRack, const TileBag& bag);

        // Generates a hypothetical opponent rack based on current beliefs.
        // Used by Vanguard during simulations.
        std::vector<char> generateWeightedRack() const;

    private:
        std::vector<char> unseenPool; // Contains every tile currently not on board or my rack
    };

}