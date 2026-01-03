#include <iostream>

#include "../include/engine/dictionary.h"
#include "../include/modes/AiAi/aiai.h"
#include "../include/modes/PvP/pvp.h"
#include "../include/modes/Home/home.h"
#include "../include/modes/PvE/pve.h"

using namespace std;

int main() {

    // selection menu
    while (true) {

        printTitleScreen();

        char mode;

        loadDictionary("csw24.txt");

        cout << "=========================================\n";
        cout << "           Welcome to LEXI_PI\n";
        cout << "         Terminal Crossword Game\n";
        cout << "               Version 0.4\n";
        cout << "=========================================\n\n";

        cout << "(1) Player vs Player\n"
             << "(2) Player vs AI\n"
             << "(3) AI vs AI (Simulation)\n"
             << "(4) Word Wizard\n"
             << "(5) How to play\n"
             << "(6) About\n"
             << "(7) Quit\n"
             << "Please select an option: ";
        cin >> mode;

        if (mode == '1') {
            runPvP();
        } else if (mode == '2') {
            runPvE();
        } else if (mode == '3') {
            runAiAi();
        } else if (mode == '4') {
            string word = " ";
            wordWizard(word);
        } else if (mode == '5') {
            showHowToPlayScreen();
        } else if (mode == '6') {
            showAboutScreen();
        } else if (mode == '7') {
            cout << " May your racks be fruitful and your bingos plentiful!\n" << endl;
            break;
        }
    }

    return 0;
}