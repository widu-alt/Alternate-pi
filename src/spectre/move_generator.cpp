#include "../../include/spectre/move_generator.h"
#include <algorithm>
#include <cstring>

using namespace std;

namespace spectre {

const int SEPERATOR = 26;

// Legacy Wrapper: Speedi_Pi needs Vectors and Full Data (Leaves)
// NOW SINGLE THREADED to support Parallel Game Simulation
vector<MoveCandidate> MoveGenerator::generate(const LetterBoard &board, const TileRack &rack, Dictionary &dict, bool useThreading) {

    // NOTE: 'useThreading' arg is ignored in favor of higher-level game parallelism.
    // In High-Throughput simulations, threading inner loops causes cache contention.

    vector<MoveCandidate> candidates;
    candidates.reserve(2000);

    // The Consumer fills the vector
    auto collectingConsumer = [&](MoveCandidate& cand, int* rackCounts) -> bool {
        // Calculate Leave (Speedi_Pi needs this!)
        int leaveIdx = 0;
        for(int i=0; i<26; i++) {
            int count = rackCounts[i];
            if (count > 0) {
                cand.leave[leaveIdx++] = (char)('A' + i);
                if (count > 1) cand.leave[leaveIdx++] = (char)('A' + i);
            }
        }
        for(int k=0; k<rackCounts[26]; k++) cand.leave[leaveIdx++] = '?';
        cand.leave[leaveIdx] = '\0';

        candidates.push_back(cand);
        return true; // Keep going
    };

    MoveGenerator::generate_custom(board, rack, dict, collectingConsumer);

    // Deduplication (Required for Legacy Speedi_Pi logic)
    if (!candidates.empty()) {
        sort(candidates.begin(), candidates.end(),
            [](const MoveCandidate& a, const MoveCandidate& b) {
                if (a.row != b.row) return a.row < b.row;
                if (a.col != b.col) return a.col < b.col;
                if (a.isHorizontal != b.isHorizontal) return a.isHorizontal;
                return strcmp(a.word, b.word) < 0;
        });
        auto last = unique(candidates.begin(), candidates.end(),
            [](const MoveCandidate& a, const MoveCandidate& b) {
                return a.row == b.row && a.col == b.col &&
                       a.isHorizontal == b.isHorizontal && strcmp(a.word, b.word) == 0;
        });
        candidates.erase(last, candidates.end());
    }

    return candidates;
}

}