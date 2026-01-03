#include "../../../include/modes/PvE/pve.h"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

#include "../../../include/board.h"
#include "../../../include/move.h"
#include "../../../include/tiles.h"
#include "../../../include/rack.h"
#include "../../../include/engine/dictionary.h"
#include "../../../include/choices.h"
#include "../../../include/human_player.h"
#include "../../../include/ai_player.h"
#include "../../../include/modes/Home/home.h"

// Engine
#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/interface/renderer.h"
#include "../../../include/engine/mechanics.h"

using namespace std;

void runPvE() {

    // 1. Initialize Unified State
    GameState state;
    clearLetterBoard(state.board);
    clearBlankBoard(state.blanks);
    state.bag = createStandardTileBag();
    shuffleTileBag(state.bag);

    Board bonusBoard = createBoard();

    // 2. Select Opponent
    int botChoice;
    cout << "\n=========================================\n";
    cout << "           SELECT OPPONENT\n";
    cout << "=========================================\n";
    cout << "1. Speedi_Pi (Fast, Static Heuristics)\n";
    cout << "2. Cutie_Pi  (Championship, S.P.E.C.T.R.E. Engine)\n";
    cout << "Choice: ";
    cin >> botChoice;

    AIStyle selectedStyle = (botChoice == 1) ? AIStyle::SPEEDI_PI : AIStyle::CUTIE_PI;

    // 3. Setup Players
    PlayerController* controllers[2];

    drawTiles(state.bag, state.players[0].rack, 7);
    state.players[0].score = 0;
    controllers[0] = new HumanPlayer();

    drawTiles(state.bag, state.players[1].rack, 7);
    state.players[1].score = 0;
    controllers[1] = new AIPlayer(selectedStyle);

    string botName = ((AIPlayer*)controllers[1])->getName();
    state.currentPlayerIndex = 0;

    // Legacy tracking
    GameState lastSnapShot = state;

    LastMoveInfo lastMove;
    lastMove.exists = false;
    lastMove.playerIndex = -1;
    bool canChallenge = false;

    bool dictActive = gDictionary.loadFromFile("csw24.txt");
    if (!dictActive) cout << "WARNING: Dictionary not loaded.\n";

    // Game loop
    while (true) {

        int pIdx = state.currentPlayerIndex;
        Player& current = state.players[pIdx];

        Renderer::printBoard(bonusBoard, state.board);
        cout << "Scores: You = " << state.players[0].score << " | " << botName << " ="<< state.players[1].score << "\n";
        if (state.currentPlayerIndex == 0) {
            cout << "Your Rack:" << endl;
            Renderer::printRack(state.players[0].rack);
        }

        if (handleSixPassEndGame(state)) break;

        if (handleEmptyRackEndGame(state,
                                    bonusBoard,
                                    lastSnapShot,
                                    lastMove,
                                    canChallenge,
                                    controllers[pIdx])) {
            break;
        }

        // Get Move
        Move move = controllers[pIdx]->getMove(bonusBoard, state.board, state.blanks, state.bag,
                                               current, state.players[1-pIdx], pIdx + 1);

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
            if (executeExchangeMove(state, move)) {
                lastMove.exists = false;
                canChallenge = false;
                state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            } else {
                 if (pIdx == 1) {
                    passTurn(state, canChallenge, lastMove);
                    state.currentPlayerIndex = 1 - state.currentPlayerIndex;
                 }
            }
            continue;
        }

        if (move.type == MoveType::PLAY) {
            MoveResult result = Referee::validateMove(state, move, bonusBoard, gDictionary);
            if (result.success) {
                // UPDATED: Use Mechanics
                Mechanics::commitSnapshot(lastSnapShot, state);
                Mechanics::applyMove(state, move, result.score);

                cout << "Move Valid! Score: " << result.score << endl;

                lastMove.exists = true;
                lastMove.playerIndex = pIdx;
                lastMove.startRow = move.row;
                lastMove.startCol = move.col;
                lastMove.horizontal = move.horizontal;
                lastMove.word = move.word;
                canChallenge = true;

                bool turnSwitched = false;
                if (pIdx == 0) {
                     AIPlayer* ai = dynamic_cast<AIPlayer*>(controllers[1]);
                     if (ai && ai->shouldChallenge(move, state.board)) {
                         // AI Challenges
                         challengeMove(state, bonusBoard, lastSnapShot, lastMove, canChallenge);
                         state.currentPlayerIndex = 1;
                         turnSwitched = true;
                     }
                }
                if (!turnSwitched) state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            } else {
                cout << "Invalid Move: " << result.message << endl;
                if (pIdx == 1) {
                     passTurn(state, canChallenge, lastMove);
                     state.currentPlayerIndex = 1 - state.currentPlayerIndex;
                }
            }
        }
    }
    delete controllers[0];
    delete controllers[1];
    Renderer::waitForQuitKey();
    Renderer::clearScreen();
}





















