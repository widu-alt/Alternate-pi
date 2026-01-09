#include "../../include/spectre/treasurer.h"
#include <numeric>
#include <cmath>

using namespace std;

namespace spectre {

    // PHASE 1 LOBOTOMY: Removed Real-Time MPT Covariance Calculation.
    // The "Scrabble Bot Logic Analysis" report confirmed this was a Denial of Service
    // on our own search engine.
    // TODO: Replace with "Supersuperleaves" (Static Lookup Table) in Phase 4.

    float Treasurer::calculateVolatility(const std::vector<Tile>& rack, const std::vector<Tile>& bag) {
        // Placeholder: Return 0 volatility until we have the static tables.
        // This stops the CPU starvation.
        return 0.0f;
    }

    bool Treasurer::approve(const MoveCandidate& move, const std::vector<Tile>& currentRack, float riskAversion) {
        // PHASE 1 LOBOTOMY: STOP HARD PRUNING.
        // Always approve the move. Let Vanguard deciding based on scores, not financial theory.
        return true;
    }

    float Treasurer::getUtilityAdjustment(const MoveCandidate& move, const std::vector<Tile>& rack) {
        // Temporary: Return 0.
        // In Phase 4, this will return the "Supersuperleaf" value (Equity + RiskAdjustment).
        return 0.0f;
    }

}