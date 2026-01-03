#include "../include/fast_constraints.h"
#include "../include/dict.h"
#include "../include/dictionary.h"
#include <iostream>
#include <string>

using namespace std;

// Helper to conver char to bit index (0-25)
static inline int toIdx(char c) {
    return toupper(c) - 'A';
}

const int SEPERATOR = 26;

// Helper to verify if a suffix exists from a specific node
// Returns true if we can traverse suffix from nodeIdx and end at a valid word.
bool canTraverseSuffix(int nodeIdx, const string &suffix) {
    int curr = nodeIdx;
    for (char c: suffix) {
        int idx = toIdx(c);
        curr = gDawg.getChild(curr, idx);
        if (curr == -1) return false;
    }
    return gDawg.nodes[curr].isEndOfWord;
}

CharMask ConstraintGenerator::computeCrossCheck(const LetterBoard &letters, int row, int col) {
    // If there are no verticle neighbors, ANY letter is valid vertically.
    bool hasUp = (row > 0 && letters[row-1][col] != ' ');
    bool hasDown = (row < 14 && letters[row+1][col] != ' ');

    if (!hasUp && !hasDown) {
        return MASK_ANY;
    }

    string prefix = "";
    int r = row - 1;
    while (r >= 0 && letters[r][col] != ' ') {
        prefix += letters[r][col]; // Prepend to build correctly
        r--;
    }

    string suffix = "";
    r = row + 1;
    while (r < 15 && letters[r][col] != ' ') {
        suffix += letters[r][col];
        r++;
    }

    CharMask allowed = MASK_NONE;
    int root = gDawg.rootIndex;

    // Iterate All Candidates (A-Z)
    // We check: Candidate -> Prefix(Reverse) -> Sep -> Suffix
    for (int i = 0; i < 26; i++) {
        char candidate = (char)('A' + i);
        int curr = root;

        // A. Step 1: The Candidate (Anchor)
        curr = gDawg.getChild(curr, i);
        if (curr == -1) continue;

        // B. Step 2: The Prefix (Upwards/Reverse)
        bool prefixValid = true;
        for (char p : prefix) {
            curr = gDawg.getChild(curr, toIdx(p));
            if (curr == -1) {
                prefixValid = false;
                break;
            }
        }
        if (!prefixValid) continue;

        // C. Step 3: The Separator
        curr = gDawg.getChild(curr, SEPERATOR);
        if (curr == -1) continue;

        // D. Step 4: The Suffix (Downwards/Forward)
        if (canTraverseSuffix(curr, suffix)) {
            allowed |= (1 << i);
        }
    }

    return allowed;
}

RowConstraint ConstraintGenerator::generateRowConstraint(const LetterBoard &letters, int rowIdx) {
    RowConstraint rowData;

    for (int col = 0; col < BOARD_SIZE; col++) {
        // CASE 1: Square is already occupied
        if (letters[rowIdx][col] != ' ') {
            // Already occupied
            int idx = toIdx(letters[rowIdx][col]);
            rowData.masks[col] = (idx >= 0 && idx < 26) ? (1 << idx) : 0;
        }

        // Empty Square. Check vertical constraints.
        rowData.masks[col] = computeCrossCheck(letters, rowIdx, col);
    }

    return rowData;

}
























