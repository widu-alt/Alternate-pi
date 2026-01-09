#pragma once
#include <vector>
#include "../move.h"
#include "../../include/engine/board.h"
#include "../../include/engine/dictionary.h"

namespace spectre {

    enum class OpponentType {
        UNKNOWN,
        GREEDY,     // Plays for Max Score (Speedi_Pi)
        SMART,      // Plays for Score + Equity (Cutie_Pi)
        HUMAN       // Irrational / Unpredictable
    };

    struct ProfileData {
        int turnsAnalyzed = 0;
        int greedyMatches = 0; // Times they played the #1 Score
        int smartMatches = 0;  // Times they played the #1 Score+Equity
        OpponentType type = OpponentType::UNKNOWN;
    };

    class Profiler {
    public:
        void observe(const GameState& previousState, const Move& actualMove, Dictionary& dict);
        OpponentType getType() const { return data.type; }

        // reset for new game
        void reset() { data = ProfileData(); }

    private:
        ProfileData data;

        // Thresholds
        const int ANALYSIS_WINDOW = 5; // How many turns to watch
        const float GREEDY_THRESHOLD = 0.8f; // 80% match rate = Greedy
    };

}