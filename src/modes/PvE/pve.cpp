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
#include "../../../include/dict.h"
#include "../../../include/choices.h"
#include "../../../include/human_player.h"
#include "../../../include/ai_player.h"
#include "../../../include/modes/Home/home.h"

using namespace std;

void runPvE() {

    // 1. Setup Board
    Board bonusBoard = createBoard();
    LetterBoard letters;
    clearLetterBoard(letters);
    BlankBoard blanks;
    clearBlankBoard(blanks);

    // 2. Setup Bag
    TileBag bag = createStandardTileBag();
    shuffleTileBag(bag);

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
    Player players[2];
    PlayerController* controllers[2];

    // Player 1 (Human)
    drawTiles(bag, players[0].rack, 7);
    players[0].score = 0;
    players[0].passCount = 0;
    controllers[0] = new HumanPlayer();

    // Player 2 (AI)
    drawTiles(bag, players[1].rack, 7);
    players[1].score = 0;
    players[1].passCount = 0;
    controllers[1] = new AIPlayer(selectedStyle);

    string botName = ((AIPlayer*)controllers[1])->getName();

    int currentPlayer = 0; // 0 -> Human, 1 -> AI
    GameSnapshot lastSnapShot;
    LastMoveInfo lastMove;
    lastMove.exists = false;
    lastMove.playerIndex = -1;

    bool canChallenge = false;
    bool dictActive = true;

    if (!loadDictionary("csw24.txt")) {
        cout << "WARNING: Dictionary not loaded, AI might misbehave.\n";
        dictActive = false;
    }

    printTitle();
    cout << "\nStarting Match: You vs " << botName << "\n\n";
    cout << "Good luck, hooman.\n\n";

    printBoard(bonusBoard, letters);
    cout << "Scores: You = " << players[0].score << " | " << botName << " ="<< players[1].score << "\n";

    // Only show rack in human's turn
    if (currentPlayer == 0) {
        cout << "Your Rack:" << endl;
        printRack(players[currentPlayer].rack);
    }

    // Game loop
    while (true) {
        if (handleSixPassEndGame(players)) {
            break;
        }

        if (handleEmptyRackEndGame(bonusBoard,
                                   letters,
                                   blanks,
                                   bag,
                                   players,
                                   lastSnapShot,
                                   lastMove,
                                   currentPlayer,
                                   canChallenge,
                                   dictActive,
                                   controllers[currentPlayer])) {
            break;
        }

        // Get Move  (Human input)
        Move move = controllers[currentPlayer]->getMove(bonusBoard,
                                                        letters,
                                                        blanks,
                                                        bag,
                                                        players[currentPlayer], // Me
                                                        players[1 - currentPlayer], // Opponent
                                                        currentPlayer + 1);

        // move execution
        if (move.type == MoveType::PASS) {
            passTurn(players, currentPlayer, canChallenge, lastMove);
            //printBoard(bonusBoard, letters);
            //cout << "Scores: You = " << players[0].score << " | Cutie_Pi = " << players[1].score << endl;

            if (currentPlayer == 0) {
                cout << "\nYour Turn:" << endl;
                printRack(players[currentPlayer].rack);
            }
            continue;
        }

        if (move.type == MoveType::QUIT) {
            // Only Human can quit via menu
            if (handleQuit(players, currentPlayer)) {
                break;
            }
            continue;
        }

        if (move.type == MoveType::CHALLENGE) {
            // Human is challenging the AI's previous move
            challengeMove(bonusBoard, letters, blanks, bag, players,
                          lastSnapShot, lastMove, currentPlayer, canChallenge, dictActive);

            // VISUAL UPDATE: Reprint the board to show if tiles were removed or stayed
            cout << "\n--- Board Status After Challenge ---\n";
            printBoard(bonusBoard, letters);
            cout << "Scores: You = " << players[0].score << " | " << botName << " ="<< players[1].score << endl;

            // If it's still Human's turn (Successful challenge = Opponent lost turn, Human plays), show rack
            if (currentPlayer == 0) {
                cout << "\nYour Turn:" << endl;
                printRack(players[currentPlayer].rack);
            }
            continue;
        }

        if (move.type == MoveType::EXCHANGE) {
            bool success = executeExchangeMove(bag, players[currentPlayer], move);
            if (success) {
                lastMove.exists = false;
                canChallenge = false;

                players[currentPlayer].passCount++;

                printBoard(bonusBoard, letters);
                cout << "Scores: You = " << players[0].score << " | " << botName << " ="<< players[1].score << endl;

                currentPlayer = 1 - currentPlayer;
                if (currentPlayer == 0) {
                    cout << "\nYour Turn:" << endl;
                    printRack(players[currentPlayer].rack);
                }
            } else {
                if (currentPlayer == 1) {
                    cout << "[AI] Exchange failed (Bag empty?). Forcing PASS.\n";
                    passTurn(players, currentPlayer, canChallenge, lastMove);

                    // Force switch to Human
                    currentPlayer = 0;
                    cout << "\nYour Turn:" << endl;
                    printRack(players[currentPlayer].rack);
                } else {
                    cout << "Invalid exchange. Try again.\n";
                }
            }
            continue;
        }

        if (move.type == MoveType::PLAY) {
            bool success = executePlayMove(bonusBoard, letters, blanks, bag, players,
                                           players[currentPlayer], move, lastSnapShot);

            if (success) {
                lastMove.exists = true;
                lastMove.playerIndex = currentPlayer;
                lastMove.startRow = move.row;
                lastMove.startCol = move.col;
                lastMove.horizontal = move.horizontal;
                canChallenge = true;

                bool turnSwitched = false;
                if (currentPlayer == 0) { // Human just played
                    AIPlayer* ai = dynamic_cast<AIPlayer*>(controllers[1]);
                    // checking if AI is looking to challenge the move
                    if (ai && ai->shouldChallenge(move, letters)) {
                        challengePhrase();
                        cout << "\n\n";
                        this_thread::sleep_for(chrono::milliseconds(2500)); // dramatic pause
                        printBoard(bonusBoard, letters);

                        // Execute challenge as AI
                        int challengerIndex = 1;
                        challengeMove(bonusBoard, letters, blanks, bag, players,
                                      lastSnapShot, lastMove, challengerIndex, canChallenge, dictActive);
                        printBoard(bonusBoard, letters);
                        currentPlayer = 1;
                        turnSwitched = true;
                    }
                }

                if (!turnSwitched) {
                    printBoard(bonusBoard, letters);
                    cout << "Scores: You = " << players[0].score << " | " << botName << " ="<< players[1].score << endl;
                    currentPlayer = 1 - currentPlayer;
                }

                if (currentPlayer == 0) {
                    //cout << "\nYour Turn:" << endl;
                    printRack(players[currentPlayer].rack);
                }
            } else {
                if (currentPlayer == 1) {
                    cout << "[Critical Error] AI played an invalid move. Force passing to prevent an infinity loop\n";
                    passTurn(players, currentPlayer, canChallenge, lastMove);
                    if (currentPlayer == 0) {
                        //cout << "\nYour Turn:" << endl;
                        printRack(players[currentPlayer].rack);
                    }
                } else {
                    // Human can retry
                    cout << "Invalid move. Try again.\n";
                }
            }
        }
    }

    // Clean up
    delete controllers[0];
    delete controllers[1];
    waitForQuitKey();
    clearScreen();
}





















