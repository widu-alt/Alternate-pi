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