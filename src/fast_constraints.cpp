/*
 * OPTIMIZED: Graph Tunneling Implementation
 * CONCEPT: Instead of iterating A-Z, we traverse the GADDAG path defined by neighbors.
 * SPEEDUP: O(26) -> O(1) (Average case: 2-3 edge checks per square)
 */

#include "../include/fast_constraints.h"
#include "../include/engine/dictionary.h"
#include <iostream>
#include <string>
#include <bit> // Requires C++20, enables std::countr_zero

using namespace std;

// Helper: Convert char to 0-25 index
static inline int toIdx(char c) {
    return (c & 31) - 1; // "Optimization Hack": A(65) & 31 = 1. 1-1=0. Fast toIdx.
}

const int SEPERATOR = 26;

// Helper: Check if a suffix path exists from a specific node
// This is the "Forward Check" (Downwards)
bool canTraverseSuffix(int nodeIdx, const LetterBoard &letters, int startRow, int col) {
    int curr = nodeIdx;
    int r = startRow;
    while (r < 15 && letters[r][col] != ' ') {
        curr = gDictionary.getChild(curr, toIdx(letters[r][col]));
        if (curr == -1) return false;
        r++;
    }
    return gDictionary.nodes[curr].isEndOfWord;
}

CharMask ConstraintGenerator::computeCrossCheck(const LetterBoard &letters, int row, int col) {
    // 1. Identify Neighbors
    bool hasUp = (row > 0 && letters[row-1][col] != ' ');
    bool hasDown = (row < 14 && letters[row+1][col] != ' ');

    // BASE CASE: No neighbors = No vertical constraints
    if (!hasUp && !hasDown) {
        return MASK_ANY;
    }

    CharMask allowed = MASK_NONE;

    // CASE 1: Upper Neighbors Exists
    // Strategy: Anchor at the letter immediately ABOVE.
    // Path: Rev(Upper) -> SEPERATOR -> [Candidate] -> Lower
    if (hasUp) {
        int curr = gDictionary.rootIndex;
        int r = row - 1;
        
        // TUNNELING UP: Traverse Rev(Upper)
        // We scan the board upwards and walk the graph simultaneously.
        while (r >= 0 && letters[r][col] != ' ') {
            curr = gDictionary.getChild(curr, toIdx(letters[r][col]));
            if (curr == -1) return MASK_NONE; // Board contains invalid word (impossible?)
            r--;
        }

        // CROSS THE GAP: The Separator
        curr = gDictionary.getChild(curr, SEPERATOR);
        if (curr == -1) return MASK_NONE;

        // EXPAND: The edges of 'curr' are the ONLY valid candidates by prefix.
        // We filter these candidates by checking if they connect to 'Lower'.
        uint32_t edges = gDictionary.nodes[curr].edgeMask;

        while (edges) {
            // "Bit Twiddling": Extract lowest set bit index efficiently
            int candidateIdx = std::countr_zero(edges);
            
            // Speculate: If we place this candidate...
            int nextNode = gDictionary.getChild(curr, candidateIdx);
            
            bool isValid = false;
            if (hasDown) {
                // Must complete the lower suffix
                isValid = canTraverseSuffix(nextNode, letters, row + 1, col);
            } else {
                // No lower suffix? Just need to be a valid word end.
                isValid = gDictionary.nodes[nextNode].isEndOfWord;
            }

            if (isValid) {
                allowed |= (1 << candidateIdx);
            }

            // Remove bit and continue
            edges &= ~(1 << candidateIdx);
        }

        return allowed;
    }

    // CASE 2: No Upper, Only Lower
    // Strategy: Anchor at the LAST letter of Lower.
    // Path: Rev(Lower) -> SEPERATOR -> [Candidate] -> (EndOfWord)
    // Note: Since Upper is empty, the Candidate is the start of the word.
    if (hasDown) {
        // Find the bottom of the Lower string
        int endRow = row + 1;
        while (endRow < 15 && letters[endRow][col] != ' ') {
            endRow++;
        }
        endRow--; // Point to last actual letter

        int curr = gDictionary.rootIndex;
        
        // TUNNELING UP (through the Lower string backwards)
        for (int r = endRow; r >= row + 1; r--) {
            curr = gDictionary.getChild(curr, toIdx(letters[r][col]));
            if (curr == -1) return MASK_NONE;
        }

        // CROSS THE GAP
        curr = gDictionary.getChild(curr, SEPERATOR);
        if (curr == -1) return MASK_NONE;

        // EXPAND: Edges are candidates.
        // Since Upper is empty, the Candidate MUST complete the word immediately.
        // (i.e., The path in GADDAG ends after the Candidate)
        uint32_t edges = gDictionary.nodes[curr].edgeMask;
        
        while (edges) {
            int candidateIdx = std::countr_zero(edges);
            int nextNode = gDictionary.getChild(curr, candidateIdx);
            
            // Check if this path ends here.
            // Path: ... -> Sep -> Candidate. Is Candidate EndOfWord?
            if (gDictionary.nodes[nextNode].isEndOfWord) {
                allowed |= (1 << candidateIdx);
            }

            edges &= ~(1 << candidateIdx);
        }

        return allowed;
    }

    return MASK_ANY;
}

RowConstraint ConstraintGenerator::generateRowConstraint(const LetterBoard &letters, int rowIdx) {
    RowConstraint rowData;

    // Prefetching row data for cache locality could go here, 
    // but the main bottleneck is the CrossCheck.

    for (int col = 0; col < BOARD_SIZE; col++) {
        // 1. Occupied Square
        if (letters[rowIdx][col] != ' ') {
            int idx = toIdx(letters[rowIdx][col]);
            rowData.masks[col] = (1 << idx); // Only the existing letter is valid
        } 
        // 2. Empty Square
        else {
            rowData.masks[col] = computeCrossCheck(letters, rowIdx, col);
        }
    }

    return rowData;
}