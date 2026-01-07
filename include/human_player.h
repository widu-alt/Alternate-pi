#pragma once

#include "player_controller.h"

class HumanPlayer : public PlayerController {
public:
    // FIX: Updated signature to match PlayerController
    Move getMove(const GameState& state,
                 const Board &bonusBoard,
                 const LastMoveInfo& lastMove,
                 bool canChallenge) override;

    // FIX: Added missing override
    Move getEndGameResponse(const GameState& state,
                            const LastMoveInfo& lastMove) override;

    std::string getName() const override { return "Human"; }
};