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

// [Existing includes...]
#include "../include/engine/state.h"

// --- ADD THESE FUNCTIONS TO src/choices.cpp ---

void applyMoveToState(GameState& state, const Move& move, int score) {
    int dr = move.horizontal ? 0 : 1;
    int dc = move.horizontal ? 1 : 0;
    int r = move.row;
    int c = move.col;

    TileRack& rack = state.players[state.currentPlayerIndex].rack;
    string word = move.word;

    // 1. Place Tiles
    for (char letter : word) {
        while (r < BOARD_SIZE && c < BOARD_SIZE && state.board[r][c] != ' ') {
            r += dr; c += dc;
        }
        state.board[r][c] = toupper(letter);
        state.blanks[r][c] = (letter >= 'a' && letter <= 'z');

        // Remove from Rack
        for (auto it = rack.begin(); it != rack.end(); ++it) {
            bool match = false;
            if (it->letter == '?') match = true;
            else if (toupper(it->letter) == toupper(letter)) match = true;

            if (match) {
                rack.erase(it);
                break;
            }
        }
        r += dr; c += dc;
    }

    // 2. Update Score & Pass Count
    state.players[state.currentPlayerIndex].score += score;
    state.players[state.currentPlayerIndex].passCount = 0;

    // 3. Draw Tiles
    if (rack.size() < 7 && !state.bag.empty()) {
        drawTiles(state.bag, rack, static_cast<int>(7 - rack.size()));
    }
}

void takeSnapshot(GameSnapshot &lastSnapShot,
                  const LetterBoard &letters,
                  const BlankBoard &blanks,
                  const Player players[2],
                  const TileBag &bag) {
    lastSnapShot.letters = letters;
    lastSnapShot.blanks = blanks;
    lastSnapShot.bag = bag;
    lastSnapShot.players[0] = players[0];
    lastSnapShot.players[1] = players[1];
}