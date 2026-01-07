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
#include "../../../include/engine/game_director.h"
#include "../../../include/choices.h"
#include "../../../include/ai_player.h"
#include "../../../include/modes/Home/home.h"
#include "../../../include/interface/renderer.h"

#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/engine/mechanics.h"

using namespace std;

// Wrapper for threading
MatchResult runSingleGame(AIStyle s1, AIStyle s2, int id, bool verbose) {
    AIPlayer bot1(s1);
    AIPlayer bot2(s2);
    Board b = createBoard();

    GameDirector::Config cfg;
    cfg.verbose = verbose;
    cfg.allowChallenge = false; // Speed optimization for Simulation

    GameDirector director(&bot1, &bot2, b, cfg);
    return director.run(id);
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

