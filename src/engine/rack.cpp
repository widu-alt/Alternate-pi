#include "../../include/engine/rack.h"
#include "../../include/engine/tiles.h"

#include <iostream>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

// Draw up to 'count' tiles from the bag into the rack.
// Returns how many tiles were actually drawn.
int drawTiles(TileBag &bag, TileRack &rack, int count) {
    int drawn = 0;
    while (drawn < count && !bag.empty()) {
        rack.push_back(bag.back()); // take from end
        bag.pop_back();
        ++drawn;
    }
    return drawn;
}

//Swap two tiles in the rack (based on player view)
bool handleSwapCommand(TileRack &rack, int index1, int index2) {

    // Covert to 0 based
    int i = index1 - 1;
    int j = index2 - 1;

    //validate indexes
    if (i < 0 || j < 0 || i >= static_cast<int>(rack.size()) || j >= static_cast<int>(rack.size())) {
        return false;
    }

    swap(rack[i], rack[j]);
    return true;
}

bool exchangeRack(TileRack &rack, const string& lettersStr, TileBag &bag) {
    if (lettersStr.empty()) return false;

    // Working with a temporary
    TileRack tempRack = rack;
    TileRack removed;

    // Uppercaseing the requested letters
    string letters = lettersStr;
    for (char &c : letters) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }

    // Trying to remove one matching tile from tempRack for each requested letter
    for (char ch: letters) {
        bool found = false;

        // C++ Learning
        // tempRack.begin() returns vector<Tile>::iterator
        // And a vector iterator behaves exactly like a pointer to an element.
        for (auto it = tempRack.begin(); it != tempRack.end(); ++it) {
            char rackCh = static_cast<char>(toupper(static_cast<unsigned char>((*it).letter)));
            if (rackCh == ch) {
                removed.push_back(*it);
                tempRack.erase(it);
                found = true;
                break;
            }

        }

        if (!found) {
            // Not enough copies of this letter in the rack.
            return false;
        }
    }

    // All requested letters are found.
    rack = tempRack;

    // Put removed tiles back into the bag
    bag.insert(bag.end(), removed.begin(), removed.end());

    //shuffle the bag
    shuffleTileBag(bag);

    // Draw the same number of tiles back
    drawTiles(bag, rack, static_cast<int>(removed.size()));

    return true;
}

//Shuffle the rack play order randomly
void shuffleRack(TileRack &rack) {
    static mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    shuffle(rack.begin(), rack.end(), rng);
}

// Parse a player command for the rack.
// - "4-7" → swap tile 4 and 7
// - "0"   → shuffle rack
// Returns true if command was valid and applied.
bool applyRackCommand(TileBag &bag, TileRack &rack, const string &command) {
    if (command == "X" || command == "x") {
        cout << "Enter rack indices to exchange (ex:AE?N): ";
        string lettersStr;
        cin >> lettersStr;

        if (!exchangeRack(rack, lettersStr, bag)) {
            cout << "\nInvalid exchange: you don't have those tiles.\n";
            return false;
        }

        return true;
    }

    if (command == "0") {
        shuffleRack(rack);
        return true;
    }

    //Expect format "x-y"
    size_t dashPos = command.find('-');
    // above find the "-" in the string and returns its position.
    // ex: "4-7", the position index of "-" is 1.
    // npos is a special constant that means "not found in the string" (big number like 18231234564321...)
    if (dashPos == string::npos) {
        return false;
    }

    string left = command.substr(0, dashPos);
    //meaning take everything between 0 and dashpos(1), in this case 4.
    string right = command.substr(dashPos + 1);
    //These two lines splits the string from "-"

    if (left.empty() || right.empty()) {
        return false;
    }

    //stoi -> string to int
    int index1 = stoi(right);
    int index2 = stoi(left);

    return handleSwapCommand(rack, index1, index2);
}
