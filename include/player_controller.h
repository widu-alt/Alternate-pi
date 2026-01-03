#pragma once

#include "move.h"
#include "engine/board.h"
#include "engine/rack.h"
#include "engine/tiles.h"

class PlayerController {
public:
    virtual ~PlayerController() {}

    // Calculate and return the best move based on the current board state
    virtual Move getMove(const Board &bonusBoard,
                         const LetterBoard &letters,
                         const BlankBoard &blankBoard,
                         const TileBag &bag,
                         const Player &me,
                         const Player &opponent,
                         int PlayerNum) = 0;

    // Handle end-game specific decisions (like passing or clearing rack)
    virtual Move getEndGameDecision() = 0;

    // NEW: Allows the controller to observe the opponent's move.
    // This is required for the AI (Spy) to update its probability model based on
    // what the opponent played or exchanged.
    // We provide a default empty implementation so HumanPlayer doesn't need to override it.
    virtual void observeMove(const Move& move, const LetterBoard& board) {}
};