#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <cctype>

namespace spectre {

class TileTracker {
private:
    // [CRITICAL FIX] Use flat array instead of std::map.
    // std::map allocates nodes on the heap. Accessing this in a hot loop
    // causes fragmentation and heap corruption.
    int counts[27];
    int totalCount;

public:
    TileTracker() { reset(); }

    void reset() {
        // Standard Scrabble Distribution
        // A-9, B-2, C-2, D-4, E-12, F-2, G-3, H-2, I-9, J-1, K-1, L-4, M-2
        // N-6, O-8, P-2, Q-1, R-6, S-4, T-6, U-4, V-2, W-2, X-1, Y-2, Z-1, ?-2
        static const int dist[] = {9,2,2,4,12,2,3,2,9,1,1,4,2,6,8,2,1,6,4,6,4,2,2,1,2,1,2};
        std::memcpy(counts, dist, 27 * sizeof(int));
        totalCount = 100;
    }

    void markSeen(char c) {
        int idx = -1;
        if (c == '?') idx = 26;
        else if (isalpha(c)) idx = toupper(c) - 'A';

        if (idx >= 0 && idx < 27) {
            if (counts[idx] > 0) {
                counts[idx]--;
                totalCount--;
            }
        }
    }

    void markSeen(const std::string& str) {
        for (char c : str) markSeen(c);
    }

    void remove(char c) { markSeen(c); }

    int getUnseenCount(char c) const {
        int idx = -1;
        if (c == '?') idx = 26;
        else if (isalpha(c)) idx = toupper(c) - 'A';

        if (idx >= 0 && idx < 27) return counts[idx];
        return 0;
    }

    int getTotalUnseen() const { return totalCount; }

    // [CRITICAL FIX] Populates an existing buffer. ZERO ALLOCATION.
    void populateRemainingTiles(std::vector<char>& buffer) const {
        buffer.clear();
        // Reserve once if needed, but in the loop, capacity will usually hold.
        if (buffer.capacity() < (size_t)totalCount) buffer.reserve(totalCount);

        for (int i = 0; i < 26; ++i) {
            for (int k = 0; k < counts[i]; ++k) buffer.push_back((char)('A' + i));
        }
        for (int k = 0; k < counts[26]; ++k) buffer.push_back('?');
    }

    // Legacy support (Allocates! Do not use in hot loops)
    std::vector<char> getRemainingTiles() const {
        std::vector<char> bag;
        populateRemainingTiles(bag);
        return bag;
    }
};
}