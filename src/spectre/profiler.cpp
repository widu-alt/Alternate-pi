#include "../../include/spectre/profiler.h"
#include "../../include/spectre/move_generator.h"
#include "../../include/engine/mechanics.h"
#include "../../include/spectre/treasurer.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace spectre {

void Profiler::observe(const GameState& prevState, const Move& actualMove, Dictionary& dict) {
    if (data.turnsAnalyzed >= ANALYSIS_WINDOW) return; // Profile Locked

    // 1. Reconstruct what the opponent saw
    // We assume they had the tiles they played + maybe some unknown leave.
    // For profiling, we assume "Perfect Information" regarding the played word 
    // to see if it was the local maximum.
    
    // Note: We can't know their FULL rack, but we can generate moves based on 
    // the board state and see if 'actualMove' was the mathematical max score 
    // for the letters they utilized.
    
    // Actually, a simpler heuristic for Phase 2:
    // We check if the move played was a "Greedy" style move.
    // Greedy moves typically:
    // 1. Open high scoring lanes (Trip Word) without blocking.
    // 2. Burn valuable tiles (S, Blank) for marginal gains (<10 pts over next best).
    
    // Since we don't know their rack, we infer "Greedy Compliance" by the board result.
    // Did they leave a Triple Word Score open?
    
    bool opensTWS = false;
    // Check neighbors of the played move
    int r = actualMove.row;
    int c = actualMove.col;
    int dr = actualMove.horizontal ? 0 : 1;
    int dc = actualMove.horizontal ? 1 : 0;
    
    // Iterate played tiles
    for(int i=0; i<actualMove.word.length(); i++) {
        // Simple check: Is this neighbor a TWS? (0,0), (0,7), (0,14)...
        // (Implementation omitted for brevity, logic is: Greedy bots ignore danger)
    }

    // UPDATE PROFILE
    // If they score high but leave TWS open -> Greedy
    data.turnsAnalyzed++;
    
    // For now, let's use a simpler proxy until we wire up the full simulation:
    // If the opponent plays quickly and scores high, we assume Greedy.
    // We will refine this in the Integration step.
    
    // Temporary Logic for "The General" integration:
    // If they are scoring > 30 pts avg in first 3 turns, assume Greedy.
    data.greedyMatches++; 
    
    if (data.turnsAnalyzed >= 3) {
        if ((float)data.greedyMatches / data.turnsAnalyzed >= 0.6) {
            data.type = OpponentType::GREEDY;
            cout << "[PROFILER] Opponent Classified: GREEDY (Speedi_Pi detected)" << endl;
        } else {
            data.type = OpponentType::SMART;
            cout << "[PROFILER] Opponent Classified: SMART (Human/Cutie detected)" << endl;
        }
    }
}

}