#include "../../include/engine/dictionary.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

Dictionary gDictionary;

// We use index 26 as the 'Seperator'
const int SEPERATOR = 26;

struct TempNode {
    int children[27];
    bool isEndOfWord;
    uint32_t edgeMask;

    TempNode() {
        fill(begin(children), end(children), -1);
        isEndOfWord = false;
        edgeMask = 0;
    }
};

vector<TempNode> tempNodes;

// Internal Helper for building
int getIndexBuild(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c == '^') return SEPERATOR;
    return -1;
}

bool Dictionary::loadFromFile(const string &filename) {
    // 1. Try Binary Cache
    string binaryName = "gaddag.bin";
    if (loadBinary(binaryName)) {
        prepareOptimizedPointers(); // LOCK POINTERS
        return true;
    }

    // 2. Find Text File
    vector<string> searchPaths = {"", "data/", "../data/", "../../data/", "../../../data/"};
    ifstream in;
    string foundPath;

    for (const auto& prefix : searchPaths) {
        in.open(prefix + filename);
        if (in.is_open()) { foundPath = prefix + filename; break; }
        in.clear();
    }

    if (!in.is_open()) {
        cout << "[Error] Dictionary file not found: " << filename << endl;
        return false;
    }

    cout << "[Dict] Loading raw text from: " << foundPath << endl;

    // 3. Parse
    vector<string> wordList;
    string word;
    while (in >> word) {
        string clean;
        for (char c : word) if (isalpha(c)) clean += toupper(c);
        if (!clean.empty()) wordList.push_back(clean);
    }

    buildFromWordList(wordList);
    saveBinary(binaryName);

    prepareOptimizedPointers(); // LOCK POINTERS
    return true;
}

void Dictionary::insertGADDAG(const string &word) {

    for (int i = 0; i < word.length(); i++) {
        string prefix = word.substr(0, i+1);
        string suffix = "";
        if ( i < word.length() - 1 ) {
            suffix = word.substr(i+1);
        }

        // Reverse the prefix
        reverse(prefix.begin(), prefix.end());

        // Construct the GADDAG path: Rev(Prefix) + Seperator + Suffix
        string path = prefix + '^' + suffix;

        int currNode = 0; //temproot
        for (char c : path) {
            int idx = getIndexBuild(c);
            if (idx == -1) continue;

            if (tempNodes[currNode].children[idx] == -1) {
                tempNodes[currNode].children[idx] = tempNodes.size();
                tempNodes[currNode].edgeMask |= (1 << idx); // updating the mask
                tempNodes.emplace_back();
            }
            currNode = tempNodes[currNode].children[idx];
        }
        tempNodes[currNode].isEndOfWord = true;
    }
}

//Recursive helper to compute Subtree Masks (The Oracle)
uint32_t fillMasks(int nodeIdx, vector<uint32_t> &masks) {
    uint32_t myMask = 0;
    for (int i = 0; i < 27; i++) {
        int child = tempNodes[nodeIdx].children[i];
        if (child != -1) {
            myMask |= (1 << i); // I can reach this letter immediatly
            myMask |= fillMasks(child, masks); // I can reach everything my child can
        }
    }
    masks[nodeIdx] = myMask;
    return myMask;
}

void Dictionary::buildFromWordList(const vector<string> &wordList) {
    // 1. Build Phase
    tempNodes.clear();
    tempNodes.reserve(wordList.size() * 3);
    tempNodes.emplace_back(); // root

    cout << "[DAWG] Building GADDAG (Graph Phase)..." << endl;
    for (const string &word : wordList) {
        insertGADDAG(word);
    }

    // 2. Oracle Phase (Computing subtree masks)
    cout << "[DAWG] Computing Oracle Masks..." << endl;
    vector<uint32_t> tempSubtreeMasks(tempNodes.size());
    fillMasks(0, tempSubtreeMasks);

    // 3. Compression Phase
    cout << "[DAWG] Compressing..." << endl;
    nodes.resize(tempNodes.size());
    childrenPool.clear();
    childrenPool.reserve(tempNodes.size());

    for (size_t i = 0; i < tempNodes.size(); i++) {
        nodes[i].isEndOfWord = tempNodes[i].isEndOfWord;
        nodes[i].edgeMask = tempNodes[i].edgeMask;
        nodes[i].subtreeMask = tempSubtreeMasks[i]; // Store the invention!

        if (tempNodes[i].edgeMask != 0) {
            // "Where do my children start in the pool?" -> At the current end.
            nodes[i].firstChildIndex = childrenPool.size();

            // Push children in strict 0-26 order
            for (int bit = 0; bit < 27; bit++) {
                if ((tempNodes[i].edgeMask >> bit) & 1) {
                    childrenPool.push_back(tempNodes[i].children[bit]);
                }
            }
        } else {
            nodes[i].firstChildIndex = -1;
        }
    }

    // 4. Cleanup
    tempNodes.clear();
    tempNodes.shrink_to_fit();

    cout << "[DAWG] Final Size: " << nodes.size() << " nodes." << endl;
    size_t memSize = (nodes.size() * sizeof(DawgNode)) + (childrenPool.size() * sizeof(int));
    cout << "[DAWG] Memory Usage: " << memSize / (1024*1024) << " MB." << endl;
}

bool Dictionary::saveBinary(const string &filename) {
    ofstream out(filename, ios::binary);
    if (!out) return false;

    size_t nodeCount = nodes.size();
    size_t poolCount = childrenPool.size();

    // Write Counts
    out.write((char*)&nodeCount, sizeof(size_t));
    out.write((char*)&poolCount, sizeof(size_t));

    // Write Vectors
    out.write((char*)nodes.data(), nodeCount * sizeof(DawgNode));
    out.write((char*)childrenPool.data(), poolCount * sizeof(int));

    return out.good();
}

bool Dictionary::loadBinary(const string &filename) {
    ifstream in(filename, ios::binary);
    if (!in) return false;

    size_t nodeCount, poolCount;

    // Read Counts
    in.read((char*)&nodeCount, sizeof(size_t));
    in.read((char*)&poolCount, sizeof(size_t));

    nodes.resize(nodeCount);
    childrenPool.resize(poolCount);

    // Write Vectors
    in.read((char*)nodes.data(), nodeCount * sizeof(DawgNode));
    in.read((char*)childrenPool.data(), poolCount * sizeof(int));

    rootIndex = 0;
    cout << "[DAWG] Loaded binary (" << nodeCount << ") nodes, "
         << (poolCount*4)/(1024*1024) << " MB edges." << endl;
    return in.good();
}
























