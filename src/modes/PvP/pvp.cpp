#include <iostream>
#include <string>
#include <sstream>

#include "../../../include/engine/board.h"
#include "../../../include/move.h"
#include "../../../include/engine/tiles.h"
#include "../../../include/engine/rack.h"
#include "../../../include/engine/dictionary.h"
#include "../../../include/choices.h"
#include "../../../include/modes/PvP/pvp.h"
#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/human_player.h"
#include "../../../include/modes/Home/home.h"
#include "../../../include/interface/renderer.h"
#include "../../../include/engine/mechanics.h"

using namespace std;

void runPvP() {
    // 1. Setup State
    GameState state;

    // Clear board and blanks directly in the state
    clearLetterBoard(state.board);
    clearBlankBoard(state.blanks);

    // Create and shuffle bag
    state.bag = createStandardTileBag();
    shuffleTileBag(state.bag);

    PlayerController* controllers[2];

    for (int i=0; i < 2; i++) {
        drawTiles(state.bag, state.players[i].rack, 7);
        state.players[i].score = 0;
        state.players[i].passCount = 0;
        controllers[i] = new HumanPlayer();
    }

    state.currentPlayerIndex = 0;

    GameState lastSnapShot = state;

    LastMoveInfo lastMove;
    lastMove.exists = false;
    lastMove.playerIndex = -1;
    bool canChallenge = false;

    // Bonus board is static
    Board bonusBoard = createBoard();

    Renderer::printTitle();
    cout << "Welcome to Terminal Crossword Game (2-player mode)\n";

    bool dictActive = true;
    if (!gDictionary.loadFromFile("csw24.txt")) {
        cout << "WARNING: Dictionary not loaded.\n";
        dictActive = false;
    }

    while (true) {
        int pIdx = state.currentPlayerIndex;
        Player& current = state.players[pIdx];
        Player& opponent = state.players[1 - pIdx];

        Renderer::printBoard(bonusBoard, state.board);
        cout << "Scores: Player 1 = " << state.players[0].score
             << " | Player 2 = " << state.players[1].score << endl;
        cout << "Player " << (pIdx + 1) << "'s Rack" << endl;
        Renderer::printRack(current.rack);

        if (handleSixPassEndGame(state)) break;

        // FIX: Update arguments to use 'state' members
        if (handleEmptyRackEndGame(state, bonusBoard, lastSnapShot, lastMove, canChallenge, controllers[pIdx])) {
            break;
        }

        // FIX: Update 'getMove' arguments
        Move move = controllers[pIdx]->getMove(bonusBoard,
                                               state.board,
                                               state.blanks,
                                               state.bag,
                                               current,
                                               opponent,
                                               pIdx + 1);

        // --- HANDLING MOVES ---

        if (move.type == MoveType::PASS) {
            passTurn(state, canChallenge, lastMove);
            state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            continue;
        }

        if (move.type == MoveType::QUIT) {
            if (handleQuit(state)) break;
            continue;
        }

        if (move.type == MoveType::CHALLENGE) {
            challengeMove(state, bonusBoard, lastSnapShot, lastMove, canChallenge);
            continue;
        }

        if (move.type == MoveType::EXCHANGE) {
            bool success = executeExchangeMove(state, move);
            if (success) {
                lastMove.exists = false;
                canChallenge = false;
                state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            }
            continue;
        }

        if (move.type == MoveType::PLAY) {
            MoveResult result = Referee::validateMove(state, move, bonusBoard, gDictionary);
            if (result.success) {

                Mechanics::commitSnapshot(lastSnapShot, state);
                Mechanics::applyMove(state, move, result.score);

                cout << "Move Successful! Points: " << result.score << endl;
                lastMove.exists = true;
                lastMove.playerIndex = pIdx;
                lastMove.startRow = move.row;
                lastMove.startCol = move.col;
                lastMove.horizontal = move.horizontal;
                lastMove.word = move.word;
                canChallenge = true;
                state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            } else {
                cout << "Invalid Move: " << result.message << endl;
            }
        }
    }

    delete controllers[0];
    delete controllers[1];
    Renderer::waitForQuitKey();
    Renderer::clearScreen();
}

























