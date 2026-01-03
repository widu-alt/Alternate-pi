#include <iostream>
#include <limits>
#include <string>

#include "../include/engine/board.h"
#include "../include/move.h"
#include "../include/engine/tiles.h"
#include "../include/engine/dictionary.h"
#include "../include/engine/mechanics.h"
#include "../include/choices.h"

using namespace std;

bool executePlayMove(GameState& state,
                     const Move &move,
                     const Board &bonusBoard) {

    MoveResult result = Referee::validateMove(state, move, bonusBoard, gDictionary);

    if (result.success) {
        // 2. Act (The Executioner)
        cout << "Move Valid! Score: " << result.score << endl;
        Mechanics::applyMove(state, move, result.score);
        return true;
    } else {
        cout << "Invalid Move: " << result.message << endl;
        return false;
    }
}

bool executeExchangeMove(GameState& state, const Move &move) {
    if (Mechanics::attemptExchange(state, move)) {
        cout << "Exchange successful. Turn passes.\n";
        return true;
    } else {
        cout << "Exchange failed (Not enough tiles).\n";
        return false;
    }
}

bool handleSixPassEndGame(GameState& state) {
    if (state.players[0].passCount < 3 || state.players[1].passCount < 3) return false;

    cout << "\nSix consecutive scoreless turns. Game Over.\n";

    Mechanics::applySixPassPenalty(state);

    cout << "Final Scores: P1: " << state.players[0].score << " | P2: " << state.players[1].score << endl;
    return true;
}

bool handleEmptyRackEndGame(GameState& state,
                            const Board &bonusBoard,
                            GameState &lastSnapshot,
                            LastMoveInfo &lastMove,
                            bool &canChallenge,
                            PlayerController* controller) {

    int pIdx = lastMove.playerIndex;
    // Condition: Previous move existed, player index valid, and that player is now out of tiles
    if (!(lastMove.exists && pIdx != -1 && state.players[pIdx].rack.empty())) {
        return false;
    }

    // Ask the player who just went out what they want to do (usually automatic, but framework allows choice)
    Move decision = controller->getEndGameDecision();

    if (decision.type == MoveType::PASS) {
        Mechanics::applyEmptyRackBonus(state, pIdx);
        cout << "\nGame Over (Rack Empty).\n";
        cout << "Final Scores: P1: " << state.players[0].score << " | P2: " << state.players[1].score << endl;
        return true;
    }

    if (decision.type == MoveType::CHALLENGE) {
        // Execute challenge logic
        challengeMove(state, bonusBoard, lastSnapshot, lastMove, canChallenge);

        // If challenge FAILED (meaning the winning move was VALID), and bag is empty -> Game Over
        // If challenge SUCCEEDED, the move was undone, tiles returned, game continues.

        if (state.bag.empty() && state.players[pIdx].rack.empty()) {
            Mechanics::applyEmptyRackBonus(state, pIdx);
            cout << "\nGame Over.\n";
            cout << "Final Scores: P1: " << state.players[0].score << " | P2: " << state.players[1].score << endl;
            return true;
        }
        return false;
    }
    return false;
}

void passTurn(GameState& state, bool &canChallenge, LastMoveInfo &lastMove) {

    cout << "Player " << (state.currentPlayerIndex + 1) << " passes.\n";
    state.players[state.currentPlayerIndex].passCount++;
    canChallenge = false;
    lastMove.exists = false;
}

void showTileSet(const TileBag &bag, const Player players[2], int currentPlayer) {

    int opponent = 1 - currentPlayer;
    bool revealOpponent = (static_cast<int>(bag.size()) <= 7);

    // players[opponent].rack is a TileRack = vector<Tile>
    printTileBag(bag, players[opponent].rack, revealOpponent);
}

void challengeMove(GameState& state,
                   const Board &bonusBoard,
                   const GameState &lastSnapshot,
                   LastMoveInfo &lastMove,
                   bool &canChallenge)
{
    if (!canChallenge || !lastMove.exists) {
        cout << "No move available to challenge.\n";
        return;
    }
    if (lastMove.playerIndex == state.currentPlayerIndex) {
        cout << "You cannot challenge yourself.\n";
        return;
    }

    string mainWord = extractMainWord(state.board, lastMove.startRow, lastMove.startCol, lastMove.horizontal);
    vector<string> crossWords = crossWordList(state.board, lastSnapshot.board, lastMove.startRow, lastMove.startCol, lastMove.horizontal);

    cout << "Challenging Main: " << mainWord << "\n";

    bool invalid = !gDictionary.isValidWord(mainWord);
    if(invalid) cout << "INVALID: " << mainWord << endl;

    for (const string& w : crossWords) {
        if (!gDictionary.isValidWord(w)) {
            cout << "INVALID CROSS: " << w << endl;
            invalid = true;
        }
    }

    if (invalid) {
        cout << "\n>>> CHALLENGE SUCCESSFUL! <<<\n";
        cout << "Reverting state...\n";
        Mechanics::restoreSnapshot(state, lastSnapshot);
        canChallenge = false;
        lastMove.exists = false;
        cout << "Board reverted. It is now Player " << (state.currentPlayerIndex + 1) << "'s turn.\n";
    } else {
        cout << "\n>>> CHALLENGE FAILED! <<<\n";
        // Penalty: Opponent gets +5 Points
        state.players[lastMove.playerIndex].score += 5;
        cout << "Player " << (lastMove.playerIndex + 1) << " awarded +5 points.\n";
        canChallenge = false;
        lastMove.exists = false;
        cout << "Move stands.\n";
    }

    cout << "Press ENTER...";
    cin.ignore(); cin.get();
}

// Handle Resignation
bool handleQuit(GameState &state) {

    cout << "\nGame Over.\n";

    cout << "\nPlayer " << (state.currentPlayerIndex + 1) << " Resigns.\n";

    cout << "Final Scores:\n";
    cout << "Player 1: " << state.players[0].score << endl;
    cout << "Player 2: " << state.players[1].score << endl;

    return true;
}