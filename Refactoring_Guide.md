# Step 1: Refactoring Guide (The "Brain Transplant")

This guide details how to decouple the game logic from the console input, allowing an AI to "plug in" as a player.

## 1. Create the Interface (`include/controller.h`)
This file defines the contract that both Humans and AI must follow.

```cpp
#pragma once
#include <string>
#include "board.h"
#include "rack.h"

// Abstract Base Class
class PlayerController {
public:
    virtual ~PlayerController() = default;

    // The core method: "It's your turn, what do you want to do?"
    // Returns a command string (e.g., "A10 H WORD", "PASS", "EXCHANGE ABC")
    virtual std::string getMove(
        const Board& board, 
        const LetterBoard& letters, 
        const TileRack& rack, 
        int score, 
        int opponentScore
    ) = 0;
};
```

## 2. Implement Human Controller (`src/controller.cpp`)
This class wraps the existing `cin` logic.

```cpp
#include "../include/controller.h"
#include <iostream>

class HumanController : public PlayerController {
public:
    std::string getMove(const Board& board, const LetterBoard& letters, const TileRack& rack, int score, int opponentScore) override {
        std::cout << "\nYour Turn!\n";
        std::cout << "Commands: [RowCol Dir Word] (Play), [P] (Pass), [E tiles] (Exchange)\n";
        std::cout << "Enter Command: ";
        
        std::string input;
        // Read the full line or token depending on your parsing logic
        // For now, let's assume we read a line to handle spaces
        std::getline(std::cin, input);
        
        return input;
    }
};
```

## 3. Modify the Game Loop (`src/modes/PvP/pvp.cpp`)
You need to replace the hardcoded `cin` calls with the controller.

**Before:**
```cpp
// Inside the game loop...
cout << "Enter Choice: ";
cin >> input;
if (input == "M") { ... }
```

**After:**
```cpp
// 1. Setup Controllers (at start of function)
PlayerController* controllers[2];
controllers[0] = new HumanController(); // Player 1
controllers[1] = new HumanController(); // Player 2 (Later, this can be AI!)

// 2. Inside the game loop
string command = controllers[currentPlayer]->getMove(bonusBoard, letters, players[currentPlayer].rack, ...);

// 3. Parse the command string
// You will need to move your parsing logic (splitting "A10 H WORD") 
// into a helper function that takes this string and executes the move.
```

## 4. Checklist for Success
1.  [ ] Create `include/controller.h`.
2.  [ ] Create `src/controller.cpp` and implement `HumanController`.
3.  [ ] In `pvp.cpp`, instantiate two `HumanController` objects.
4.  [ ] Refactor `handleMoveChoice` in `choices.cpp` to accept a string argument instead of reading from `cin`.
5.  [ ] Verify the game still works exactly as before (PvP).

Once this is done, adding an AI is as simple as:
`controllers[1] = new AIController();`