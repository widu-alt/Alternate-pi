#pragma once

#include "board.h"
#include "rack.h"
#include "move.h"
#include "player_controller.h"
#include "spectre/move_generator.h"
#include "spectre/spy.h"
#include <vector>
#include <string>

// DEFINE THE BOTS
enum class AIStyle {
    SPEEDI_PI,  // Fast, Static Heuristics (The "Speedster")
    CUTIE_PI    // MCTS, Simulation (The "Champion")
};

class AIPlayer : public PlayerController {
public:
    // Default to Championship Mode (CUTIE_PI)
    AIPlayer(AIStyle style = AIStyle::CUTIE_PI);

    Move getMove(const Board &bonusBoard,
                 const LetterBoard &letters,
                 const BlankBoard &blankBoard,
                 const TileBag &bag,
                 const Player &me,
                 const Player &opponent,
                 int PlayerNum) override;

    Move getEndGameDecision() override;

    // NEW: Wire the brain to the game loop
    void observeMove(const Move& move, const LetterBoard& board) override;

    // Challenge Logic (Moved to PUBLIC so the Game Loop can ask the AI)
    bool shouldChallenge(const Move &opponentMove, const LetterBoard &board) const;

    // Legacy/Debug helper
    void findAllMoves(const LetterBoard &letters, const TileRack &rack);

    std::string getName() const;

private:
    AIStyle style;
    std::vector<spectre::MoveCandidate> candidates;

    // THE BRAIN: Persistent Spy instance (remembers opponent history)
    spectre::Spy spy;

    struct DifferentialMove {
        int row;
        int col;
        std::string word;
    };

    DifferentialMove calculateEngineMove(const LetterBoard &letters, const spectre::MoveCandidate &bestMove);
    bool isRackBad(const TileRack &rack);
    std::string getTilesToExchange(const TileRack &rack);
};