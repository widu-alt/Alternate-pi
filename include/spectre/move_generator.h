#pragma once

#include <vector>
#include <string>
#include "../engine/board.h"
#include "../../include/engine/rack.h"
#include "../../include/fast_constraints.h"
#include "../engine/dictionary.h"

using namespace std;

namespace spectre {

// Lightweight Move Structure (POD - Plain Old Data)
// Designed to be created on the stack with zero overhead.
struct MoveCandidate {
    short row;
    short col;
    short score;
    bool isHorizontal;
    char word[16];
    char leave[8]; // Remaining tiles
};

class MoveGenerator {
public:
    // -------------------------------------------------------------------------
    // OPTIMIZED INTERFACE: Raw Rack Counts
    // -------------------------------------------------------------------------
    // Accepts int[27] directly. Zero allocations.
    // -------------------------------------------------------------------------
    template <typename Consumer>
    static void generate_raw(const LetterBoard &board, int* rackCounts, Dictionary &dict, Consumer& consumer) {

        // 1. Transpose Board (Stack Allocation)
        LetterBoard transposed;
        for (int r = 0; r < 15; r++) {
            for (int c = 0; c < 15; c++) {
                transposed[c][r] = board[r][c];
            }
        }

        // 2. Pre-calculate Constraints (Stack Allocation)
        RowConstraint constraintsH[15];
        RowConstraint constraintsV[15];
        for(int i=0; i<15; i++) {
            constraintsH[i] = ConstraintGenerator::generateRowConstraint(board, i);
            constraintsV[i] = ConstraintGenerator::generateRowConstraint(transposed, i);
        }

        // 3. Execute Search
        int localRack[27];

        // Horizontal
        for (int r = 0; r < 15; r++) {
            memcpy(localRack, rackCounts, 27 * sizeof(int));
            if (!genMovesGADDAG(r, board, localRack, constraintsH[r], true, dict, consumer)) return;
        }

        // Vertical
        for (int r = 0; r < 15; r++) {
            memcpy(localRack, rackCounts, 27 * sizeof(int));
            if (!genMovesGADDAG(r, transposed, localRack, constraintsV[r], false, dict, consumer)) return;
        }
    }

    // -------------------------------------------------------------------------
    // STANDARD INTERFACE (TileRack Wrapper)
    // -------------------------------------------------------------------------
    template <typename Consumer>
    static void generate_custom(const LetterBoard &board, const TileRack &rack, Dictionary &dict, Consumer& consumer) {
        int rackCounts[27] = {0};
        for (const Tile& t : rack) {
            if (t.letter == '?') rackCounts[26]++;
            else if (isalpha(t.letter)) rackCounts[toupper(t.letter) - 'A']++;
        }
        generate_raw(board, rackCounts, dict, consumer);
    }

    // Legacy Compatibility Wrapper
    // Used by Speedi_Pi where we actually want a sorted vector of all moves.
    static vector<MoveCandidate> generate(const LetterBoard &board,
                                          const TileRack &rack,
                                          Dictionary &dict,
                                          bool useThreading = true);

private:

    static const int SEPERATOR = 26;

    // Helper: Recomputes the bitmask of tiles currently in the rack
    static uint32_t getRackMask(int* rackCounts) {
        uint32_t mask = 0;
        if (rackCounts[26] > 0) return 0xFFFFFFFF; // Blank allows everything
        for (int i = 0; i < 26; i++) {
            if (rackCounts[i] > 0) mask |= (1 << i);
        }
        return mask;
    }

    // -------------------------------------------------------------------------
    // TEMPLATED RECURSION (Inlined for Performance)
    // -------------------------------------------------------------------------

    // Entry point for a specific row
    template <typename Consumer>
    static bool genMovesGADDAG(int row, const LetterBoard &board, int *rackCounts,
                              const RowConstraint &constraints, bool isHorizontal,
                              Dictionary& dict, Consumer& consumer) {

        // Calculate what is already on the board in this row
        uint32_t boardRowMask = 0;
        for(int c = 0; c < 15; c++) {
            if(board[row][c] != ' ') boardRowMask |= (1 << (board[row][c] - 'A'));
        }

        // Initial Pruning Mask: Rack + Board + Separator
        uint32_t myRackMask = getRackMask(rackCounts);
        uint32_t pruningMask = myRackMask | boardRowMask | (1 << SEPERATOR);

        char wordBuf[20]; // Recursive word buffer

        // Iterate over potential Anchors
        for (int c = 0; c < 15; c++) {
            if (board[row][c] != ' ') continue; // Anchors must be empty

            // Definition of an Anchor: Adjacent to existing tile or specific constraints
            bool isAnchor = false;
            if (c > 0 && board[row][c-1] != ' ') isAnchor = true;
            else if (c < 14 && board[row][c+1] != ' ') isAnchor = true;
            else if (constraints.masks[c] != MASK_ANY) isAnchor = true;
            else if (row == 7 && c == 7) isAnchor = true; // Start square

            if (!isAnchor) continue;

            wordBuf[0] = '\0';
            // Start GADDAG traversal (Going Left)
            if (!goLeft(row, c, dict.rootIndex, constraints, myRackMask, pruningMask,
                rackCounts, wordBuf, 0, board, isHorizontal, c, dict, consumer)) return false;
        }
        return true;
    }

    // Recursive Step: Going Left (building prefix backwards)
    template <typename Consumer>
    static bool goLeft(int row, int col, int node, const RowConstraint &constraints,
                  uint32_t rackMask, uint32_t pruningMask, int* rackCounts,
                  char *wordBuf, int wordLen, const LetterBoard &board,
                  bool isHoriz, int anchorCol, Dictionary& dict, Consumer& consumer) {

        // Check if we can turn around (switch to goRight)
        bool canStopGoingLeft = (col < 0) || (board[row][col] == ' ');

        if (canStopGoingLeft) {
            int sepIndex = SEPERATOR;
            // Check if the current node has a Separator edge
            if ((dict.nodes[node].edgeMask >> sepIndex) & 1) {
                int separatorNode = dict.getChild(node, sepIndex);

                // Reverse the buffer to get the correct prefix
                char rightBuf[20];
                for (int i=0; i<wordLen; i++) rightBuf[i] = wordBuf[wordLen - 1 - i];

                // Switch to goRight
                if (!goRight(row, anchorCol + 1, separatorNode, constraints, rackMask, pruningMask,
                    rackCounts, rightBuf, wordLen, board, isHoriz, anchorCol, dict, consumer)) return false;
            }
        }

        if (col < 0) return true; // Hit edge of board

        char boardChar = (col >= 0) ? board[row][col] : ' ';
        uint32_t boardMask = (col >= 0) ? constraints.masks[col] : 0;
        uint32_t effectiveMask = dict.nodes[node].edgeMask;

        if (boardChar != ' ') {
            // Existing Tile: Must match
            int charIdx = boardChar - 'A';
            if (!((effectiveMask >> charIdx) & 1)) return true;

            wordBuf[wordLen] = boardChar;
            return goLeft(row, col - 1, dict.getChild(node, charIdx), constraints, rackMask, pruningMask,
                rackCounts, wordBuf, wordLen + 1, board, isHoriz, anchorCol, dict, consumer);
        } else {
            // Empty Square: Place Tile
            effectiveMask &= (rackMask & boardMask); // Filter by Rack and Constraints

            while (effectiveMask) {
                int i = __builtin_ctz(effectiveMask); // Find next available letter
                char letter = (char)('A' + i);
                int nextNode = dict.getChild(node, i);

                // ORACLE PRUNING: Check if this branch leads to dead ends given our rack
                if (!dict.canPrune(nextNode, pruningMask)) {
                    // Consume Tile
                    bool usedBlank = false;
                    if (rackCounts[i] > 0) rackCounts[i]--;
                    else if (rackCounts[26] > 0) { rackCounts[26]--; usedBlank = true; letter = tolower(letter); }
                    else { effectiveMask &= ~(1 << i); continue; }

                    wordBuf[wordLen] = letter;

                    // Update Masks for children (Lookahead Pruning)
                    uint32_t nextRackMask = rackMask;
                    if (usedBlank && rackCounts[26] == 0) nextRackMask = getRackMask(rackCounts);
                    else if (!usedBlank && rackCounts[i] == 0) nextRackMask &= ~(1 << i);

                    uint32_t nextPruningMask = nextRackMask | (pruningMask & ~rackMask) | (1 << SEPERATOR);

                    // Recurse
                    if (!goLeft(row, col - 1, nextNode, constraints, nextRackMask, nextPruningMask,
                        rackCounts, wordBuf, wordLen + 1, board, isHoriz, anchorCol, dict, consumer)) return false;

                    // Backtrack
                    if (usedBlank) rackCounts[26]++; else rackCounts[i]++;
                }
                effectiveMask &= ~(1 << i);
            }
        }
        return true;
    }

    // Recursive Step: Going Right (finishing the word)
    template <typename Consumer>
    static bool goRight(int row, int col, int node, const RowConstraint &constraints,
                   uint32_t rackMask, uint32_t pruningMask, int* rackCounts,
                   char *wordBuf, int wordLen, const LetterBoard &board,
                   bool isHoriz, int anchorCol, Dictionary& dict, Consumer& consumer) {

        // 1. Found a Valid Word?
        if (dict.nodes[node].isEndOfWord) {
            // Ensure we aren't merging with another word improperly
            if ((col > 14) || (board[row][col] == ' ')) {
                MoveCandidate cand;
                cand.isHorizontal = isHoriz;
                // Calculate Start Position based on length
                cand.row = (short)(isHoriz ? row : (col - wordLen));
                cand.col = (short)(isHoriz ? (col - wordLen) : row);
                cand.score = 0; // Default

                // Fast Memcpy (No string overhead)
                memcpy(cand.word, wordBuf, wordLen);
                cand.word[wordLen] = '\0';
                cand.leave[0] = '\0'; // Leave is lazy-loaded by Consumer

                // CALL THE CONSUMER
                if (!consumer(cand, rackCounts)) return false; // Stop if consumer is satisfied
            }
        }

        if (col > 14) return true;

        char boardChar = board[row][col];
        uint32_t boardMask = constraints.masks[col];
        uint32_t effectiveMask = dict.nodes[node].edgeMask;

        if (boardChar != ' ') {
            // Existing Tile
            int charIdx = boardChar - 'A';
            if (!((effectiveMask >> charIdx) & 1)) return true;

            wordBuf[wordLen] = boardChar;
            return goRight(row, col + 1, dict.getChild(node, charIdx), constraints, rackMask, pruningMask,
                rackCounts, wordBuf, wordLen + 1, board, isHoriz, anchorCol, dict, consumer);
        } else {
            // Empty Square
            effectiveMask &= (rackMask & boardMask);
            while (effectiveMask) {
                int i = __builtin_ctz(effectiveMask);
                char letter = (char)('A' + i);
                int nextNode = dict.getChild(node, i);

                if (!dict.canPrune(nextNode, pruningMask)) {
                    bool usedBlank = false;
                    if (rackCounts[i] > 0) rackCounts[i]--;
                    else if (rackCounts[26] > 0) { rackCounts[26]--; usedBlank = true; letter = tolower(letter); }
                    else { effectiveMask &= ~(1 << i); continue; }

                    wordBuf[wordLen] = letter;

                    uint32_t nextRackMask = rackMask;
                    if (usedBlank && rackCounts[26] == 0) nextRackMask = getRackMask(rackCounts);
                    else if (!usedBlank && rackCounts[i] == 0) nextRackMask &= ~(1 << i);

                    uint32_t nextPruningMask = nextRackMask | (pruningMask & ~rackMask) | (1 << SEPERATOR);

                    if (!goRight(row, col + 1, nextNode, constraints, nextRackMask, nextPruningMask,
                        rackCounts, wordBuf, wordLen + 1, board, isHoriz, anchorCol, dict, consumer)) return false;

                    if (usedBlank) rackCounts[26]++; else rackCounts[i]++;
                }
                effectiveMask &= ~(1 << i);
            }
        }
        return true;
    }
};

}