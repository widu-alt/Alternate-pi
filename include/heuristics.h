#pragma once
#include <cctype>
#include <algorithm>
#include "tile_tracker.h"

namespace spectre {

    class Heuristics {
    public:
        // [USED BY SPEED_PI & GAME ENGINE]
        static int getTileValue(char letter) {
            static const int values[] = {
                1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10, 0
            };
            if (letter == '?') return 0;
            int idx = toupper(letter) - 'A';
            if (idx >= 0 && idx < 26) return values[idx];
            return 0;
        }

        // [USED BY VANGUARD] - Quackle/Maven Static Leave Values
        static float getLeaveValue(char letter) {
            static const float leaveValues[] = {
                1.0f, // A
               -2.0f, // B
               -1.0f, // C
                0.0f, // D
                3.0f, // E
               -2.0f, // F
               -2.0f, // G
                2.0f, // H
                1.0f, // I
               -3.0f, // J
               -2.5f, // K
                0.5f, // L
               -1.0f, // M
                1.0f, // N
               -1.5f, // O
               -1.5f, // P
               -6.0f, // Q
                3.5f, // R
                8.0f, // S
                1.5f, // T
               -3.0f, // U
               -5.0f, // V
               -3.5f, // W
               -3.0f, // X
               -2.5f, // Y
               -2.0f, // Z
               25.0f  // ?
           };

            if (letter == '?') return 25.0f;
            int idx = toupper(letter) - 'A';
            if (idx >= 0 && idx < 26) return leaveValues[idx];
            return 0.0f;
        }

        // [LEGACY SUPPORT] - Stub for AIPlayer to prevent build errors
        static void updateWeights(const TileTracker& tracker) {
            // In a full implementation, this would adjust dynamic weights
            // based on the remaining tiles in 'tracker'.
            // For now, we keep it empty to allow compilation.
            (void)tracker;
        }
    };

}