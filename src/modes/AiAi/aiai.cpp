#include <iostream>
#include <vector>
#include <numeric>
#include <thread>
#include <chrono>
#include <future>
#include <mutex>

#include "../../../include/modes/AiAi/aiai.h"
#include "../../../include/engine/board.h"
#include "../../../include/move.h"
#include "../../../include/engine/tiles.h"
#include "../../../include/engine/rack.h"
#include "../../../include/engine/dictionary.h"
#include "../../../include/choices.h"
#include "../../../include/ai_player.h"
#include "../../../include/modes/Home/home.h"
#include "../../../include/interface/renderer.h"

#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/engine/mechanics.h"

using namespace std;

// Global mutex to prevent threads from garbling the console output
static std::mutex g_io_mutex;

// Stats
struct MatchResult {
    int scoreP1;
    int scoreP2;
    int winner; // 0 = P1, 1 = P2, -1 = Draw
};

// Encapsulate a single game logic for threading
MatchResult runSingleGame(AIStyle styleP1, AIStyle styleP2, int gameId, bool verbose) {
    // 1. Init State
    GameState state;
    clearLetterBoard(state.board);
    clearBlankBoard(state.blanks);
    state.bag = createStandardTileBag();
    shuffleTileBag(state.bag);
    Board bonusBoard = createBoard(); // Static const board creation is fast

    // Ensuring dictionary is loaded (Global One-Time Check)
    if (gDictionary.nodes.empty()) {
        gDictionary.loadFromFile("csw24.txt");
    }

    // Setup AI Controllers
    AIPlayer bot1(styleP1);
    AIPlayer bot2(styleP2);
    PlayerController* controllers[2] = { &bot1, &bot2 };

    // Initial Draw
    for (int i=0; i < 2; i++) {
        drawTiles(state.bag, state.players[i].rack, 7);
        state.players[i].score = 0;
        state.players[i].passCount = 0;
    }
    state.currentPlayerIndex = 0;

    bool gameOver = false;

    auto printState = [&](const string& action, const Move& move) {
        if (!verbose) return;
        std::lock_guard<std::mutex> lock(g_io_mutex);

        cout << "\n------------------------------------------------------------\n";
        cout << "GAME " << gameId << " | Turn: Player " << (state.currentPlayerIndex + 1) << " ("
             << (state.currentPlayerIndex == 0 ? "P1" : "P2") << ")\n";

        cout << "Action: " << action;
        if (move.type == MoveType::PLAY) cout << " '" << move.word << "' at " << (char)('A' + move.row) << (move.col + 1);
        if (move.type == MoveType::EXCHANGE) cout << " Tiles: " << move.exchangeLetters;
        cout << endl;

        Renderer::printBoard(bonusBoard, state.board);

        cout << "Rack P1: "; Renderer::printRack(state.players[0].rack);
        cout << "Rack P2: "; Renderer::printRack(state.players[1].rack);

        cout << "Scores: P1=" << state.players[0].score << " | P2=" << state.players[1].score << endl;
        cout << "------------------------------------------------------------\n";
    };

    while (!gameOver) {
        int pIdx = state.currentPlayerIndex;
        Player& current = state.players[pIdx];
        Player& opponent = state.players[1 - pIdx];

        // --- A. End Game Checks ---

        // 1. Six-Pass Rule (Draw)
        if (state.players[0].passCount >= 3 && state.players[1].passCount >= 3) {
            Mechanics::applySixPassPenalty(state);
            gameOver = true;
            break;
        }

        // --- B. AI Move Generation ---
        Move move = controllers[pIdx]->getMove(bonusBoard, state.board, state.blanks, state.bag,
                                               current, opponent, pIdx + 1);

        // --- C. Execution (No "Choices" middleware) ---

        if (move.type == MoveType::PASS) {
            state.players[pIdx].passCount++;

            if (verbose) {
                std::lock_guard<std::mutex> lock(g_io_mutex);
                cout << "Game " << gameId << " P" << (pIdx+1) << ": PASS\n";
            }
        }
        else if (move.type == MoveType::EXCHANGE) {
            // Mechanics handles the rack swap + passCount increment
            if (Mechanics::attemptExchange(state, move)) {
                if (verbose) {
                    std::lock_guard<std::mutex> lock(g_io_mutex);
                    cout << "Game " << gameId << " P" << (pIdx+1) << ": EXCHANGE " << move.exchangeLetters << "\n";
                }
            } else {
                // Should not happen with correct AI, but fallback to Pass
                state.players[pIdx].passCount++;
            }
        }
        else if (move.type == MoveType::PLAY) {
            // 1. Validate & Score (Referee)
            // Even though AI is "perfect", we need Referee to calculate the exact score points
            MoveResult result = Referee::validateMove(state, move, bonusBoard, gDictionary);

            if (result.success) {
                // 2. Apply (Mechanics)
                // This updates board, rack, score, refills tiles, and resets passCount
                Mechanics::applyMove(state, move, result.score);

                printState("PLAY", move);

                // 3. Check for "Empty Rack" Victory immediately after a valid play
                if (state.bag.empty() && state.players[pIdx].rack.empty()) {
                    Mechanics::applyEmptyRackBonus(state, pIdx);
                    gameOver = true;
                }

                if (verbose) {
                    std::lock_guard<std::mutex> lock(g_io_mutex);
                    cout << "Game " << gameId << " P" << (pIdx+1) << ": " << move.word
                         << " (" << result.score << " pts)\n";
                }
            } else {
                // AI made invalid move (Bug in Vanguard?) -> Treat as Pass to prevent infinite loops
                state.players[pIdx].passCount++;
                if (verbose) {
                    std::lock_guard<std::mutex> lock(g_io_mutex);
                    cout << "ERROR: Game " << gameId << " AI Invalid Move: " << result.message << endl;
                }
            }
        }
        // Switch Turn
        if (!gameOver) {
            state.currentPlayerIndex = 1 - state.currentPlayerIndex;
        }
    }

    // 3. Result Compilation
    MatchResult matchRes;
    if (state.players[0].score > state.players[1].score) matchRes.winner = 0;
    else if (state.players[1].score > state.players[0].score) matchRes.winner = 1;
    else matchRes.winner = -1; // Draw

    matchRes.scoreP1 = state.players[0].score;
    matchRes.scoreP2 = state.players[1].score;

    if (verbose) {
        std::lock_guard<std::mutex> lock(g_io_mutex);
        cout << "Game " << gameId << " Finished. "
             << matchRes.scoreP1 << " - " << matchRes.scoreP2 << endl;
    }

    return matchRes;
}

void runAiAi() {
    int numGames;
    bool verbose;
    int matchType;

    cout << "\n=========================================\n";
    cout << "           AI vs AI SIMULATION\n";
    cout << "=========================================\n";
    cout << "Select Matchup:\n";
    cout << "1. Speedi_Pi vs Speedi_Pi (Pure Speed Test)\n";
    cout << "2. Cutie_Pi  vs Speedi_Pi (Smart vs Fast)\n";
    cout << "3. Cutie_Pi  vs Cutie_Pi  (Clash of Titans)\n";
    cout << "Choice: ";
    cin >> matchType;

    AIStyle styleP1, styleP2;
    if (matchType == 1) { styleP1 = AIStyle::SPEEDI_PI; styleP2 = AIStyle::SPEEDI_PI; }
    else if (matchType == 2) { styleP1 = AIStyle::CUTIE_PI; styleP2 = AIStyle::SPEEDI_PI; }
    else { styleP1 = AIStyle::CUTIE_PI; styleP2 = AIStyle::CUTIE_PI; }

    cout << "Enter number of games to simulate: ";
    cin >> numGames;
    cout << "Watch the games? (1 = Yes, 0 = No/Fast): ";
    cin >> verbose;

    if (!gDictionary.loadFromFile("csw24.txt")) { cout << "Error: Dictionary not found.\n"; return; }

    auto startTotal = chrono::high_resolution_clock::now();

    // PARALLEL EXECUTION
    // utilize std::async to launch games on available cores
    vector<future<MatchResult>> futures;
    int batchSize = thread::hardware_concurrency(); // likely 6 on your Ryzen
    if (batchSize == 0) batchSize = 4;

    cout << "Launching " << numGames << " games across " << batchSize << " threads..." << endl;

    for (int i = 0; i < numGames; i++) {
        futures.push_back(async(launch::async, runSingleGame, styleP1, styleP2, i + 1, verbose));
    }

    vector<MatchResult> results;
    for (auto &f : futures) {
        results.push_back(f.get());
    }

    auto endTotal = chrono::high_resolution_clock::now();
    double elapsedMs = chrono::duration_cast<chrono::milliseconds>(endTotal-startTotal).count();

    // Statistics
    long long totalP1 = 0, totalP2 = 0;
    int winsP1 = 0, winsP2 = 0, draws = 0;

    for (const auto& res: results) {
        totalP1 += res.scoreP1;
        totalP2 += res.scoreP2;
        if (res.winner == 0) winsP1++;
        else if (res.winner == 1) winsP2++;
        else draws++;
    }

    string nameP1 = (styleP1 == AIStyle::SPEEDI_PI ? "Speedi_Pi" : "Cutie_Pi");
    string nameP2 = (styleP2 == AIStyle::SPEEDI_PI ? "Speedi_Pi" : "Cutie_Pi");
    if (matchType == 1 || matchType == 3) nameP2 = "Evil " + nameP2;

    cout << "\n\n=========================================\n";
    cout << "           SIMULATION RESULTS            \n";
    cout << "=========================================\n";
    cout << "Matchup: " << nameP1 << " vs " << nameP2 << endl;
    cout << "Games played: " << numGames << endl;
    cout << "Time Elapsed: " << (elapsedMs / 1000.0) << " seconds" << endl;
    cout << "Throughput:   " << (numGames / (elapsedMs / 1000.0)) << " games/sec" << endl;
    cout << "=========================================\n";
    cout << nameP1 << " Wins: " << winsP1 << " (" << (winsP1 * 100.0 / numGames) << "%)\n";
    cout << nameP2 << " Wins: " << winsP2 << " (" << (winsP2 * 100.0 / numGames) << "%)\n";
    cout << "Draws: " << draws << "\n";
    cout << "Avg Score (" << nameP1 << "): " << (totalP1 / (double)numGames) << "\n";
    cout << "Avg Score (" << nameP2 << "): " << (totalP2 / (double)numGames) << "\n";
    cout << "Combined Avg: " << ((totalP1 + totalP2) / (double)numGames / 2.0) << "\n";
    cout << "=========================================\n";

    Renderer::waitForQuitKey();
    Renderer::clearScreen();
}

