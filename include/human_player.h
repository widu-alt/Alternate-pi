#pragma once

#include "player_controller.h"

class HumanPlayer : public PlayerController {
public:
    // Standard Interface
    Move getMove(const GameState& state,
                 const Board& bonusBoard,
                 const LastMoveInfo& lastMove,
                 bool canChallenge) override;

    Move getEndGameResponse(const GameState& state,
                            const LastMoveInfo& lastMove) override;

    std::string getName() const override { return "Human"; }

private:
    // Your specific helpers
    Move handleRackLogic(TileRack &rack, TileBag &bag);

    Move parseMoveInput(const Board &bonusBoard,
                        const GameState &state, // Passed state for Preview/Validation
                        const LetterBoard &letters,
                        const BlankBoard &blankBoard,
                        const TileRack &rack,
                        const TileBag &bag);
};