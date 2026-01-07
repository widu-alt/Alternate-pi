#pragma once
#include "state.h"
#include "referee.h"
#include "mechanics.h"
#include "../player_controller.h"
#include "../engine/dictionary.h"

#include <mutex>

struct MatchResult {
    int scoreP1;
    int scoreP2;
    int winner; // 0, 1, or -1 (Draw)
};

class GameDirector {
public:
    struct Config {
        bool verbose = true;
        bool allowChallenge = true;
        bool sixPassEndsGame = true;
        int delayMs = 0;
    };

    GameDirector(PlayerController* p1, PlayerController* p2,
                 const Board& bonusBoard,
                 Config cfg = Config() );

    MatchResult run(int gameId = 1);

private:
    PlayerController* controllers[2];
    Board bonusBoard;
    Config config;

    // Internal State
    GameState state;
    GameState snapshot; // State BEFORE the last move (for reverting)
    LastMoveInfo lastMove;
    bool canChallenge;

    // Core Logic
    void initGame();
    bool processTurn(int pIdx);
    void executePlay(int pIdx, Move& move);
    bool executeChallenge(int challengerIdx); // Returns true if challenge succeeded
    void log(const std::string& msg);
};