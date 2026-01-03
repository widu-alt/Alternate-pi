LEXI\_PI Developer Documentation

Version: 1.0 (Stable Grandmaster Release) Engine Type: GADDAG-based Scrabble AI Language: C++17



1\. System Architecture

The project is divided into three distinct layers:



The Core Engine (src/dawg.cpp, src/board.cpp): Handles the physical reality of the game (dictionary, board state, tile placement).



The Intelligence Layer (src/ai\_player.cpp, src/fast\_constraints.cpp): Generates, scores, and selects moves.



The Game Loop Layer (src/modes/): Manages turns, UI, and game flow for different modes (PvP, PvE, AiAi).



Class Hierarchy

PlayerController (Abstract Base)



HumanPlayer: Handles CLI input.



AIPlayer: Handles algorithmic move generation.



Dawg: The Directed Acyclic Word Graph (Dictionary).



LetterBoard: A 15x15 2D array of characters.



ConstraintGenerator: Static helper for move validation logic.



2\. Core Algorithms

2.1. The GADDAG Data Structure

File: src/dawg.cpp Standard Tries are too slow for Scrabble because you often hook words onto existing tiles (e.g., placing 'K' before 'ITE' to make 'KITE'). We use a GADDAG (Gordon’s Acyclic Deterministic Finite Automaton).



Concept: Every word is stored in multiple "rotations" containing a specific Separator character (^).



Example: The word "CARE" is stored as 4 paths:



C ^ A R E (Start at C, go right)



A C ^ R E (Start at A, go left to C, then right to R, E)



R A C ^ E (Start at R, go left to A, C, then right to E)



E R A C ^ (Start at E, go left to R, A, C)



Why? This allows the AI to start generating a word at any letter already on the board (the "Anchor") and grow outwards in both directions simultaneously.



2.2. Bitwise Constraint Generation

File: src/fast\_constraints.cpp Before generating moves, we calculate a RowConstraint for the target row.



Cross-Check Mask: For every empty square, we calculate which letters are valid vertically.



Implementation: We construct a 32-bit integer (CharMask) where the Nth bit is 1 if the Nth letter of the alphabet forms a valid vertical word.



Usage: During horizontal recursion, if the AI tries to place 'Z' at square (3, 5), it checks: if (constraint\[5] \& (1 << 'Z')). If 0, it prunes the branch immediately.



2.3. The Move Generation Pipeline

File: src/ai\_player.cpp -> findAllMoves



Anchor Identification: Identify all "Active Squares" (empty squares adjacent to existing tiles).



Graph Traversal (genMovesGADDAG):



Iterate over every Anchor.



goLeft: Traverse the GADDAG backwards to form the prefix (left part).



Pivot: When the Separator ^ is reached, switch to goRight.



goRight: Traverse forwards to fill the suffix.



Validation:



Does it fit on the board?



Do the cross-words match the dictionary?



Do we have the tiles in the rack?



3\. Stability Protocols (Critical Fixes)

These mechanisms were implemented to solve specific crash/loop bugs. Do not remove.



3.1. The Anti-Ghost Logic (Memory Safety)

Location: AIPlayer::goRight



Issue: The AI used to read board\[row]\[15] (out of bounds), finding garbage data and creating invalid words like "INTERNATIONALLY" off the edge.



Protocol: if (col > 14) return; is enforced before any board access.



3.2. The Anchor Pivot Fix (Logic Safety)

Location: AIPlayer::goLeft



Issue: The AI would pivot to goRight immediately after placing a tile, ignoring existing tiles to its left (e.g., forming "OUTKYDST").



Protocol: The recursion can only pivot if board\[row]\[col-1] is empty or off-board. It must consume the entire existing prefix before switching directions.



3.3. The Exchange Safety Valve (Loop Safety)

Location: pve.cpp, aiai.cpp



Issue: AI would try to exchange tiles when the bag was empty (< 7 tiles). The engine rejected it, the game loop retried, leading to an infinite loop.



Protocol:



AIPlayer checks bag.size() >= 7 before requesting exchange.



pve.cpp catches a false return from executeExchangeMove and forces a PASS to hand control back to the other player.



3.4. The Challenge Scanner (Quality Assurance)

Location: AIPlayer::shouldChallenge



Issue: The AI blindly trusted the Move object text. If a player hooked "S" to "APPLE" to make invalid "APPLES", the AI only checked "S".



Protocol: The AI ignores the move text. It physically scans the board at the move coordinates, reconstructs the full main word and all cross-words, and verifies them against the dictionary manually.



4\. Helper \& Utility Functions

isRackBad(rack): Returns true if rack has no vowels, no consonants, or Q without U. Triggers Exchange strategy.



calculateTrueScore(move): Accurate scoring engine handling Double/Triple Word/Letter scores and Bingos (50pts).



challengePhrase(): Selects a random pop-culture quote (HAL 9000, Terminator) to display during a challenge. Optimised with static variables for zero-allocation performance.



5\. Future Optimization Roadmap (Phase 2)

These are the next steps to reach "Super-Engine" status.



Tunneling (fast\_constraints.cpp):



Current: Brute force 'A'-'Z' string construction.



Target: Use GADDAG graph traversal to calculate vertical constraints in microseconds.



Multi-Threading (ai\_player.cpp):



Current: Single-threaded loop 0-14.



Target: Use std::async or std::thread to process rows 0-7 and 8-14 in parallel.



Monte Carlo Simulation:



Current: Best Score = Best Move.



Target: Simulate 1000 random end-games for the top 10 candidates. Pick the move with the highest Win Probability, even if it scores less points (Defensive Play).



End of Documentation

