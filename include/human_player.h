#pragma once

#include "player_controller.h"

class HumanPlayer : public PlayerController {
public:
    Move getMove(const Board &bonusBoard,
                 const LetterBoard &letters,
                 const BlankBoard &blankBoard,
                 const TileBag &bag,
                 const Player &me,
                 const Player &opponent,
                 int playerNum) override;

    Move getEndGameDecision() override;

private:
    Move handleRackLogic(TileRack &rack, TileBag &bag);
    Move parseMoveInput(const Board &bonusBoard,
                        const LetterBoard &letters,
                        const BlankBoard &blankBoard,
                        const TileRack &rack,
                        const TileBag &bag);
};