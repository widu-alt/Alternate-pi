#include <iostream>
#include <limits>
#include <string>

#include "../include/board.h"
#include "../include/move.h"
#include "../include/tiles.h"
#include "../include/rack.h"
#include "../include/dict.h"
#include "../include/choices.h"

using namespace std;

// Show unseen tiles
void showUnseenTiles(const TileBag &bag, const Player players[2], int currentPlayer) {

    int opponent = 1 - currentPlayer;
    bool revealOpponent = (static_cast<int>(bag.size()) <= 7);

    printTileBag(bag, players[opponent].rack, revealOpponent);
}

bool executePlayMove(GameState& state,
                     const Move &move,
                     const Board &bonusBoard) {

    MoveResult result = Referee::validateMove(state, move, bonusBoard, gDawg);

    if (result.success) {
        // 2. Act (The Executioner)
        cout << "Move Valid! Score: " << result.score << endl;
        applyMoveToState(state, move, result.score);
        return true;
    } else {
        cout << "Invalid Move: " << result.message << endl;
        return false;
    }
}

bool executeExchangeMove(TileBag &bag, Player &player, const Move &move) {
    bool ok = exchangeRack(player.rack, move.exchangeLetters, bag) ;

    if (ok) {
        cout << "Exchange successful. Passing the turn. \n";
        return true;
    } else {
        cout << "Exchange failed (Not enough tiles).\n";
        return false;
    }
}



bool handleSixPassEndGame(Player players[2]) {

    if (players[0].passCount < 3 || players[1].passCount < 3) {
        return false;
    }

    cout << "\nSix consecutive scoreless turns by both players\n";

    int rackScore[2] = {0, 0};

    for (int i = 0; i < 2; i++) {
        for (auto &tile: players[i].rack) {
            rackScore[i] += tile.points;
        }
    }
    players[0].score -= rackScore[0] * 2;
    players[1].score -= rackScore[1] * 2;

    cout << "\nGame Over.\n";
    cout << "Final Scores:\n";
    cout << "Player 1: " << players[0].score << endl;
    cout << "Player 2: " << players[1].score << endl;

    if (players[0].score > players[1].score) {
        cout << "Player 1 wins!\n";
    } else if (players[1].score > players[0].score) {
        cout << "Player 2 wins!\n";
    } else {
        cout << "Match is a tie!\n";
    }

    return true;
}

bool handleEmptyRackEndGame(Board &bonusBoard,
                            LetterBoard &letters,
                            BlankBoard &blanks,
                            TileBag &bag,
                            Player players[2],
                            GameSnapshot &lastSnapShot,
                            LastMoveInfo &lastMove,
                            int &currentPlayer,
                            bool &canChallenge,
                            bool &dictActive,
                            PlayerController* controller) {

    // Only trigger this move if there WAS a last move AND that player now has an EMPTY rack.

    if (!(lastMove.exists && lastMove.playerIndex != -1 &&
            players[lastMove.playerIndex].rack.empty())) {
        return false;
    }

    int emptiedPlayer = lastMove.playerIndex; // player who finished all the tiles.

    Move decision = controller->getEndGameDecision();

    if (decision.type == MoveType::PASS) {

        cout << "Player " << (currentPlayer + 1) << " passes their turn." << endl;

        int other = 1 - emptiedPlayer;
        int rackPoints = 0;

        for (const auto &t : players[other].rack) {
            rackPoints += t.points * 2;
        }

        players[emptiedPlayer].score += rackPoints;

        cout << "\nGame Over.\n";
        cout << "Final Scores:\n";
        cout << "Player 1: " << players[0].score << endl;
        cout << "Player 2: " << players[1].score << endl;

        if (players[0].score > players[1].score) {
            cout << "Player 1 wins!\n";
        } else if (players[1].score > players[0].score) {
            cout << "Player 2 wins!\n";
        } else {
            cout << "Match is a tie!\n";
        }

        //game over
        return true;
    }

    if (decision.type == MoveType::CHALLENGE) {
        challengeMove(bonusBoard,
                        letters,
                        blanks,
                        bag,
                        players,
                        lastSnapShot,
                        lastMove,
                        currentPlayer,
                        canChallenge,
                        dictActive);

        if (bag.empty() && players[emptiedPlayer].rack.empty()) {
            // Challenge failed, still in Endgame.
            int other = 1 - emptiedPlayer;
            int rackPoints = 0;

            for (const auto &t : players[other].rack) {
                rackPoints += t.points * 2;
            }

            players[emptiedPlayer].score += rackPoints;

            cout << "\nGame Over.\n";
            cout << "Final Scores:\n";
            cout << "Player 1: " << players[0].score << endl;
            cout << "Player 2: " << players[1].score << endl;

            if (players[0].score > players[1].score) {
                cout << "Player 1 wins!\n";
            } else if (players[1].score > players[0].score) {
                cout << "Player 2 wins!\n";
            } else {
                cout << "Match is a tie!\n";
            }

            //game over
            return true;
        }

        // Challenge succeeded and game continues.
        return false;
    }

    return false;
}

void passTurn(Player players[2], int &currentPlayer, bool &canChallenge, LastMoveInfo &lastMove) {

    cout << "Player " << (currentPlayer + 1) << " passes their turn." << endl;

    players[currentPlayer].passCount += 1;

    // After a pass, can no longer challenge the previous word.
    canChallenge = false;
    lastMove.exists = false;

    currentPlayer = 1 - currentPlayer;
}

void showTileSet(const TileBag &bag, const Player players[2], int currentPlayer) {

    int opponent = 1 - currentPlayer;
    bool revealOpponent = (static_cast<int>(bag.size()) <= 7);

    // players[opponent].rack is a TileRack = vector<Tile>
    printTileBag(bag, players[opponent].rack, revealOpponent);
}

void challengeMove(Board &bonusBoard,
                   LetterBoard &letters,
                   BlankBoard &blanks,
                   TileBag &bag,
                   Player players[2],
                   GameSnapshot &lastSnapShot,
                   LastMoveInfo &lastMove,
                   int &currentPlayer,
                   bool &canChallenge,
                   bool &dictActive) {

    // 1) Basic checks
    if (!dictActive) {
        cout << "Dictionary not loaded. Cannot Challenge.\n";
        return;
    }

    if (!canChallenge || !lastMove.exists) {
        cout << "No word available to challenge.\n";
        return;
    }

    // can only challenge the opponents last word
    if (lastMove.playerIndex == currentPlayer) {
        cout << "You can only challenge your opponent's last word.\n";
        return;
    }

    // 2) Get the word from the board
    string challengedWord = extractMainWord(letters, lastMove.startRow,
                            lastMove.startCol, lastMove.horizontal);

    // Get the list of cross words from the board.
    vector<string> crossWords = crossWordList(letters, lastSnapShot.letters,
                                lastMove.startRow, lastMove.startCol, lastMove.horizontal);

    if (challengedWord.empty()) {
        cout << "No word found to challenge\n";
        canChallenge = false;
        lastMove.exists = false;
        return;
    }

    cout << "Challenging words: " << challengedWord;
    for (auto &crossWord : crossWords) {
        cout << "  " << crossWord;
    }

    bool isValidCrossWord = true;

    for ( string &word: crossWords) {
        if (!isValidWord(word)) {
            isValidCrossWord = false;
        }
    }

    // 3) Check dictionary
    if (!isValidWord(challengedWord) || !isValidCrossWord) {
        //Challenge Successful:
        // Word is not in dictionary -> undo it and remove it from the board.
        cout << "\nChallenge successful! The play is not valid.\n";

        // Restore snapshot from before the last word move
        letters = lastSnapShot.letters;
        blanks = lastSnapShot.blanks;
        bag = lastSnapShot.bag;
        players[0] = lastSnapShot.players[0];
        players[1] = lastSnapShot.players[1];

        // Offending player loses their move, challenger still has the turn.
        // currentPlayer already is the challenger, so NOT flipping the turn.

        canChallenge = false;
        lastMove.exists = false;

        cout << "Board and scores reverted to before the invalid word.\n";
        //printBoard(bonusBoard, letters);

        cout << "Scores: Player 1 = " << players[0].score
             << " | Player 2 = " << players[1].score << endl;
        //cout << "Your rack:\n";
        //printRack(players[currentPlayer].rack);

    } else {
        // Challenge Failed.
        // Word is valid, opponent gets +5
        cout << "\nChallenge failed! The play is valid.\n";
        players[lastMove.playerIndex].score += 5;
        cout << "Player " << (lastMove.playerIndex + 1) << " gains 5 points.\n";

        canChallenge = false;
        lastMove.exists = false;

        //printBoard(bonusBoard, letters);

        cout << "Scores: Player 1 = " << players[0].score
             << " | Player 2 = " << players[1].score << endl;

        //cout << "You still have your turn. Your rack:\n";
        //printRack(players[currentPlayer].rack);
    }
}

// Handle Resignation
bool handleQuit(const Player players[2], int currentPlayer) {

    cout << "\nGame Over.\n";

    cout << "Player " << (currentPlayer + 1) << " Resigns from the game.\n"
         << "Player " << ((1 - currentPlayer) + 1) << " Wins the game." << endl;

    cout << "Final Scores:\n";
    cout << "Player 1: " << players[0].score << endl;
    cout << "Player 2: " << players[1].score << endl;

    return true;
}

/*
void handleRackChoice(Board &bonusBoard,
                      LetterBoard &letters,
                      TileBag &bag,
                      Player players[2],
                      int &currentPlayer,
                      bool &canChallenge,
                      LastMoveInfo &lastMove) {
    // Reference for current player's rack
    TileRack &currentRack = players[currentPlayer].rack;


    cout << R"(Enter rack command ("4-7" to swap, "0" to shuffle, "X" to exchange): )";
    string command;
    cin >> command;

    bool ok = applyRackCommand(bag, currentRack, command);

    if (ok) {
        cout << "Rack Updated";
        printRack(currentRack);

        // After an exchange the turn is over
        if (command == "X" || command == "x") {
            // after an exchange you can no longer challenge that last word.
            players[currentPlayer].passCount += 1;
            canChallenge = false;
            lastMove.exists = false;

            // exchange uses the turn, switching players.
            currentPlayer = 1 - currentPlayer;
            cout << "Exchange is used. Switching over to Player " << (currentPlayer + 1) << "'s turn\n";
            cout << "\n\nPlayer" << (currentPlayer + 1) << "'s Rack" << endl;
            printBoard(bonusBoard, letters);
            cout << "Scores: Player 1 = " << players[0].score << " | Player 2 = " << players[1].score << endl;
            printRack(players[currentPlayer].rack);
        }
    } else {
        cout << "\nInvalid rack command\n";
    }
}
*/

/*
void handleMoveChoice(Board &bonusBoard,
                      LetterBoard &letters,
                      BlankBoard &blanks,
                      TileBag &bag,
                      Player players[2],
                      GameSnapshot &lastSnapShot,
                      LastMoveInfo &lastMove,
                      int &currentPlayer,
                      bool &canChallenge) {
    // Refernce for current player's rack
    TileRack &currentRack = players[currentPlayer].rack;

    cout << "Enter move in format <RowCol> <H/V> <WordFromRack>\n";
    cout << "Example: A10 V RETINAS\n\n";
    cout << "RowCol: ";
    string pos;
    cin >> pos;

    cout << "Direction (H/V): ";
    string dirStr;
    cin >> dirStr;

    cout << "Word (from rack only): ";
    string word;
    cin >> word;

    if (pos.size() < 2 || pos.size() > 3) {
        cout << "Invalid Position";
        return;
    }

    char rowChar = static_cast<char>(toupper(static_cast<unsigned char>(pos[0])));
    int row = rowChar - 'A';
    if (row < 0 || row >= BOARD_SIZE) {
        cout << "Row out of range";
        return;
    }

    int col = 0;

    // C++ Learning
    // pos.substr(1) takes the string from index 1 foward
    // ex : "A10" -> "10"
    // and stoi turns that to an int ( "10" -> 10)
    // catch block runs if stoi() throws as exception ( "AAA" )
    try {
        col = stoi(pos.substr(1)) - 1; // 1-based to 0-based
    } catch (...) {
        cout << "Invalid column number";
        return;
    }

    if (col < 0 || col >= BOARD_SIZE) {
        cout << "Column out of range";
        return;
    }

    bool horizontal = (toupper(static_cast<unsigned char>(dirStr[0])) == 'H');

    // Preview on copies
    LetterBoard previewLetters = letters;
    BlankBoard previewBlanks = blanks;
    TileRack previewRack = currentRack;
    TileBag previewBag = bag;

    MoveResult preview = playWord(
        bonusBoard,
        previewLetters,
        previewBlanks,
        previewBag,
        previewRack,
        row,
        col,
        horizontal,
        word
    );

    if (!preview.success) {
        cout << "Move Failed " << preview.errorMessage << endl;
        return;
    }

    cout << "\nProposed move score (main word + cross words): " << preview.score << endl;


    cout << "\nPreview board:\n";
    printBoard(bonusBoard, previewLetters);

    cout << "\nPreview rack:\n";
    printRack(previewRack);


    cout << "\nConfirm move? (y/n): ";
    char confirm;
    cin >> confirm;
    confirm = static_cast<char>(toupper(static_cast<unsigned char>(confirm)));

    if (confirm != 'Y') {
        cout << "Move cancelled.\n";
        return;
    }

    // Taking an snapshot before applying final word
    lastSnapShot.letters = letters;
    lastSnapShot.blanks = blanks;
    lastSnapShot.bag = bag;
    lastSnapShot.players[0] = players[0];
    lastSnapShot.players[1] = players[1];

    MoveResult finalResult = playWord(
        bonusBoard,
        letters,
        blanks,
        bag,
        currentRack,
        row,
        col,
        horizontal,
        word
    );

    if (!finalResult.success) {
        // Should not normally happen since preview succeeded
        cout << "Unexpected error applying move: " << finalResult.errorMessage << endl;
    } else {
        cout << "Move played. Score: " << finalResult.score << endl;
        players[currentPlayer].score += finalResult.score;

        // This will revert back to previous values if the play becomes illegal.
        players[0].passCount = 0;
        players[1].passCount = 0;

        // Recording the last move for potential challenging.
        lastMove.exists = true;
        lastMove.playerIndex = currentPlayer;
        lastMove.startRow = row;
        lastMove.startCol = col;
        lastMove.horizontal = horizontal;
        canChallenge = true;
    }

    printBoard(bonusBoard, letters);
    cout << "Scores: Player 1 = " << players[0].score << " | Player 2 = " << players[1].score << endl;

    // Show next player's rack
    currentPlayer = 1 - currentPlayer;

    cout << "\nNow its Player " << (currentPlayer + 1) << "'s turn" << endl;
    cout << "Rack:\n";

    printRack(players[currentPlayer].rack);
}

*/





































