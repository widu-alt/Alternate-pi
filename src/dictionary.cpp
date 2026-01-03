#include "../include/dictionary.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

Dawg gDawg;

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

Dawg::Dawg() {
    // Creating the root node immediately so that the graph is never empty
    rootIndex = 0;
    //nodes.emplace_back(); // create root
}

// Helper to get array index
int getIndex(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c == '^') return SEPERATOR; // '^' represent the seperator internally.
    return -1;
}

void Dawg::insertGADDAG(const string &word) {

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
            int idx = getIndex(c);
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

void Dawg::buildFromWordList(const vector<string> &wordList) {
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

bool Dawg::saveBinary(const string &filename) {
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

bool Dawg::loadBinary(const string &filename) {
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

bool Dawg::isValidWord(const string &word) const {
    if (word.empty()) return false;
    int curr = rootIndex;

    // First letter
    int idx0 = getIndex(word[0]);
    curr = getChild(curr, idx0);
    if (curr == -1) return false;

    // Separator
    curr = getChild(curr, SEPERATOR);
    if (curr == -1) return false;

    for (int i = 1; i < word.length(); i++) {
        curr = getChild(curr, getIndex(word[i]));
        if (curr == -1) return false;
    }

    return nodes[curr].isEndOfWord;
}

/*
void Dawg::clear() {
    nodes.clear();
    nodes.emplace_back();
    rootIndex = 0;
}

void Dawg::buildFromWordList(const vector < string > & wordList) {
    cout << "Compiling Dictionary into Orpheus Graph..." << endl;

    clear();

    // Assumes the input 'wordList' is sorted alphabetically for a standard Trie construction.
    // (true DAWG minimization would happen here, but for V1 a trie is sufficient.
    // and safer to implement quickly. Will add node merging later to save RAM

    for (const string & word : wordList) {
        int currentNodeIndex = rootIndex;

        for (char c: word) {
            int letterIdx = toupper(c) - 'A';
            if (letterIdx < 0 || letterIdx > 25) {
                continue; //skip invalid letters
            }

            // Update the mask for the "Constraint Tunnel"
            // |= means adds the new bit to the existing mask without destroying existing bits
            // ex:- edgeMask = 00000101 (A and C exist)
            // adding 'D' -> 1 << 3 = 00001000
            // edgeMask |= 00001000
            // edgeMask = 00001101 ( A, C, D exist)
            nodes[currentNodeIndex].edgeMask |= (1 << letterIdx);

            // Traverse or Create
            if (nodes[currentNodeIndex].children[letterIdx] == -1) {
                // Create a new node
                nodes.emplace_back();
                int newNodeIndex = static_cast<int>(nodes.size()- 1);
                nodes[currentNodeIndex].children[letterIdx] = newNodeIndex;
                currentNodeIndex = newNodeIndex;
            } else {
                currentNodeIndex = nodes[currentNodeIndex].children[letterIdx];
            }
        }
        nodes[currentNodeIndex].isEndOfWord = true;
    }

    size_t ramUsage = nodes.size() * sizeof(DawgNode);
    cout << "Graph Compiled. Nodes " << nodes.size()
         << " (Memory: " << (ramUsage / 1024 / 1024) << " MB)" << endl;
}

bool Dawg::contains(const string & word) const {
    int current = rootIndex;
    for (char c: word) {
        int idx = toupper(c) - 'A';
        if (idx < 0 || idx > 25) {
            return false;
        }

        if (nodes[current].children[idx] == -1) {
            return false;
        }
        current = nodes[current].children[idx];
    }
    return nodes[current].isEndOfWord;
}
*/



























