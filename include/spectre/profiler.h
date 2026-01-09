#pragma once
#include <vector>
#include "../move.h"
#include "../../include/engine/board.h"
#include "../../include/engine/dictionary.h"

namespace spectre {

    enum class OpponentType {
        UNKNOWN,
        GREEDY,     // Speedi_Pi (Max Score, ignores danger)
        SMART,      // Cutie_Pi / Human (Balanced)
        HUMAN
    };

    struct ProfileData {
        int turnsAnalyzed = 0;
        int riskyMoves = 0;   // Times they left a TWS open
        OpponentType type = OpponentType::UNKNOWN;
    };

    class Profiler {
    public:
        void observe(const Move& actualMove, const LetterBoard& board);
        OpponentType getType() const { return data.type; }
        void reset() { data = ProfileData(); }

    private:
        ProfileData data;
        const int ANALYSIS_WINDOW = 5;
    };

}