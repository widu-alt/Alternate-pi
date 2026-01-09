#include "../../include/spectre/treasurer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

namespace spectre {

double Treasurer::evaluateEquity(const std::vector<Tile>& leave, int scoreDiff, int bagSize) {
    if (leave.empty()) return 0.0; // Empty rack has 0 intrinsic value (though going out has utility)

    std::vector<char> tiles;
    for(const auto& t : leave) tiles.push_back(t.letter);

    // 1. FUNDAMENTAL ANALYSIS (Sum of Asset Values)
    double fundamentals = 0.0;
    for (char c : tiles) {
        fundamentals += getFundamentalValue(c, bagSize);
    }

    // 2. COVARIANCE / SYNERGY (Hedging & Cannibalization)
    double synergy = calculateSynergy(tiles);

    // 3. RISK ASSESSMENT (Gamma Protocol)
    double volatility = calculateVolatility(tiles);
    double gamma = getGamma(scoreDiff);

    // 4. NET ASSET VALUE CALCULATION
    // Formula: Equity = (Fundamentals + Synergy) - (Risk_Aversion * Volatility)

    // Logic Check:
    // If Gamma is POSITIVE (Leading), we penalize volatility (Pension Fund).
    // If Gamma is NEGATIVE (Trailing), we bonus volatility (Hedge Fund).

    double equity = (fundamentals + synergy) - (gamma * volatility);

    return equity;
}

double Treasurer::getFundamentalValue(char tile, int bagSize) {
    // ASSET CLASSES based on "Scrabble Rack as Financial Portfolio"

    // 1. DERIVATIVES (The Blank)
    // Modeled as a Call Option with Theta Decay.
    if (tile == '?') {
        if (bagSize > 40) return 35.0; // High Volatility Phase
        if (bagSize > 10) return 25.0; // Stable Phase
        return 10.0; // Expiry Phase (Theta decay destroys value)
    }

    // 2. BLUE CHIP EQUITY (Liquidity Providers)
    if (tile == 'S') return 10.0;

    // 3. TREASURY BONDS (Solvency/Turnover)
    // E, R, I, N, A, T -> Safe, low yield
    if (string("ERINAT").find(tile) != string::npos) return 4.0;
    if (string("DL").find(tile) != string::npos) return 2.0;

    // 4. HIGH YIELD BONDS (Speculative)
    // Z, X, J, K -> High potential, illiquid
    if (string("ZXJK").find(tile) != string::npos) return -3.0; // Holding cost is negative unless played

    // 5. TOXIC ASSETS (Liabilities)
    // Q, V, U, W -> Negative intrinsic value
    if (tile == 'Q') return -12.0; // Massive liability
    if (tile == 'V') return -5.0;
    if (tile == 'U') return -4.0;
    if (tile == 'W') return -3.0;
    if (tile == 'G') return -3.0;

    // Standard Commodities
    return 1.0;
}

double Treasurer::calculateSynergy(const std::vector<char>& tiles) {
    double synergy = 0.0;
    string rackStr = "";
    for(char c : tiles) rackStr += c;
    sort(rackStr.begin(), rackStr.end());

    // A. HEDGING (Neutralizing Toxic Assets)
    bool hasQ = (rackStr.find('Q') != string::npos);
    bool hasU = (rackStr.find('U') != string::npos);

    if (hasQ && hasU) {
        synergy += 8.0; // The 'U' insures the 'Q'. Liability (-12) becomes Bond (-4 + 8 = +4).
    }

    // B. CORRELATION (Positive Synergy)
    if (rackStr.find("ING") != string::npos) synergy += 12.0;
    else if (rackStr.find("IN") != string::npos) synergy += 4.0;

    if (rackStr.find("ER") != string::npos) synergy += 6.0;
    if (rackStr.find("EST") != string::npos) synergy += 6.0;

    // C. CANNIBALIZATION (Negative Synergy / Duplicate Penalty)
    // "Sector Concentration Risk"
    map<char, int> counts;
    for(char c : tiles) counts[c]++;

    for(auto const& [key, val] : counts) {
        if (val > 1) {
            // Penalty scales exponentially with duplication: 2->-3, 3->-9
            if (key == 'E' || key == 'A' || key == 'I') {
                synergy -= pow(val, 2.0);
            } else {
                synergy -= (pow(val, 2.5)); // Heavy penalty for duplicate consonants
            }
        }
    }

    // Balance Penalty (Vowel/Consonant Ratio)
    int vowels = 0;
    for(char c : tiles) if(string("AEIOU").find(c) != string::npos) vowels++;
    int consonants = tiles.size() - vowels;

    if (vowels == 0 && consonants > 0) synergy -= 5.0;
    if (consonants == 0 && vowels > 0) synergy -= 5.0;

    return synergy;
}

double Treasurer::calculateVolatility(const std::vector<char>& tiles) {
    // Measures the "Risk" of the rack.
    // High Volatility = High Dispersion of outcomes (Bingo or Zero).
    // Low Volatility = Consistent Scoring.

    double vol = 0.0;
    for(char c : tiles) {
        if (c == '?') vol += 10.0; // Infinite Variance
        if (string("QZJX").find(c) != string::npos) vol += 8.0; // Binary outcomes
    }
    return vol;
}

double Treasurer::getGamma(int scoreDiff) {
    // DYNAMIC RISK AVERSION

    // Scenario A: Leading (> 60 pts) -> Pension Fund
    // We want to MINIMIZE volatility.
    // High positive Gamma penalizes risky tiles.
    if (scoreDiff > 60) return 1.5;
    if (scoreDiff > 30) return 0.8;

    // Scenario B: Trailing (> 60 pts) -> Distressed Hedge Fund
    // We want to MAXIMIZE volatility.
    // Negative Gamma turns Risk (Volatility) into a Bonus.
    if (scoreDiff < -60) return -1.2;
    if (scoreDiff < -30) return -0.5;

    // Scenario C: Competitive -> Balanced Fund
    return 0.1; // Slight preference for stability
}

}