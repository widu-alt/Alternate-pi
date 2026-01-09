#include "../../include/spectre/profiler.h"
#include <iostream>

using namespace std;

namespace spectre {

    // Helper: Is this cell a Triple Word Score?
    bool isTWS(int r, int c) {
        if (r == 0 || r == 7 || r == 14) {
            if (c == 0 || c == 7 || c == 14) return true;
        }
        return false;
    }

    void Profiler::observe(const Move& actualMove, const LetterBoard& board) {
        if (data.turnsAnalyzed >= ANALYSIS_WINDOW) return;

        // DETECT GREEDY BEHAVIOR:
        // Greedy bots often place tiles NEXT to a Triple Word Score (TWS)
        // without covering it, just to get a few extra points.

        bool leftTWSOpen = false;

        int r = actualMove.row;
        int c = actualMove.col;
        int dr = actualMove.horizontal ? 0 : 1;
        int dc = actualMove.horizontal ? 1 : 0;

        for(int i=0; i<actualMove.word.length(); i++) {
            // Check 4 neighbors
            int nr[] = {r+1, r-1, r, r};
            int nc[] = {c, c, c+1, c-1};

            for(int k=0; k<4; k++) {
                int checkR = nr[k];
                int checkC = nc[k];

                if (checkR >= 0 && checkR < 15 && checkC >= 0 && checkC < 15) {
                    // If neighbor is TWS and it is EMPTY
                    if (isTWS(checkR, checkC) && board[checkR][checkC] == ' ') {
                        leftTWSOpen = true;
                    }
                }
            }
            r += dr; c += dc;
        }

        data.turnsAnalyzed++;
        if (leftTWSOpen) {
            data.riskyMoves++;
            // Speedi_Pi almost ALWAYS leaves TWS open if it scores +1 point.
            // A smart human/bot would block it.
        }

        // Verdict Logic
        if (data.turnsAnalyzed >= 3) {
            if (data.riskyMoves > 0) {
                data.type = OpponentType::GREEDY;
                // cout << "[PROFILER] Opponent Classified: GREEDY" << endl;
            } else {
                data.type = OpponentType::SMART;
                // cout << "[PROFILER] Opponent Classified: SMART" << endl;
            }
        }
    }

}