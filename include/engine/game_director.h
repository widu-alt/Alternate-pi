#pragma once
#include "state.h"
#include "referee.h"
#include "mechanics.h"
#include "../player_controller.h"
#include "../engine/dictionary.h"

#include <mutex>

#include <mutex>

struct MatchResult {
    int scoreP1;
    int scoreP2;
    int winner; // 0, 1, or -1 (Draw)
};

class GameDirector {
public:
    struct Config {
        bool verbose;
        bool allowChallenge;
        bool sixPassEndsGame;
        int delayMs;

        // Explicit Constructor to fix compiler error
        Config() : verbose(true), allowChallenge(true), sixPassEndsGame(true), delayMs(0) {}
    };

    GameDirector(PlayerController* p1, PlayerController* p2,
                 const Board& bonusBoard,
                 Config cfg = Config());

    MatchResult run(int gameId = 1);

private:
    PlayerController* controllers[2];
    Board bonusBoard;
    Config config;

    // Internal State
    GameState state;
    GameState snapshot;
    LastMoveInfo lastMove;
    bool canChallenge;

    // Core Logic
    void initGame();
    bool processTurn(int pIdx);
    void executePlay(int pIdx, Move& move);
    bool executeChallenge(int challengerIdx);
    void log(const std::string& msg);
};