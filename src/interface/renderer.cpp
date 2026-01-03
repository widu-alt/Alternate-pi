#include "../../include/interface/renderer.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

#ifndef BOARD_SIZE
#define BOARD_SIZE 15
#endif

// =============================================================
// CONSTANTS & HELPERS
// =============================================================

//Return 'text' centered inside a field of given width.
// If width is larger than the text, spaces are added on both sides.
static string centerText(const string& text, int width) {
    int padding = width - static_cast<int>(text.size());
    if (padding < 0) {
        //Text is longer than the width, just return it as-is
        return text;
    }

    int leftPadding = padding/2;
    int rightPadding = padding - leftPadding;

    return string(leftPadding, ' ') + text + string(rightPadding, ' ');
}

//Converting type to a label for printing
string cellLabel(CellType type) {
    switch (type) {
        case CellType::Normal:
            return " ";
        case CellType::DLS:
            return ">2x<";
        case CellType::TLS:
            return ">3x<";
        case CellType::DWS:
            return "<2x>";
        case CellType::TWS:
            return "<3x>";
        default: return " ";
    }
}

static void printLegend() {
    cout << "\nLegend:\n";
    cout << "  <2x> = Double Word   <3x> = Triple Word\n";
    cout << "  >2x< = Double Letter >3x< = Triple Letter\n\n";
}

static void printHorizontalBorder() {
    cout << "    +";
    for (int col = 0; col < BOARD_SIZE; col++) {
        for (int i = 0; i < CELL_INNER_WIDTH; i++) {
            cout << "-";
        }
        cout << "+";
    }
    cout << "\n";
}

//Printing header rows with column numbers 1-15
static void printColumnHeader() {
    printHorizontalBorder();

    // Leading spaces so numbers align with the board below
    cout << "    |";
    for (int col = 0; col < BOARD_SIZE; col++) {
        string numberText = to_string(col + 1);
        cout << centerText(numberText, CELL_INNER_WIDTH) << "|";
    }
    cout << "\n";

    printHorizontalBorder();
}

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

// =============================================================
// PUBLIC METHODS
// =============================================================

void Renderer::printTitle() {
    cout << "=========================================\n";
    cout << "                LEXI_PI\n";
    cout << "             Terminal Edition\n";
    cout << "\n";
    //cout << "     For My Lovely Girlfriend, Minadee\n";
    cout << "=========================================\n\n";
}

void Renderer::printBoard(const Board &bonusBoard, const LetterBoard &letters) {
    //printTitle();
    printLegend();
    printColumnHeader();
    cout << "\033[90m";

    for (int row = 0; row < BOARD_SIZE; row++) {
        char rowLabel = static_cast<char>('A' + row);

        //Row lebel at the left (eg: "A |")
        cout << "  " << "\033[0m" << rowLabel << "\033[90m" << " |";

        //print each cell in this row
        for (int col = 0; col < BOARD_SIZE; col++) {
            char letterOnBoard = letters[row][col];
            string cellText;

            if (letterOnBoard != ' ') {
                // There is a tile here, show letter instead of bonus
                //cellText = string(1, letterOnBoard);
                cout << "\033[38;2;255;165;0m" << centerText(string(1, letterOnBoard), CELL_INNER_WIDTH)  << "\033[90m" << "|";
            } else {
                //cellText = cellLabel(bonusBoard[row][col]);
                cout << centerText(cellLabel(bonusBoard[row][col]), CELL_INNER_WIDTH) << "|";
            }

            //cout << "\033[90m" << centerText(cellText, CELL_INNER_WIDTH) << "|" << "\033[0m";
        }
        cout << "\n";

        //print border under the row
        printHorizontalBorder();
        cout << "\033[0m";

    }
}

//Print the rack like 1:A(1) 2:T(1) 3:Q(10)
void Renderer::printRack(const TileRack &rack) {
    if (rack.empty()) {
        cout << "Rack is empty." << endl;
        return;
    }

    const int innerWidth = 5; //width inside the tile (between | |)
    const int tileWidth = innerWidth + 2; //including borders

    string topLine = "  ";
    string letterLine = "  ";
    string pointsLine = "  ";
    string bottomLine = "  ";
    string indexLine = "  ";

    for (size_t i = 0; i < rack.size(); ++i) {
        const Tile &t = rack[i];

        topLine += "+-----";
        bottomLine += "+-----";

        // Letter (centered).
        char letterChar = (t.letter == '?') ? ' ' : t.letter;
        string letterStr(1, letterChar);
        string centeredLetter = centerText(letterStr, innerWidth);
        letterLine += "|" + centeredLetter;

        // Points (Bottom - right)
        string pointsStr = to_string(t.points);
        int spacesBefore = innerWidth - static_cast<int>(pointsStr.size());
        if (spacesBefore < 0) {
            spacesBefore = 0;
        }
        string pointsInner(spacesBefore, ' ');
        pointsInner += pointsStr;
        pointsLine += "|" + pointsInner;

        // Indices under tiles (starting from 1)
        string indexStr = to_string(static_cast<int>(i)+1);
        indexLine += centerText(indexStr, innerWidth+1);
    }

    topLine += "+";
    bottomLine += "+";
    letterLine += "|";
    pointsLine += "|";

    cout << "\nYour rack:\n";
    cout << topLine << endl;
    cout << letterLine << endl;
    cout << pointsLine << endl;
    cout << bottomLine << endl;
    cout << indexLine << endl;
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

// Show unseen tiles
void Renderer::showUnseenTiles(const TileBag &bag, const Player players[2], int currentPlayer) {

    int opponent = 1 - currentPlayer;
    bool revealOpponent = (static_cast<int>(bag.size()) <= 7);

    printTileBag(bag, players[opponent].rack, revealOpponent);
}

void Renderer::clearScreen() {
    // for Windows
#ifdef _WIN32
    system("cls");
    // for Linux/macOS (ANSI terminal)
#else
    // ANSI clear: erase screen, move cursor to home
    cout << "\033[2J\033[H";
#endif
}

// wait until user presses Q or q
void Renderer::waitForQuitKey() {
    cout << "\n\033[1;33mPress 'Q' to return...\033[0m\n";

    while (true) {
        char ch;
        if (!(cin >> ch)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        if (ch == 'Q') {
            break;
        }
        cout << "\n\033[1;33mPress 'Q' to return...\033[0m\n";
    }
}


