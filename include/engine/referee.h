#pragma once

#pragma once
#include "types.h"
#include "state.h"
#include "../dictionary.h"

class Referee {
public:

    // The core function: Input State + Move -> Output Result
    static MoveResult validateMove(const GameState &state, const Move &move, const Board &bonusBoard, Dictionary &dict);

    // Helpers
    static int calculateScore(const LetterBoard &letters, const Board &bonuses, const Move &move);
    static bool checkConnectivity(const LetterBoard &letters, const Move &move);
};