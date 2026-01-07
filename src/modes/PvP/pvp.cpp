#include <iostream>
#include <string>
#include <sstream>

#include "../../../include/engine/board.h"
#include "../../../include/move.h"
#include "../../../include/engine/tiles.h"
#include "../../../include/engine/rack.h"
#include "../../../include/engine/dictionary.h"
#include "../../../include/engine/game_director.h"
#include "../../../include/choices.h"
#include "../../../include/modes/PvP/pvp.h"
#include "../../../include/engine/state.h"
#include "../../../include/engine/referee.h"
#include "../../../include/human_player.h"
#include "../../../include/modes/Home/home.h"
#include "../../../include/interface/renderer.h"
#include "../../../include/engine/mechanics.h"

using namespace std;

void runPvP() {
    if (gDictionary.nodes.empty()) gDictionary.loadFromFile("csw24.txt");

    HumanPlayer p1;
    HumanPlayer p2;
    Board b = createBoard();

    GameDirector::Config cfg;
    cfg.verbose = true;
    cfg.allowChallenge = true;

    GameDirector director(&p1, &p2, b, cfg);
    director.run();

    Renderer::waitForQuitKey();
    Renderer::clearScreen();
}
























