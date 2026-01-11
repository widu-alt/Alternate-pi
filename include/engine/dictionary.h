#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <bit>
#include <iostream>

// 26 Letters + 1 Seperator ('^')
#define LETTER_COUNT 27

using namespace std;

// Orpheus Graph Node
struct DawgNode {
    uint32_t edgeMask;
    uint32_t subtreeMask;
    int firstChildIndex;
    bool isEndOfWord;
};

class Dictionary {
public:
    vector<DawgNode> nodes;
    vector<int> childrenPool;

    // --- RAW ACCESS POINTERS (The Speed Hack) ---
    // These bypass vector::operator[] overhead in hot loops.
    const DawgNode* nodePtr = nullptr;
    const int* childPtr = nullptr;

    int rootIndex = 0;

    Dictionary() {}

    // Call this after loading/building to lock in the pointers
    void prepareOptimizedPointers() {
        if (!nodes.empty()) nodePtr = nodes.data();
        if (!childrenPool.empty()) childPtr = childrenPool.data();
    }

    bool loadFromFile(const string &filename);

    // --- HOT PATH: INLINED TRAVERSAL ---

    // Fast Indexer (Handles A-Z, a-z, and ^)
    static inline int fastIndex(char c) {
        if (c == '^') return 26;
        return (c & 31) - 1; // A=1 -> 0
    }

    inline int getChild(int nodeIdx, int letterIdx) const {
        // DIRECT MEMORY ACCESS (No vector overhead)
        const DawgNode& node = nodePtr[nodeIdx];

        if (!((node.edgeMask >> letterIdx) & 1)) return -1;

        uint32_t mask = (1 << letterIdx) - 1;

        // Compiler Intrinsic for Population Count
        #if defined(_MSC_VER)
            int offset = __popcnt(node.edgeMask & mask);
        #else
            int offset = __builtin_popcount(node.edgeMask & mask);
        #endif

        return childPtr[node.firstChildIndex + offset];
    }

    inline bool canPrune(int nodeIdx, uint32_t rackMask) const {
        // DIRECT MEMORY ACCESS
        return (nodePtr[nodeIdx].subtreeMask != 0) &&
               ((nodePtr[nodeIdx].subtreeMask & rackMask) == 0);
    }

    // Inlined Validation (Removes function call overhead in Referee)
    inline bool isValidWord(const string &word) const {
        if (word.empty()) return false;
        int curr = rootIndex;

        // Optimized Unrolled Check
        int idx0 = fastIndex(word[0]);
        curr = getChild(curr, idx0);
        if (curr == -1) return false;

        curr = getChild(curr, 26); // Separator
        if (curr == -1) return false;

        for (size_t i = 1; i < word.length(); i++) {
            curr = getChild(curr, fastIndex(word[i]));
            if (curr == -1) return false;
        }

        return nodePtr[curr].isEndOfWord;
    }

private:
    void buildFromWordList(const vector<string> &wordList);
    void insertGADDAG(const string &word);
    bool loadBinary(const string& filename);
    bool saveBinary(const string& filename);
};

extern Dictionary gDictionary;