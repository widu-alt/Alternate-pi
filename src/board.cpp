#include <iostream>

#include "../include/board.h"

using namespace std;

//Converting type to a label for printing
string cellLabel(CellType type) {
    switch (type) {
        case CellType::Normal:
            return " ";
        case CellType::DLS:
            return ">2x<";
        case CellType::TLS:
            return ">3x<";
        case CellType::DWS:
            return "<2x>";
        case CellType::TWS:
            return "<3x>";
        default: return " ";
    }
}

//Return 'text' centered inside a field of given width.
// If width is larger than the text, spaces are added on both sides.
static string centerText(const string& text, int width) {
    int padding = width - static_cast<int>(text.size());
    if (padding < 0) {
        //Text is longer than the width, just return it as-is
        return text;
    }

    int leftPadding = padding/2;
    int rightPadding = padding - leftPadding;

    return string(leftPadding, ' ') + text + string(rightPadding, ' ');
}

static void printLegend() {
    cout << "\nLegend:\n";
    cout << "  <2x> = Double Word   <3x> = Triple Word\n";
    cout << "  >2x< = Double Letter >3x< = Triple Letter\n\n";
}

static void printHorizontalBorder() {
    cout << "    +";
    for (int col = 0; col < BOARD_SIZE; col++) {
        for (int i = 0; i < CELL_INNER_WIDTH; i++) {
            cout << "-";
        }
        cout << "+";
    }
    cout << "\n";
}

//Printing header rows with column numbers 1-15
static void printColumnHeader() {
    printHorizontalBorder();

    // Leading spaces so numbers align with the board below
    cout << "    |";
    for (int col = 0; col < BOARD_SIZE; col++) {
        string numberText = to_string(col + 1);
        cout << centerText(numberText, CELL_INNER_WIDTH) << "|";
    }
    cout << "\n";

    printHorizontalBorder();
}

void printTitle() {
    cout << "=========================================\n";
    cout << "                LEXI_PI\n";
    cout << "             Terminal Edition\n";
    cout << "\n";
    //cout << "     For My Lovely Girlfriend, Minadee\n";
    cout << "=========================================\n\n";
}

// Building the board and setting up all the bonus squares
Board createBoard() {
    Board board{};

    //filling everything with normal cells.
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            board[row][col] = CellType::Normal;
        }
    }

    // --------------------------------------------------------------
    // Official Scrabble Bonus Layout
    // (Row = A–O → 0–14, Col = 1–15 → 0–14)
    // --------------------------------------------------------------

    // --- Triple Word Score (TWS) ---
    int tws[][2] = {
        {0,0},{0,7},{0,14},
        {7,0},{7,14},
        {14,0},{14,7},{14,14}
    };

    for (auto &p : tws) board[p[0]][p[1]] = CellType::TWS;

    //C++ learning note
    /*
     * in "for(auto &p : tws)" the p becomes a reference to each element in the list.
     * each element looks like {row, column}
     * :tws means for each element inside tws
     * so p is effectively, first iteration p = {0,0}, second iteration p = {0,7} etc...
     */

    // --- Double Word Score (DWS) ---
    // (Note: H7 was already set by you, but including it does no harm)
    int dws[][2] = {
        {1,1},{2,2},{3,3},{4,4},{7,7},
        {10,10},{11,11},{12,12},{13,13},
        {1,13},{2,12},{3,11},{4,10},
        {10,4},{11,3},{12,2},{13,1}
    };
    for (auto &p : dws) board[p[0]][p[1]] = CellType::DWS;

    // --- Triple Letter Score (TLS) ---
    int tls[][2] = {
        {1,5},{1,9},
        {5,1},{5,5},{5,9},{5,13},
        {9,1},{9,5},{9,9},{9,13},
        {13,5},{13,9}
    };
    for (auto &p : tls) board[p[0]][p[1]] = CellType::TLS;

    // --- Double Letter Score (DLS) ---
    int dls[][2] = {
        {0,3},{0,11},
        {2,6},{2,8},
        {3,0},{3,7},{3,14},
        {6,2},{6,6},{6,8},{6,12},
        {7,3},{7,11},
        {8,2},{8,6},{8,8},{8,12},
        {11,0},{11,7},{11,14},
        {12,6},{12,8},
        {14,3},{14,11}
    };
    for (auto &p : dls) board[p[0]][p[1]] = CellType::DLS;

    return board;
}

// clearing the letter board
void clearLetterBoard(LetterBoard &letters) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            letters[row][col] = ' '; //empty
        }
    }
}

// clearing the blank board
void clearBlankBoard(BlankBoard &blanks) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            blanks[row][col] = false;
        }
    }
}

// placing the word on letter board
bool placeWordOnBoard(LetterBoard &letters, int row, int col, bool horizontal, const string &word) {
    int dr = horizontal ? 0 : 1; //goes row by row if its vertical (row step)
    int dc = horizontal ? 1 : 0; //goes column by column if its horizontal (column step)

    // 1) bounds check
    int r = row;
    int c = col;

    // C++ learning
    // for (char ch: word) means loop through each character inside the string word
    for (char ch: word) {
        if (r<0 || r >= BOARD_SIZE || c<0 || c >= BOARD_SIZE) {
            return false;
        }
        r += dr;
        c += dc;
    }

    // 2) Checking for conflicts
    r = row;
    c = col;

    for (char ch: word) {

        // C++ learning
        // std::toupper converts letter strings to uppercase

        char upper = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        char existing = letters[r][c];

        if (existing != ' ' && existing != upper) {
            return false; // trying to overwrite a different letter
        }
        r += dr;
        c += dc;
    }

    // 3) Actually place the letters
    r = row;
    c = col;
    for (char ch: word) {
        char upper = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        letters[r][c] = upper;
        r += dr;
        c += dc;
    }

    return true;
}

void printBoard(const Board &bonusBoard, const LetterBoard &letters) {
    //printTitle();
    printLegend();
    printColumnHeader();
    cout << "\033[90m";

    for (int row = 0; row < BOARD_SIZE; row++) {
        char rowLabel = static_cast<char>('A' + row);

        //Row lebel at the left (eg: "A |")
        cout << "  " << "\033[0m" << rowLabel << "\033[90m" << " |";

        //print each cell in this row
        for (int col = 0; col < BOARD_SIZE; col++) {
            char letterOnBoard = letters[row][col];
            string cellText;

            if (letterOnBoard != ' ') {
                // There is a tile here, show letter instead of bonus
                //cellText = string(1, letterOnBoard);
                cout << "\033[38;2;255;165;0m" << centerText(string(1, letterOnBoard), CELL_INNER_WIDTH)  << "\033[90m" << "|";
            } else {
                //cellText = cellLabel(bonusBoard[row][col]);
                cout << centerText(cellLabel(bonusBoard[row][col]), CELL_INNER_WIDTH) << "|";
            }

            //cout << "\033[90m" << centerText(cellText, CELL_INNER_WIDTH) << "|" << "\033[0m";
        }
        cout << "\n";

        //print border under the row
        printHorizontalBorder();
        cout << "\033[0m";

    }
}