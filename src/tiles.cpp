#include "../include/tiles.h"
#include "../include/rack.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

static vector<string> buildTileGroups(const vector<Tile> &bag) {

    // counts[0..25] = A-Z, counts[26] = '?'
    int counts[27] = {0};

    for (const Tile &tile : bag) {
        char ch = tile.letter;
        unsigned char uch = static_cast<unsigned char>(ch);

        if (ch == '?') {
            counts[26]++;
        } else if (isalpha(uch)){
            ch = static_cast<char>(toupper(uch));
            counts[ch - 'A']++;
        }
    }

    // Building token gaps like "AAAAA", "BB", "??"
    vector<string> tokens;

    for (int i = 0; i < 26; i++) {
        int count = counts[i];
        if (count <= 0) continue;

        char letter = static_cast<char>('A' + i);
        while (count > 0) {
            int groupSize = min(5, count);
            tokens.push_back(string(groupSize, letter));
            count -= groupSize;
        }
    }

    if (counts[26] > 0){
        int count = counts[26];
        while (count > 0) {
            int groupSize = min(5, count);
            tokens.push_back(string(groupSize, '?'));
            count -= groupSize;
        }
    }

    return tokens;
}

// Pack toens into multiple lines inside a box
static void printGroupedTokens(const vector<string> &tokens, const string &headerLabel) {

    // Pack tokens into lines with limited no. of groups per line
    const int MAX_GROUPS_PER_LINE = 10;
    vector<string> lines;
    string current;
    int groupsInCurrentLine = 0;

    for (const string &tok : tokens) {
        if (groupsInCurrentLine == 0) {
            // first group in the line
            current = "  " + tok; // reset
            groupsInCurrentLine = 1;
        } else if (groupsInCurrentLine >= MAX_GROUPS_PER_LINE) {
            // line full
            lines.push_back(current);
            current = "  " + tok;  // reset
            groupsInCurrentLine = 1;
        } else {
            current += "  " + tok; // append
            groupsInCurrentLine++;
        }
    }

    // push the last partial line too
    if (!current.empty()) {
        lines.push_back(current);
    }

    // Find the widest line for box width
    size_t contentWidth = 0;
    for (const string &ln : lines) {
        contentWidth = max(contentWidth, ln.size());
    }

    // +-----(contentWidth x "-")-----+
    string border = "+" + string(contentWidth + 2, '-') + "+";

    cout << headerLabel << endl;
    cout << border << endl;

    for (const string &ln : lines) {
        string padded = ln;
        if (padded.size() < contentWidth) {
            padded += string(contentWidth - padded.size(), ' ');
        }
        cout << padded << endl;
    }
    cout << border << endl;
}

// Show the tilebag from player's prespective
void printTileBag(const TileBag &bag, const vector<Tile> &opponentRack, bool revealOpponentRack) {

    // Build "unseen" set of tiles (bag + opponent's rack)
    vector<Tile> unseen;
    unseen.reserve(bag.size() + opponentRack.size());
    unseen.insert(unseen.end(), bag.begin(), bag.end());
    unseen.insert(unseen.end(), opponentRack.begin(), opponentRack.end());

    int unseenCount = static_cast<int>(unseen.size());
    int bagCount = static_cast<int>(bag.size());
    int oppCount = static_cast<int>(opponentRack.size());

    cout << "\nTiles not in your rack: " << unseenCount << endl;

    // Show unseen tiles as grouped distribution
    vector<string> unseenTokens = buildTileGroups(unseen);
    printGroupedTokens(unseenTokens, " Unseen tiles");

    // if the real bag has 7 or fewer tiles, unseen exactly what the opponent has
    if (revealOpponentRack) {
        if (!opponentRack.empty() && unseenCount <= 7) {
            cout << "\n(Endgame) Oppnent rack size: " << unseenCount << endl;
        }
    }
}



TileBag createStandardTileBag() {
    TileBag bag;

    /*C++ LEARNING
        [&bag] means the lambda wants to acces to the variable bag and it wants access by reference.
        so the lambda can modify the baf directly.
        letter -> The letter to be added
        points -> The number of points the tile is worth
        count -> How many of those tiles to be added
        ex: addTiles('A', 1, 9) means 9 tiles of letter A each worth 1 point are added to the bag.
        bag.push_back(Tile{'A', 1}); x 9 times
     */
    auto addTiles = [&bag](char letter, int points, int count) {
        for (int i = 0; i < count; i++) {
            bag.push_back(Tile{letter, points});
        }
    };

    // Letter distribution:
    // E×12; A,I×9; O×8; N,R,T×6; L,S,U,D×4; G×3;
    // B,C,M,P,F,H,V,W,Y×2; K,J,X,Q,Z×1; blanks×2

    //1-point letters
    addTiles('E', 1, 12);
    addTiles('A', 1, 9);
    addTiles('I', 1,  9);
    addTiles('O', 1,  8);
    addTiles('N', 1,  6);
    addTiles('R', 1,  6);
    addTiles('T', 1,  6);
    addTiles('L', 1,  4);
    addTiles('S', 1,  4);
    addTiles('U', 1,  4);

    // 2-point letters
    addTiles('D', 2, 4);
    addTiles('G', 2, 3);

    // 3-point letters
    addTiles('B', 3, 2);
    addTiles('C', 3, 2);
    addTiles('M', 3, 2);
    addTiles('P', 3, 2);

    // 4-point letters
    addTiles('F', 4, 2);
    addTiles('H', 4, 2);
    addTiles('V', 4, 2);
    addTiles('W', 4, 2);
    addTiles('Y', 4, 2);

    // 5-point letter
    addTiles('K', 5, 1);

    // 8-point letters
    addTiles('J', 8, 1);
    addTiles('X', 8, 1);

    // 10-point letters
    addTiles('Q',10, 1);
    addTiles('Z',10, 1);

    // Blanks: use '?' as the letter, 0 points
    addTiles('?', 0, 2);

    return bag;
}

//Suffels the tile bag (use once at game start)
void shuffleTileBag(TileBag &bag) {
    static mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    shuffle(bag.begin(), bag.end(), rng);
}
