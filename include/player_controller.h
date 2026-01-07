#pragma once

#include "move.h"
#include "engine/board.h"
#include "engine/rack.h"
#include "engine/state.h"
#include "engine/tiles.h"

class PlayerController {
public:
    virtual ~PlayerController() = default;

    // Standard Move Generation
    // 'lastMove' contains context (words formed, did opponent empty rack?)
    // 'canChallenge' tells the bot if the rules allow a challenge right now
    virtual Move getMove(const GameState& state,
                         const Board& bonusBoard,
                         const LastMoveInfo& lastMove,
                         bool canChallenge) = 0;

    // Explicit hook for the "Opponent Emptied Rack" scenario.
    // Must return MoveType::PASS (Accept loss/end) or MoveType::CHALLENGE (Fight it)
    virtual Move getEndGameResponse(const GameState& state,
                                    const LastMoveInfo& lastMove) = 0;

    virtual std::string getName() const { return "Player"; }
};