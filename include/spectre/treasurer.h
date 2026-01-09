#pragma once

#include "../engine/types.h"
#include <vector>
#include <map>
#include <string>

namespace spectre {

    class Treasurer {
    public:
        // Main Valuation Function
        // Returns the Net Asset Value (Equity) of the leave
        // scoreDiff = (MyScore - OppScore). Used to calculate Gamma.
        static double evaluateEquity(const std::vector<Tile>& leave, int scoreDiff, int bagSize);

    private:
        // Financial Models
        static double getFundamentalValue(char tile, int bagSize);
        static double calculateSynergy(const std::vector<char>& tiles);
        static double calculateVolatility(const std::vector<char>& tiles);
        static double getGamma(int scoreDiff);
    };

}