# Move Generation Algorithm (Backtracking)

This algorithm uses the **GADDAG** to find every possible valid move on the board.

## 1. The Concept: "Anchors"
An **Anchor** is any empty square that is adjacent to an existing tile on the board.
The AI iterates through every row and column. When it finds an anchor, it tries to "grow" a word from that spot using the GADDAG.

## 2. Pseudo-Code

```cpp
// Main function to find all moves
void FindAllMoves(Board board, Rack rack, GADDAG gaddag) {
    
    // 1. Pre-calculate "Cross-Checks"
    // For every empty square, determine which letters are valid 
    // if we place them there (based on vertical neighbors).
    // e.g., if 'Q' is above, only 'U', 'I', 'A' might be valid.
    auto crossChecks = CalculateCrossChecks(board);

    // 2. Iterate over every square
    for (int row = 0; row < 15; row++) {
        for (int col = 0; col < 15; col++) {
            
            // If this is an "Anchor" (empty but next to a tile)
            if (IsAnchor(board, row, col)) {
                
                // Start generating words from the GADDAG root
                // We start by going "Left" (Backwards in GADDAG)
                Gen(row, col, "", rack, gaddag.root, LEFT, crossChecks);
            }
        }
    }
}

// Recursive Backtracking Function
void Gen(int r, int c, string currentWord, Rack currentRack, Node* node, Direction dir, CrossChecks checks) {

    // A. If we have formed a valid word in the GADDAG
    if (node->isEndOfWord && IsValidMoveOnBoard(r, c)) {
        RecordMove(currentWord, Score(currentWord));
    }

    // B. Try to extend the word
    
    // Case 1: The square (r,c) is EMPTY
    if (board[r][c] == EMPTY) {
        
        // Try every tile in our rack
        for (Tile t : currentRack) {
            
            // 1. Check if this letter exists in the GADDAG path
            if (node->children.has(t.letter)) {
                
                // 2. Check if this letter is allowed by Cross-Checks
                if (checks[r][c].allows(t.letter)) {
                    
                    // Recurse!
                    // Remove tile from rack, move to next square
                    Gen(next_r, next_c, currentWord + t.letter, currentRack - t, node->children[t.letter], dir, checks);
                }
            }
        }
        
        // Special Case: Switch direction from Left to Right
        // If we were going Left, we can hit the '+' separator in GADDAG 
        // and start going Right from the original anchor.
        if (dir == LEFT && node->children.has('+')) {
             Gen(original_anchor_r, original_anchor_c + 1, currentWord, currentRack, node->children['+'], RIGHT, checks);
        }
    }
    
    // Case 2: The square (r,c) is OCCUPIED (Existing tile on board)
    else {
        char existingLetter = board[r][c];
        
        // We MUST match the existing letter
        if (node->children.has(existingLetter)) {
            Gen(next_r, next_c, currentWord + existingLetter, currentRack, node->children[existingLetter], dir, checks);
        }
    }
}
```

## 3. Why this is fast
Instead of checking millions of random words, this algorithm only follows paths that:
1.  Actually exist in the dictionary (GADDAG).
2.  Actually fit on the board (Cross-Checks).
3.  Actually use tiles we have (Rack).

It prunes the search tree massively, allowing it to find the best move in milliseconds.