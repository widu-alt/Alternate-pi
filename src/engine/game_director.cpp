#include "../../include/engine/game_director.h"
#include "../../include/interface/renderer.h"
#include "../../include/choices.h" // For extractMainWord/crossWordList helpers
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

static std::mutex g_director_mutex;

GameDirector::GameDirector(PlayerController* p1, PlayerController* p2,
                           const Board& bBoard,
                           Config cfg) : bonusBoard(bBoard), config(cfg)
{
    controllers[0] = p1;
    controllers[1] = p2;
}

void GameDirector::log(const string& msg) {
    if (!config.verbose) return;
    std::lock_guard<std::mutex> lock(g_director_mutex);
    cout << msg << endl;
}

void GameDirector::initGame() {
    // 1. Clear State (Using Mechanics/Helpers)
    clearLetterBoard(state.board);
    clearBlankBoard(state.blanks);
    state.bag = createStandardTileBag();
    shuffleTileBag(state.bag);

    for(int i=0; i<2; i++) {
        state.players[i].rack.clear();
        state.players[i].score = 0;
        state.players[i].passCount = 0;
        drawTiles(state.bag, state.players[i].rack, 7);
    }
    state.currentPlayerIndex = 0;

    lastMove.reset();
    snapshot = state.clone();
    canChallenge = false;
}

MatchResult GameDirector::run(int gameId) {
    initGame();
    if (config.verbose) log("Game " + to_string(gameId) + " Started.");

    bool gameRunning = true;
    while (gameRunning) {
        int pIdx = state.currentPlayerIndex;

        // 1. Render
        if (config.verbose) {
            std::lock_guard<std::mutex> lock(g_director_mutex);
            Renderer::printBoard(bonusBoard, state.board);
            cout << "Scores: P1=" << state.players[0].score << " | P2=" << state.players[1].score << endl;
            if (controllers[pIdx]->getName() == "Human") Renderer::printRack(state.players[pIdx].rack);
        }

        // 2. END GAME: Empty Rack Check (The "Response" Phase)
        if (lastMove.exists && lastMove.emptiedRack && state.bag.empty()) {
            if (config.verbose) log("Player " + to_string(lastMove.playerIndex + 1) + " has emptied their rack!");

            Move response = controllers[pIdx]->getEndGameResponse(state, lastMove);

            if (response.type == MoveType::CHALLENGE && config.allowChallenge) {
                bool successful = executeChallenge(pIdx);
                if (successful) {
                    // Challenge Success: Move reverted. Offender lost turn.
                    // Game continues (rack is no longer empty).
                    // It is now the Challenger's (pIdx) turn to play.
                    log("Challenge Successful. Game Continues.");
                    continue;
                } else {
                    // Challenge Failed: Move Valid. Game Over.
                    gameRunning = false;
                    break;
                }
            } else {
                // Pass: Accept Defeat. Apply Bonus.
                Mechanics::applyEmptyRackBonus(state, lastMove.playerIndex);
                if (config.verbose) log("Game Over. Bonus Awarded.");
                gameRunning = false;
                break;
            }
        }

        // 3. END GAME: Six Pass
        if (config.sixPassEndsGame && state.players[0].passCount >= 3 && state.players[1].passCount >= 3) {
            if (config.verbose) log("Game Over: 6 Consecutive Passes.");
            Mechanics::applySixPassPenalty(state);
            gameRunning = false;
            break;
        }

        // 4. Normal Turn
        gameRunning = processTurn(pIdx);

        if (config.delayMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(config.delayMs));
    }

    MatchResult res;
    res.scoreP1 = state.players[0].score;
    res.scoreP2 = state.players[1].score;
    if (res.scoreP1 > res.scoreP2) res.winner = 0;
    else if (res.scoreP2 > res.scoreP1) res.winner = 1;
    else res.winner = -1;
    return res;
}

bool GameDirector::processTurn(int pIdx) {
    Move move = controllers[pIdx]->getMove(state, bonusBoard, lastMove, canChallenge);

    if (move.type == MoveType::QUIT) return false;

    // Handle Challenge (Mid-Game)
    if (move.type == MoveType::CHALLENGE) {
        if (config.allowChallenge && canChallenge) {
            if (executeChallenge(pIdx)) {
                // Success: Reverted. Offender lost turn.
                // Return TRUE to loop again with SAME pIdx (Challenger gets to play now)
                return true;
            }
            // Failure: Offender kept points. Challenger (pIdx) loses turn.
            state.currentPlayerIndex = 1 - pIdx;
            return true;
        }
    }

    // Handle Play/Pass/Exchange
    if (move.type == MoveType::PASS) {
        state.players[pIdx].passCount++;
        if (config.verbose) log("Player " + to_string(pIdx+1) + " Passed.");
        canChallenge = false;
        lastMove.reset();
    }
    else if (move.type == MoveType::EXCHANGE) {
        if (Mechanics::attemptExchange(state, move)) {
            if (config.verbose) log("Player " + to_string(pIdx+1) + " Exchanged.");
            canChallenge = false;
            lastMove.reset();
        } else {
            if (config.verbose) log("Exchange Failed. Treated as Pass.");
            state.players[pIdx].passCount++;
        }
    }
    else if (move.type == MoveType::PLAY) {
        executePlay(pIdx, move);
    }

    state.currentPlayerIndex = 1 - pIdx;
    return true;
}

void GameDirector::executePlay(int pIdx, Move& move) {
    MoveResult result = Referee::validateMove(state, move, bonusBoard, gDictionary);

    if (result.success) {
        // 1. Commit Snapshot (Clean state)
        Mechanics::commitSnapshot(snapshot, state);

        // 2. Apply Move (Dirty state)
        Mechanics::applyMove(state, move, result.score);

        // 3. Populate History
        lastMove.exists = true;
        lastMove.playerIndex = pIdx;
        lastMove.move = move;
        lastMove.score = result.score;
        lastMove.emptiedRack = state.players[pIdx].rack.empty();

        // OPTIMIZATION: Only build word strings if challenges are enabled
        if (config.allowChallenge) {
            lastMove.formedWords.clear();
            string mainWord = extractMainWord(state.board, move.row, move.col, move.horizontal);
            lastMove.formedWords.push_back(mainWord);

            vector<string> crossWords = crossWordList(state.board, snapshot.board, move.row, move.col, move.horizontal);
            lastMove.formedWords.insert(lastMove.formedWords.end(), crossWords.begin(), crossWords.end());

            canChallenge = true;
        } else {
            canChallenge = false;
        }

        if (config.verbose) {
            log("Player " + to_string(pIdx+1) + " Played: " + move.word + " (" + to_string(result.score) + ")");
        }
    } else {
        if (config.verbose) log("Invalid Move: " + result.message);
        state.players[pIdx].passCount++;
    }
}

bool GameDirector::executeChallenge(int challengerIdx) {
    // 1. Verify words using the list we populated in executePlay
    // If list is empty (fallback), we re-scan.
    bool invalid = false;

    if (!lastMove.formedWords.empty()) {
        for(const string& w : lastMove.formedWords) {
            if(!gDictionary.isValidWord(w)) { invalid = true; break; }
        }
    } else {
        // Fallback Re-scan logic (if formedWords wasn't populated for some reason)
        string mainWord = extractMainWord(state.board, lastMove.move.row, lastMove.move.col, lastMove.move.horizontal);
        if(!gDictionary.isValidWord(mainWord)) invalid = true;
        // (Crossword scan omitted for brevity in fallback, assume main list is populated)
    }

    if (invalid) {
        if (config.verbose) log(">>> CHALLENGE SUCCESSFUL! Invalid Word Found.");

        Mechanics::restoreSnapshot(state, snapshot);
        canChallenge = false;
        lastMove.reset();

        // Turn Control: Offender lost turn. Index set to Challenger.
        state.currentPlayerIndex = challengerIdx;
        return true;
    } else {
        if (config.verbose) log(">>> CHALLENGE FAILED! Words Valid. (+5 pts to opponent)");

        state.players[lastMove.playerIndex].score += 5;
        canChallenge = false;
        lastMove.reset();

        return false;
    }
}
