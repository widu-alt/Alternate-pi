#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <bit>

// 26 Letters + 1 Seperator ('^')
#define LETTER_COUNT 27

using namespace std;

// Orpheus Graph Node
struct DawgNode {
    uint32_t edgeMask; // Bit mask of immediate children (for fast iteration)
    uint32_t subtreeMask; // Bit mask of all future children (for Oracular Lookahead)
    int firstChildIndex; // Index into the global childrenPool
    bool isEndOfWord; // Flag

    DawgNode() : edgeMask(0), subtreeMask(0), firstChildIndex(-1), isEndOfWord(false) {}
};

class Dictionary {
public:
    vector<DawgNode> nodes;
    vector<int> childrenPool; // Compact storage for edges
    int rootIndex = 0;

    Dictionary();

    // The Unified Loader: Tries binary cache first, then text file
    bool loadFromFile(const string &filename);

    // Checks if a standard word exists (mostly for debugging/constraints)
    bool isValidWord(const string &word) const;

    inline int getChild(int nodeIdx, int letterIdx) const {
        // Check if bit is set (Does the child exist?)
        if (!((nodes[nodeIdx].edgeMask >> letterIdx) & 1)) return -1;

        // Counts bits set Before this letter to find the offset
        // (Hardware instruction: popcount)
        uint32_t mask = (1 << letterIdx) - 1;
        int offset = __builtin_popcount(nodes[nodeIdx].edgeMask & mask);

        return childrenPool[nodes[nodeIdx].firstChildIndex + offset];
    }

    inline bool canPrune(int nodeIdx, uint32_t rackMask) const {
        // if the branch requires letter I which is not in the rack, PRUNE
        if (nodes[nodeIdx].subtreeMask == 0) return false; // End of path

        // 0 overlap means have none of the letters required for this path. send true.
        return (nodes[nodeIdx].subtreeMask & rackMask) == 0;
    }

private:
    // Loads the dictionary file and builds the GADDAG
    // Build from text list -> Compress -> Save to Binary
    void buildFromWordList(const vector<string> &wordList);

    //Insert a word into the graph in GADDAG format (rotations with separator)
    void insertGADDAG(const string &word);

    // Load dictionary from Binary
    bool loadBinary(const string& filename);
    bool saveBinary(const string& filename);
};

// Global access to AI's brain
extern Dictionary gDictionary;
