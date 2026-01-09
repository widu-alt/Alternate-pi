#pragma once

#include "../engine/types.h"
#include <vector>
#include <map>
#include <string>

namespace spectre {

    // Forward declaration since we use references to it
    struct MoveCandidate;

    class Treasurer {
    public:
        // Main Valuation Function
        // Returns the Net Asset Value (Equity) of the leave
        // scoreDiff = (MyScore - OppScore). Used to calculate Gamma.
        static double evaluateEquity(const std::vector<Tile>& leave, int scoreDiff, int bagSize);

        // [LOBOTOMY ADDITIONS]
        // These must be declared to match the .cpp implementations, even if they just return true/0.
        static bool approve(const MoveCandidate& move, const std::vector<Tile>& currentRack, float riskAversion);
        static double getUtilityAdjustment(const MoveCandidate& move, const std::vector<Tile>& rack);

    private:
        // Financial Models
        static double getFundamentalValue(char tile, int bagSize);
        static double calculateSynergy(const std::vector<char>& tiles);
        static double calculateVolatility(const std::vector<char>& tiles);
        static double getGamma(int scoreDiff);
    };

}