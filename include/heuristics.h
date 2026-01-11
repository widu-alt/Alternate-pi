#pragma once
#include <cctype>
#include <algorithm>
#include "tile_tracker.h"

namespace spectre {

    class Heuristics {
    public:
        // OPTIMIZATION: Public const arrays for Direct Access (No function call overhead)
        static constexpr int TILE_VALUES[26] = {
            1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10
        };

        static constexpr float LEAVE_VALUES[26] = {
            1.0f, -2.0f, -1.0f, 0.0f, 3.0f, -2.0f, -2.0f, 2.0f, 1.0f, -3.0f,
            -2.5f, 0.5f, -1.0f, 1.0f, -1.5f, -1.5f, -6.0f, 3.5f, 8.0f, 1.5f,
            -3.0f, -5.0f, -3.5f, -3.0f, -2.5f, -2.0f
        };

        // --- Fast Access Helpers ---

        // Unsafe: Assumes input is 'A'-'Z' (Use inside optimized loops)
        static inline int getValueFast(int charIdx0to25) {
            return TILE_VALUES[charIdx0to25];
        }

        // Safe Wrapper (Legacy support)
        static int getTileValue(char letter) {
            if (letter == '?') return 0;
            int idx = (letter & 31) - 1; // Fast toupper for ASCII letters
            if (idx >= 0 && idx < 26) return TILE_VALUES[idx];
            return 0;
        }

        static float getLeaveValue(char letter) {
            if (letter == '?') return 25.0f;
            int idx = (letter & 31) - 1;
            if (idx >= 0 && idx < 26) return LEAVE_VALUES[idx];
            return 0.0f;
        }

        static void updateWeights(const TileTracker& tracker) { (void)tracker; }
    };
}