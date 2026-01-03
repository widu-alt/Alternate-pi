#include <iostream>
#include <vector>
#include <numeric>
#include <thread>
#include <chrono>
#include <future>
#include <mutex>

#include "../../../include/modes/AiAi/aiai.h"
#include "../../../include/board.h"
#include "../../../include/move.h"
#include "../../../include/tiles.h"
#include "../../../include/rack.h"
#include "../../../include/dict.h"
#include "../../../include/choices.h"
#include "../../../include/ai_player.h"
#include "../../../include/modes/Home/home.h"

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
    // Thread-Local Game State (No Shared Memory = No Locks = Fast)
    Board bonusBoard = createBoard();
    LetterBoard letters;
    clearLetterBoard(letters);
    BlankBoard blanks;
    clearBlankBoard(blanks);
    TileBag bag = createStandardTileBag();
    shuffleTileBag(bag);

    Player players[2];
    PlayerController* controllers[2];

    drawTiles(bag, players[0].rack, 7);
    players[0].score = 0;
    players[0].passCount = 0;
    controllers[0] = new AIPlayer(styleP1);

    drawTiles(bag, players[1].rack, 7);
    players[1].score = 0;
    players[1].passCount = 0;
    controllers[1] = new AIPlayer(styleP2);

    int currentPlayer = 0;
    GameSnapshot lastSnapShot;
    LastMoveInfo lastMove;
    lastMove.exists = false;
    lastMove.playerIndex = -1;

    bool canChallenge = false;
    bool dictActive = true;
    bool gameOver = false;

    // Helper to print state safely
    auto printState = [&](const string& action, const Move& move) {
        if (!verbose) return;
        std::lock_guard<std::mutex> lock(g_io_mutex);

        cout << "\n------------------------------------------------------------\n";
        cout << "GAME " << gameId << " | Turn: Player " << (currentPlayer + 1) << " ("
             << (currentPlayer == 0 ? "P1" : "P2") << ")\n";

        cout << "Action: " << action;
        if (move.type == MoveType::PLAY) cout << " '" << move.word << "' at " << (char)('A' + move.row) << (move.col + 1);
        if (move.type == MoveType::EXCHANGE) cout << " Tiles: " << move.exchangeLetters;
        cout << endl;

        printBoard(bonusBoard, letters);

        cout << "Rack P1: "; printRack(players[0].rack);
        cout << "Rack P2: "; printRack(players[1].rack);

        cout << "Scores: P1=" << players[0].score << " | P2=" << players[1].score << endl;
        cout << "------------------------------------------------------------\n";
    };

    while (!gameOver) {
        if (handleSixPassEndGame(players)) { gameOver = true; break; }
        if (handleEmptyRackEndGame(bonusBoard, letters, blanks, bag, players,
                                   lastSnapShot, lastMove, currentPlayer, canChallenge,
                                   dictActive, controllers[currentPlayer])) {
            gameOver = true; break;
        }

        Move move = controllers[currentPlayer]->getMove(bonusBoard, letters, blanks, bag,
                                                        players[currentPlayer],
                                                        players[1 - currentPlayer],
                                                        currentPlayer + 1);

        if (move.type == MoveType::PASS) {
            printState("PASS", move);
            passTurn(players, currentPlayer, canChallenge, lastMove);
            continue;
        }
        if (move.type == MoveType::EXCHANGE) {
            if (executeExchangeMove(bag, players[currentPlayer], move)) {
                printState("EXCHANGE", move);
                lastMove.exists = false;
                canChallenge = false;
                players[currentPlayer].passCount++;
                currentPlayer = 1 - currentPlayer;
            } else {
                if(verbose) {
                    std::lock_guard<std::mutex> lock(g_io_mutex);
                    cout << "[GAME " << gameId << "] Exchange Failed.\n";
                }
                passTurn(players, currentPlayer, canChallenge, lastMove);
            }
            continue;
        }
        if (move.type == MoveType::PLAY) {
            bool success = executePlayMove(bonusBoard, letters, blanks, bag, players,
                                           players[currentPlayer], move, lastSnapShot);
            if (success) {
                printState("PLAY", move);
                lastMove.exists = true;
                lastMove.playerIndex = currentPlayer;
                lastMove.startRow = move.row;
                lastMove.startCol = move.col;
                lastMove.horizontal = move.horizontal;
                canChallenge = true;
                currentPlayer = 1 - currentPlayer;
            } else {
                passTurn(players, currentPlayer, canChallenge, lastMove);
            }
        }
    }

    // Result
    int winner = -1;
    if (players[0].score > players[1].score) winner = 0;
    else if (players[1].score > players[0].score) winner = 1;

    delete controllers[0];
    delete controllers[1];

    if (verbose) {
        static std::mutex io_mutex;
        std::lock_guard<std::mutex> lock(io_mutex);
        cout << "Game " << gameId << " Finished. P1: " << players[0].score << " P2: " << players[1].score << endl;
    }

    return {players[0].score, players[1].score, winner};
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

    if (!loadDictionary("csw24.txt")) { cout << "Error: Dictionary not found.\n"; return; }

    auto startTotal = chrono::high_resolution_clock::now();

    // PARALLEL EXECUTION
    // We utilize std::async to launch games on available cores
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

    waitForQuitKey();
    clearScreen();
}

