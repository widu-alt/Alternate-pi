#include "../include/player_controller.h"
#include "../include/human_player.h"
#include "../include/choices.h"
#include "../include/engine/referee.h"
#include "../include/engine/state.h"
#include "../include/interface/renderer.h"
#include <iostream>
#include <limits>
#include <algorithm>

using namespace std;

Move HumanPlayer::getMove(const GameState& state,
                          const Board &bonusBoard,
                          const LastMoveInfo& lastMove,
                          bool canChallenge)
{
    // NO extra prompts. Just call the existing parser.
    return parseMoveInput(bonusBoard, state.board, state.blanks, state.players[state.currentPlayerIndex].rack, state.bag);
}

Move HumanPlayer::getEndGameResponse(const GameState& state, const LastMoveInfo& lastMove) {
    cout << "\n!!! OPPONENT EMPTIED RACK !!!\n";
    cout << "1. CHALLENGE\n";
    cout << "2. PASS\n";
    cout << "Selection: ";

    int c;
    if (!(cin >> c)) { cin.clear(); cin.ignore(); c=2; }

    if (c == 1) return Move(MoveType::CHALLENGE);
    return Move(MoveType::PASS);
}