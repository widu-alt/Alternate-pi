#pragma once

#include <vector>
#include <string>

#include "move_generator.h"
#include "../move.h"
#include "../../include/engine/board.h"
#include "../../include/engine/dictionary.h"
#include "../../include/engine/rack.h"
#include "../../include/engine/types.h"
#include "spy.h"
#include "profiler.h"

namespace spectre {

    class Vanguard {
    public:
        // Update: Now accepts 'const Spy&' instead of 'unseenBag'
        static MoveCandidate search(const LetterBoard &board,
                                    const Board &bonusBoard,
                                    const TileRack &rack,
                                    Spy &spy,
                                    Dictionary &dict,
                                    int timeLimitMs,
                                    int bagSize,
                                    int scoreDiff,
                                    OpponentType oppType);

    private:
        // Helper to play out a simulation
        static int playout(
            LetterBoard board,
            const Board& bonusBoard,
            int* myRackCounts,
            int* oppRackCounts,
            std::vector<char> bag, // passed by value to mutate copy
            bool myTurn,
            Dictionary& dict
        );

        // Helper to calculate score
        static int calculateScore(
            const LetterBoard& board,
            const Board& bonusBoard,
            const MoveCandidate& move
        );

        // Helper to apply move
        static int applyMove(
            LetterBoard& board,
            const MoveCandidate& move,
            int* rackCounts
        );

        // Helper to refill rack
        static void fillRack(int* rackCounts, std::vector<char>& bag);
    };

}