#pragma once

#include <vector>
#include <array>
#include <chrono>

#include "move_generator.h"
#include "../../include/board.h"
#include "../../include/rack.h"
#include "../../include/move.h"
#include "../../include/dictionary.h"

namespace spectre {

class Judge {
public:
    /**
     * @brief THE EXECUTIONER.
     * Solves the Scrabble Endgame using Minimax with Alpha-Beta Pruning.
     * * @param board The current state of the letter board.
     * @param bonusBoard The static bonus board (DL, TL, DW, TW).
     * @param myRack The AI's current rack.
     * @param oppRack The Opponent's inferred rack (Perfect Info).
     * @param dict The GADDAG dictionary.
     * @return Move The move that maximizes (MyScore - OppScore) to the end of the game.
     */
    static Move solveEndgame(const LetterBoard& board,
                             const Board& bonusBoard,
                             const TileRack& myRack,
                             const TileRack& oppRack,
                             Dictionary& dict);

private:
    /**
     * @brief Recursive Minimax Driver.
     * * @param board Copy of board state for simulation.
     * @param myRackCounts Histogram of AI's tiles.
     * @param oppRackCounts Histogram of Opponent's tiles.
     * @param alpha Best score for Maximizer (Lower Bound).
     * @param beta Best score for Minimizer (Upper Bound).
     * @param maximizingPlayer True if it's AI's turn, False if Opponent's.
     * @param passesInARow Detection for game-over via passing.
     * @return int The final Score Differential.
     */
    static int minimax(LetterBoard board,
                   const Board& bonusBoard,
                   int* myRackCounts,
                   int* oppRackCounts,
                   Dictionary& dict,
                   int alpha,
                   int beta,
                   bool maximizingPlayer,
                   int passesInARow,
                   int depth,
                   const std::chrono::steady_clock::time_point& startTime,
                   int timeBudgetMs);

    // Precise Scoring Engine (Internal)
    static int calculateMoveScore(const LetterBoard& board,
                                  const Board& bonusBoard,
                                  const MoveCandidate& move);

    // State Managment
    static void applyMove(LetterBoard& board,
                          const MoveCandidate& move,
                          int* rackCounts);
};

}