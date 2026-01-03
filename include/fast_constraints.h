#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "engine/board.h"

using namespace std;

// A bitmask representing allowed letters for a specific square.
// Bit 0 = 'A', Bit 1 = 'B', ... Bit 25 = 'Z'.
using CharMask = uint32_t;

constexpr CharMask MASK_ANY = 0x03ffffff; // All 26 letters allowed
constexpr CharMask MASK_NONE = 0x00000000; // No letters allowed

// Represents the constraints for a single row (15 columns)
struct RowConstraint {
    array < CharMask, BOARD_SIZE > masks;

    RowConstraint() {
        masks.fill(MASK_NONE);
    }

    // Fast Check: Is this 'letter' allowed at 'col'?
    inline bool isAllowed(int col, char letter) const {
        if (col < 0 || col >= BOARD_SIZE) {
            return false;
        }

        int idx = letter - 'A'; // A = 0, B = 1, ...

        if (idx < 0 || idx > 25) {
            return false;
        }

        // for example mask[7] = 00000000000010010000
        // now checking if E(bit 4) is valid.
        // masks[col] >> 4 = 00000000000000000001
        // now it's sitting in the first position, therefor valid.
        return (masks[col] >> idx) & 1 ;
    }
};

// The Constraint Generator Class
class ConstraintGenerator {
public:
    // Generates a constraint mask for a specific row
    // 'rowIdx' is the row we want to play on (0-14)
    static RowConstraint generateRowConstraint(const LetterBoard &letters, int rowIdx);

private:
    // Calculates the "Cross-check" (vertical constraint) for a single cell.
    // Returns a mask of letters that form valid vertical works.
    static CharMask computeCrossCheck(const LetterBoard &letters, int row, int col);
};

























