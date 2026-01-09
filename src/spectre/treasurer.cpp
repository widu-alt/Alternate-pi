#include "../../include/spectre/treasurer.h"
#include "../../include/spectre/vanguard.h" // [FIX] Required for MoveCandidate
#include "../../include/engine/types.h"     // [FIX] Required for Tile
#include <vector>
#include <cmath>

using namespace std;

namespace spectre {

    // PHASE 1 LOBOTOMY:
    // These functions are stripped of complex MPT math to stop the CPU starvation.
    // They return neutral/zero values to let the bot play purely based on score for now.

    // Header Signature: static double calculateVolatility(const std::vector<char>& tiles);
    double Treasurer::calculateVolatility(const std::vector<char>& tiles) {
        return 0.0;
    }

    // Header Signature: static double evaluateEquity(const std::vector<Tile>& leave, int scoreDiff, int bagSize);
    double Treasurer::evaluateEquity(const std::vector<Tile>& leave, int scoreDiff, int bagSize) {
        // Return 0.0 equity for now.
        // This effectively disables the "Paranoia Tax" until we implement Supersuperleaves.
        return 0.0;
    }

    // Header Signature: static bool approve(const MoveCandidate& move, const std::vector<Tile>& currentRack, float riskAversion);
    bool Treasurer::approve(const MoveCandidate& move, const std::vector<Tile>& currentRack, float riskAversion) {
        // Always approve. Stop Hard Pruning.
        return true;
    }

    // Header Signature: static double getUtilityAdjustment(const MoveCandidate& move, const std::vector<Tile>& rack);
    double Treasurer::getUtilityAdjustment(const MoveCandidate& move, const std::vector<Tile>& rack) {
        return 0.0;
    }

    // Internal helpers (getGamma, getFundamentalValue) are no longer needed
    // since evaluateEquity is stubbed out.

}