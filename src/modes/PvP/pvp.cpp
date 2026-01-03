#include <iostream>
#include <string>
#include <sstream>

#include "../../../include/board.h"
#include "../../../include/move.h"
#include "../../../include/tiles.h"
#include "../../../include/rack.h"
#include "../../../include/dict.h"
#include "../../../include/choices.h"
#include "../../../include/modes/PvP/pvp.h"
#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/human_player.h"
#include "../../../include/modes/Home/home.h"

using namespace std;

// Helper to apply a valid move to the state (Since Referee only judges)
void applyMoveToState(GameState& state, const Move& move, int score) {
    int dr = move.horizontal ? 0 : 1;
    int dc = move.horizontal ? 1 : 0;
    int r = move.row;
    int c = move.col;

    TileRack& rack = state.players[state.currentPlayerIndex].rack;
    string word = move.word;

    // 1. Place Tiles on Board
    for (char letter : word) {
        // Skip existing tiles on board
        while (r < BOARD_SIZE && c < BOARD_SIZE && state.board[r][c] != ' ') {
            r += dr; c += dc;
        }

        // Place new tile
        state.board[r][c] = toupper(letter);
        state.blanks[r][c] = (letter >= 'a' && letter <= 'z'); // Capture blank from input case

        // Remove from Rack
        // Logic: Find the first matching tile (or blank) and remove it
        for (auto it = rack.begin(); it != rack.end(); ++it) {
            bool match = false;
            if (it->letter == '?') {
                match = true; // Use blank if needed
            } else if (toupper(it->letter) == toupper(letter)) {
                match = true;
            }

            if (match) {
                rack.erase(it);
                break;
            }
        }

        r += dr; c += dc;
    }

    // 2. Update Score
    state.players[state.currentPlayerIndex].score += score;
    state.players[state.currentPlayerIndex].passCount = 0; // Reset pass count

    // 3. Draw New Tiles
    // FIX: Use 'state.bag' instead of just 'bag'
    if (rack.size() < 7 && !state.bag.empty()) {
        drawTiles(state.bag, rack, static_cast<int>(7 - rack.size()));
    }
}

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

    // Legacy variables needed for "Undo/Challenge" logic
    // (We keep these so handleEmptyRackEndGame etc. still work)
    GameSnapshot lastSnapShot;
    LastMoveInfo lastMove;
    lastMove.exists = false;
    lastMove.playerIndex = -1;
    bool canChallenge = false;

    // Bonus board is static
    Board bonusBoard = createBoard();

    printTitle();
    cout << "Welcome to Terminal Crossword Game (2-player mode)\n";

    extern Dawg gDawg; // Use global dictionary (or load local Dawg dict;)
    bool dictActive = true;
    if (!loadDictionary("csw24.txt")) {
        cout << "WARNING: Dictionary not loaded.\n";
        dictActive = false;
    }

    while (true) {
        int pIdx = state.currentPlayerIndex;
        // Use references for cleaner code
        Player& current = state.players[pIdx];
        Player& opponent = state.players[1 - pIdx];

        // FIX: Use state.board instead of 'letters'
        printBoard(bonusBoard, state.board);
        cout << "Scores: Player 1 = " << state.players[0].score
             << " | Player 2 = " << state.players[1].score << endl;
        cout << "Player " << (pIdx + 1) << "'s Rack" << endl;

        printRack(current.rack);

        // FIX: Use state.players
        if (handleSixPassEndGame(state.players)) {
            break;
        }

        // FIX: Update arguments to use 'state' members
        if (handleEmptyRackEndGame(bonusBoard,
                                   state.board,
                                   state.blanks,
                                   state.bag,
                                   state.players,
                                   lastSnapShot,
                                   lastMove,
                                   pIdx,
                                   canChallenge,
                                   dictActive,
                                   controllers[pIdx])) {
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
            // FIX: Pass 'state.players'
            passTurn(state.players, pIdx, canChallenge, lastMove);
            // State updates happen inside passTurn, but we ensure index sync just in case
            state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            continue;
        }

        if (move.type == MoveType::QUIT) {
            if (handleQuit(state.players, pIdx)) {
                break;
            }
            continue;
        }

        if (move.type == MoveType::CHALLENGE) {
            // FIX: Pass 'state' members
            challengeMove(bonusBoard, state.board, state.blanks, state.bag, state.players,
                          lastSnapShot, lastMove, pIdx, canChallenge, dictActive);
            continue;
        }

        if (move.type == MoveType::EXCHANGE) {
            // FIX: Pass 'state.bag'
            bool success = executeExchangeMove(state.bag, current, move);

            if (success) {
                lastMove.exists = false;
                canChallenge = false;
                state.currentPlayerIndex = 1 - state.currentPlayerIndex;
            }
            continue;
        }

        if (move.type == MoveType::PLAY) {
            // --- NEW REFEREE LOGIC START ---

            // 1. Validate (The Judge)
            MoveResult result = Referee::validateMove(state, move, bonusBoard, gDawg);

            if (result.success) {
                // Save Snapshot for "Undo" (Legacy support)
                // We manually construct the snapshot from current state
                takeSnapshot(lastSnapShot, state.board, state.blanks, state.players, state.bag);

                // 2. Apply (The Mutator)
                applyMoveToState(state, move, result.score);

                cout << "Move Successful! Points: " << result.score << endl;

                // Update Legacy Tracking (for EndGame checks)
                lastMove.exists = true;
                lastMove.playerIndex = pIdx;
                lastMove.startRow = move.row;
                lastMove.startCol = move.col;
                lastMove.horizontal = move.horizontal;
                canChallenge = true; // Technically redundant if Referee validates, but keeps legacy flow valid

                // Switch Turn
                state.currentPlayerIndex = 1 - state.currentPlayerIndex;

            } else {
                cout << "Invalid Move: " << result.message << endl;
            }
        }
    }

    delete controllers[0];
    delete controllers[1];
    waitForQuitKey();
    clearScreen();
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
























