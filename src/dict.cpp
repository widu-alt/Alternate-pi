#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>
#include <cctype>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include "../include/board.h"
#include "../include/move.h"
#include "../include/tiles.h"
#include "../include/rack.h"
#include "../include/dict.h"
#include "../include/dictionary.h"

using namespace std;

unordered_set<string> gDictionary;

/* to detect exe directory and append relative file path
string getExecutableDirectory() {
#ifdef _WIN32
    char buffer[PATH_MAX];
    GetModuleFileName(NULL, buffer, PATH_MAX);
    filesystem::path exePath(buffer);
    return exePath.parent_path().string();
#else
    char buffer[1024];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (count != -1) {
        filesystem::path exePath(string(buffer, count));
        return exePath.parent_path().string();
    }
    return ".";
#endif
}
*/

bool loadDictionary(const string &filename) {

    if (gDawg.loadBinary("gaddag.bin")){}

    vector<string> searchPaths = {
        "",
        "data/",
        "build/Release/data/",
        "../data/",
        "../../data/",
        "../../../data/"
    };

    ifstream in;
    string foundPath;

    for (const auto& prefix : searchPaths) {
        string fullPath = prefix + filename;

        in.clear();
        in.close();

        in.open(fullPath);
        if (in.is_open()) {
            foundPath = fullPath;
            break;
        }
    }

    if (!in.is_open()) {
        cout << "Failed to open dictionary file: " << filename << "\n";
        cout << "Searched in:\n";
        for (const auto& prefix : searchPaths) {
            cout << "  " << prefix + filename << "\n";
        }
        return false;
    }

    gDictionary.clear();
    vector<string> wordList; // temp list

    string word;

    // C++ Learning
    // Reads the next whitespace-delimited token from the file "in", stores it in word,
    //-and continues until the file ends or read fails.
    while (in >> word) {

        // Normalize the word
        string cleanWord;
        for (char &c : word) {
            if (isalpha(c)) {
                cleanWord += static_cast<char>(toupper(static_cast<unsigned char>(c)));
            }
        }

        if (!cleanWord.empty()) {
            gDictionary.insert(cleanWord);
            wordList.push_back(cleanWord); // storing
        }
    }

    // Only build GADDAG if we didnt load it from binary
    if (gDawg.nodes.empty()) {
        gDawg.buildFromWordList(wordList);
        gDawg.saveBinary("gaddag.bin"); // Cache it for next time
    }

    cout << "\nLoaded " << gDictionary.size() << " words from " << foundPath << "\n";

    return true;
}

bool isValidWord(const string &word) {
    string up = word;
    for (char &c : up) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    // C++ Learning
    // gDictionary.find(up) looks for up in the dictionary, and if not found returns gDictionary.end()
    // which basically means the word is not there.
    return gDictionary.find(up) != gDictionary.end();
}

// Get the full word from the board (for challenging)
string extractMainWord(const LetterBoard &letters, int row, int col, bool horizontal) {
    int dr = horizontal ? 0 : 1;
    int dc = horizontal ? 1 : 0;

    int r = row;
    int c = col;

    // Move backwards to the start of the word
    while (r - dr >= 0 && r - dr < BOARD_SIZE && c - dc >= 0 && c - dc < BOARD_SIZE && letters[r - dr][c - dc] != ' ') {
        r = r - dr;
        c = c - dc;
    }

    // move forwards, collecting letters
    string word;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && letters[r][c] != ' ') {
        word.push_back(letters[r][c]);
        r += dr;
        c += dc;
    }

    return word;
}

// Returns a list of all the cross words formed.
vector<string> crossWordList(const LetterBoard &letters, const LetterBoard &oldLetters,
                             int row, int col, bool mainHorizontal) {
    vector<string> words;

    // perpendicular direction
    int crossDr = mainHorizontal ? 1 : 0;
    int crossDc = mainHorizontal ? 0 : 1;

    // for going from tile to tile
    int mainDr = mainHorizontal ? 0 : 1;
    int mainDc = mainHorizontal ? 1 : 0;

    int r = row;
    int c = col;

    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && letters[r][c] != ' ') {

        // If this tile existed on the old board, skip it
        if (oldLetters[r][c] != ' ') {
            r+= mainDr;
            c+= mainDc;
            continue;
        }

        // Check if there is any neighbor in that perpendicular direction
        bool hasNeighbor = false;

        // checking either side
        int nr = r - crossDr;
        int nc = c - crossDc;
        if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && letters[nr][nc] != ' ') {
            hasNeighbor = true;
        }

        nr = r + crossDr;
        nc = c + crossDc;
        if (!hasNeighbor && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && letters[nr][nc] != ' ') {
            hasNeighbor = true;
        }

        // Single tile with no adjacent letter in cross direction
        if (hasNeighbor) {
            // Finding start of the cross word.
            int cr = r;
            int cc = c;

            while (cr - crossDr >= 0 && cr - crossDr < BOARD_SIZE && cc - crossDc >= 0 &&
                   cc - crossDc < BOARD_SIZE && letters[cr - crossDr][cc - crossDc] != ' ') {
                cr = cr - crossDr;
                cc = cc - crossDc;
            }

            string word;
            // Walk forward along the cross-word direction
            int wr = cr;
            int wc = cc;
            while (wr >= 0 && wr < BOARD_SIZE && wc >= 0 &&
                   wc < BOARD_SIZE && letters[wr][wc] != ' ') {
                word.push_back(letters[wr][wc]);
                wr += crossDr;
                wc += crossDc;
            }

            if (word.size() > 1) {
                words.push_back(word);
            }
        }

        // Advance to the next cell along the main word.
        r += mainDr;
        c += mainDc;

    }

    cout << "No of cross words found: " << words.size() << "\n";

    return words;
}