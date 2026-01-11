#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <bit> // for std::countr_zero
#include "engine/board.h"
#include "engine/dictionary.h" // Required for gDictionary access

using namespace std;

// A bitmask representing allowed letters for a specific square.
using CharMask = uint32_t;

constexpr CharMask MASK_ANY = 0x03ffffff;
constexpr CharMask MASK_NONE = 0x00000000;
const int SEPERATOR = 26;

// Helper for fast char->index
static inline int fastToIdx(char c) {
    return (c & 31) - 1;
}

struct RowConstraint {
    array < CharMask, BOARD_SIZE > masks;
    uint16_t anchorMask = 0;

    RowConstraint() {
        masks.fill(MASK_ANY);
        anchorMask = 0;
    }

    inline bool isAllowed(int col, char letter) const {
        if (col < 0 || col >= BOARD_SIZE) return false;
        int idx = letter - 'A';
        if (idx < 0 || idx > 25) return false;
        return (masks[col] >> idx) & 1;
    }
};

class ConstraintGenerator {
private:
    // Helper: Check suffix (Downwards)
    static inline bool canTraverseSuffix(int nodeIdx, const LetterBoard &letters, int startRow, int col) {
        int curr = nodeIdx;
        int r = startRow;
        while (r < 15 && letters[r][col] != ' ') {
            curr = gDictionary.getChild(curr, fastToIdx(letters[r][col]));
            if (curr == -1) return false;
            r++;
        }
        return gDictionary.nodePtr[curr].isEndOfWord;
    }

    // INLINED: Compute Vertical Constraints (Tunneling)
    static inline CharMask computeCrossCheck(const LetterBoard &letters, int row, int col) {
        bool hasUp = (row > 0 && letters[row-1][col] != ' ');
        bool hasDown = (row < 14 && letters[row+1][col] != ' ');

        // Optimization: If no vertical neighbors, anything goes!
        if (!hasUp && !hasDown) return MASK_ANY;

        CharMask allowed = MASK_NONE;

        // 1. ANCHOR UP (Reverse Traversal)
        if (hasUp) {
            int curr = gDictionary.rootIndex;
            int r = row - 1;

            while (r >= 0 && letters[r][col] != ' ') {
                curr = gDictionary.getChild(curr, fastToIdx(letters[r][col]));
                if (curr == -1) return MASK_NONE;
                r--;
            }

            curr = gDictionary.getChild(curr, SEPERATOR);
            if (curr == -1) return MASK_NONE;

            uint32_t edges = gDictionary.nodePtr[curr].edgeMask;
            while (edges) {
                int candidateIdx = std::countr_zero(edges);
                int nextNode = gDictionary.getChild(curr, candidateIdx);

                bool isValid = false;
                if (hasDown) isValid = canTraverseSuffix(nextNode, letters, row + 1, col);
                else isValid = gDictionary.nodePtr[nextNode].isEndOfWord;

                if (isValid) allowed |= (1 << candidateIdx);
                edges &= ~(1 << candidateIdx);
            }
            return allowed;
        }

        // 2. ANCHOR DOWN (No Upper)
        if (hasDown) {
            int endRow = row + 1;
            while (endRow < 15 && letters[endRow][col] != ' ') endRow++;
            endRow--;

            int curr = gDictionary.rootIndex;
            for (int r = endRow; r >= row + 1; r--) {
                curr = gDictionary.getChild(curr, fastToIdx(letters[r][col]));
                if (curr == -1) return MASK_NONE;
            }

            curr = gDictionary.getChild(curr, SEPERATOR);
            if (curr == -1) return MASK_NONE;

            uint32_t edges = gDictionary.nodePtr[curr].edgeMask;
            while (edges) {
                int candidateIdx = std::countr_zero(edges);
                int nextNode = gDictionary.getChild(curr, candidateIdx);

                if (gDictionary.nodePtr[nextNode].isEndOfWord) allowed |= (1 << candidateIdx);
                edges &= ~(1 << candidateIdx);
            }
            return allowed;
        }

        return MASK_ANY;
    }

public:
    // INLINED: Generate Constraints for a Row
    static inline RowConstraint generateRowConstraint(const LetterBoard &letters, int rowIdx) {
        RowConstraint rowData;

        // 1. Compute Occupancy Masks
        uint16_t rowOcc = 0;
        uint16_t upOcc = 0;
        uint16_t downOcc = 0;

        for(int c=0; c<15; c++) {
            if (letters[rowIdx][c] != ' ') rowOcc |= (1 << c);
            if (rowIdx > 0 && letters[rowIdx-1][c] != ' ') upOcc |= (1 << c);
            if (rowIdx < 14 && letters[rowIdx+1][c] != ' ') downOcc |= (1 << c);
        }

        // 2. Identify "Dangerous" Columns (Empty squares with vertical neighbors)
        // These are implicitly anchors because they have cross-checks.
        uint16_t dangerousCols = (upOcc | downOcc) & (~rowOcc);

        // 3. Compute Neighbor Anchors (Empty squares next to tiles)
        uint16_t neighborCols = ((rowOcc << 1) | (rowOcc >> 1)) & (~rowOcc) & 0x7FFF;

        // 4. Combine to form the Master Anchor Mask
        // An anchor is: Dangerous (CrossCheck) OR Neighbor (Hook)
        rowData.anchorMask = dangerousCols | neighborCols;

        // Special Case: Start of Game (Center Star)
        // If row is empty and no vertical neighbors, but it's row 7, then (7,7) is valid.
        // Note: A truly empty board has rowOcc=0 and dangerousCols=0.
        if (rowIdx == 7 && rowOcc == 0 && dangerousCols == 0) {
            rowData.anchorMask |= (1 << 7);
        }

        // 5. Fill Constraints (Optimized Loops)
        uint16_t existing = rowOcc;
        while(existing) {
            int col = std::countr_zero(existing);
            int idx = fastToIdx(letters[rowIdx][col]);
            rowData.masks[col] = (1 << idx);
            existing &= ~(1 << col);
        }

        while(dangerousCols) {
            int col = std::countr_zero(dangerousCols);
            rowData.masks[col] = computeCrossCheck(letters, rowIdx, col);
            dangerousCols &= ~(1 << col);
        }

        return rowData;
    }
};