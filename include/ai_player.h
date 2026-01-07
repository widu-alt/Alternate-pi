#pragma once

#include "engine/board.h"
#include "engine/rack.h"
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
    AIPlayer(AIStyle style);

    // FIX: Updated to match PlayerController's new signature
    Move getMove(const GameState& state,
                 const Board& bonusBoard,
                 const LastMoveInfo& lastMove,
                 bool canChallenge) override;

    // FIX: Updated name from 'Decision' to 'Response'
    Move getEndGameResponse(const GameState& state,
                            const LastMoveInfo& lastMove) override;

    // Hook for observing moves (Used by Cutie_Pi/Spy)
    // Note: This is not in PlayerController, it is specific to AIPlayer.
    void observeMove(const Move& move, const LetterBoard& board);

    std::string getName() const override;

private:
    AIStyle style;
    spectre::Spy spy;
    std::vector<spectre::MoveCandidate> candidates;

    // Internal Helpers
    void findAllMoves(const LetterBoard& letters, const TileRack& rack);
    int calculateStaticScore(const spectre::MoveCandidate& move,
                             const LetterBoard& letters,
                             const Board& bonusBoard);

    struct DifferentialMove { int row, col; std::string word; };
    DifferentialMove calculateDifferential(const LetterBoard& letters, const spectre::MoveCandidate& best);
    bool isRackBad(const TileRack& rack);
    std::string getTilesToExchange(const TileRack& rack);
};