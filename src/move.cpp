#include "../include/move.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace std;

// Uppercase helper
static string toUpper(const string &s) {
    string r = s;

    // C++ learning
    // std::tranform appiles a function to each element in a range and writes the results
    //-somewhere else.
    // std::transform(begin, end, destination_begin, function);
    transform(r.begin(), r.end(), r.begin(),
        [](unsigned char ch){ return static_cast<char>(toupper(ch)); });
    return r;
}

// Letter -> points
// This is needed because the board stores only chars, not Tile objects
static int letterPoints(char ch) {
    ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));

    switch (ch) {
        case 'A': case 'E': case 'I': case 'O': case 'U':
        case 'N': case 'R': case 'T': case 'L': case 'S':
            return 1;
        case 'D': case 'G':
            return 2;
        case 'B': case 'C': case 'M': case 'P':
            return 3;
        case 'F': case 'H': case 'V': case 'W': case 'Y':
            return 4;
        case 'K':
            return 5;
        case 'J': case 'X':
            return 8;
        case 'Q': case 'Z':
            return 10;
        default:
            return 0;
    }
}

// Finding a tile in the rack that can represent a letter.
// Use a blank ? if the exact letter is not in the rack.
// 'used' marks tiles that are already consumed for this move.
static int findTilesForLetter(const TileRack &rack, const vector<bool> &used, char letter) {

    char upper = static_cast<char>(toupper(static_cast<unsigned char>(letter)));

    // Trying exact match first
    for (int i=0; i < static_cast<int>(rack.size()); i++ ) {
        if (used[i]) continue;
        if (toupper(static_cast<unsigned char>(rack[i].letter)) == upper) {
            return i;
        }
    }

    // for blanks
    for (int i=0; i < static_cast<int>(rack.size()); i++ ) {
        if (used[i]) continue;
        if (rack[i].letter == '?') {
            return i;
        }
    }

    return -1;
}

// Score the main word that includes the square at (anchorRow,anchorCol)
// newTile[r][c] is true if that tile was placed this turn.
static int scoreMainWord(const Board &bonusBoard, const LetterBoard &letters, const BlankBoard &blanks,
                        bool newTile[BOARD_SIZE][BOARD_SIZE], int anchorRow, int anchorCol, bool horizontal) {
    int dr = horizontal ? 0 : 1;
    int dc = horizontal ? 1 : 0;

    // Step backwards to find the true start of the word.
    int r = anchorRow;
    int c = anchorCol;

    // finding the start of the formed word.
    while (true) {
        int pr = r - dr;
        int pc = c - dc;
        if (pr < 0 || pr >= BOARD_SIZE || pc < 0 || pc >= BOARD_SIZE) {
            break;
        }
        if (letters[pr][pc] == ' ') break;

        //True start of the word (letters[r][c])
        r = pr;
        c = pc;
    }

    int totalLetterScore = 0;
    int wordMultiplier = 1;

    // Walking forward, until we hit an empty squre
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && letters[r][c] != ' ') {

        char ch = letters[r][c];

        // No letter score if its a blank.
        int base = blanks[r][c] ? 0 : letterPoints(ch);
        int letterScore = base;

        if (newTile[r][c]) {
            CellType cell = bonusBoard[r][c];

            // Letter multiplier only affect the newly placed non-blank tiles.
            if (!blanks[r][c]) {
                if (cell == CellType::DLS) {
                    letterScore *= 2;
                }
                if (cell == CellType::TLS) {
                    letterScore *= 3;
                }
            }

            // Word multipliers only count if a new tile is on them
            if (cell == CellType::DWS) {
                wordMultiplier *= 2;
            }
            if (cell == CellType::TWS) {
                wordMultiplier *= 3;
            }
        }

        totalLetterScore += letterScore;

        r += dr;
        c += dc;
    }

    return (totalLetterScore * wordMultiplier);
}

// Score a cross word created by placing a tile at (anchorRow, anchorCol)
// mainHorizontal = orientation of the MAIN word; cross is perpendicular.
static int scoreCrossWord(const Board &bonusBoard, const LetterBoard &letters, const BlankBoard &blanks,
                        bool newTile[BOARD_SIZE][BOARD_SIZE], int anchorRow, int anchorCol, bool mainHorizontal) {
    // perpendicular direction
    int dr = mainHorizontal ? 1 : 0;
    int dc = mainHorizontal ? 0 : 1;

    int r = anchorRow;
    int c = anchorCol;

    // First check if there is any neighbor in that perpendicular direction
    bool hasNeighbor = false;

    // checking either side
    int nr = r - dr;
    int nc = c - dc;
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && letters[nr][nc] !=' ') {
        hasNeighbor = true;
    }

    nr = r + dr;
    nc = c + dc;
    if (!hasNeighbor && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && letters[nr][nc] !=' ') {
        hasNeighbor = true;
    }

    // Single tile with no adjacent letter in cross direction
    if (!hasNeighbor) {
        return 0;
    }

    // Back up to the start of the cross word
    r = anchorRow;
    c = anchorCol;

    while (true) {
        int pr = r - dr;
        int pc = c - dc;
        if (pr < 0 || pr >= BOARD_SIZE || pc < 0 || pc >= BOARD_SIZE) {
            break;
        }
        if (letters[pr][pc] == ' ') {
            break;
        }

        // start of the cross word.
        r = pr;
        c = pc;
    }

    int totalLetterScore = 0;
    int wordMultiplier = 1;
    int length = 0;

    // Walk forward along the cross-word direction
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && letters[r][c] != ' ') {

        ++length;

        char ch = letters[r][c];
        int base = blanks[r][c] ? 0 : letterPoints(ch);
        int letterScore = base;

        if (newTile[r][c]) {
            CellType cell = bonusBoard[r][c];

            if (!blanks[r][c]) {
                if (cell == CellType::DLS) {
                    letterScore *= 2;
                }else if (cell == CellType::TLS) {
                    letterScore *= 3;
                }
            }

            if (cell == CellType::DWS) {
                wordMultiplier *= 2;
            } else if (cell == CellType::TWS) {
                wordMultiplier *= 3;
            }
        }

        totalLetterScore += letterScore;
        r += dr;
        c += dc;
    }

    if (length <= 1) {
        // safety: no real cross-words formed
        return 0;
    }

    return totalLetterScore * wordMultiplier;
}

MoveResult playWord(const Board &bonusBoard, LetterBoard &letters, BlankBoard &blanks, TileBag &bag, TileRack &rack,
                    int startRow, int startCol, bool horizontal, const string &rackWordLower) {
    MoveResult res{};
    res.success = false;
    res.score = 0;

    if (rackWordLower.empty()) {
        res.message = "Empty rack word";
        return res;
    }

    string rackWord = toUpper(rackWordLower);

    //len is the length of the entered word.
    int len = static_cast<int>(rackWord.size());

    int dr = horizontal ? 0 : 1;
    int dc = horizontal ? 1 : 0;

    struct NewTile {
        int row;
        int col;
        char letter;
        bool isBlank;
        int rackIndex;
    };

    vector < NewTile > newTiles;

    // C++ Learning
    // vector<bool> usedRack(rack.size(), false); means create a vector(dynamic array) with rack.size() number
    //-of elements, and initialize ALL of them to false.
    vector<bool> usedRack(rack.size(), false);

    int r = startRow;
    int c = startCol;
    int wi = 0; //index into rackWord

    // Walk along the line, filling empties with rack letters in order,
    // Skipping over existing letters on board.
    char boardSquare = letters[r][c];
    if (boardSquare != ' ') {
        res.message = "Starting cell should be empty";
        return res;
    }

    while (true) {
        // checking if off board
        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
            if (wi == len) {
                //Used all the rack letters
                break;
            }
            res.message = "Word goes off the board.";
            return res;
        }

        char existing = letters[r][c];

        if (existing == ' ') {
            if (wi >= len) {
                // No more rack letters to place and we hit an empty
                break;
            }

            // Plan to place rackWord[wi] here
            NewTile nt;
            nt.row = r;
            nt.col = c;
            nt.letter = rackWord[wi];
            nt.isBlank = false;
            nt.rackIndex = -1;
            newTiles.push_back(nt);

            ++wi;
        }else {
            // There is already a letter here on the board
            // It becomes part of the full word, but doesnt consume rack tiles.
            // rackWord doesn't include these letters.
        }

        r += dr;
        c += dc;
    }

    //safety checks
    if (wi != len) {
        res.message = "Not enough space in that direction for your tiles.";
        return res;
    }

    // End up not placing any tiles (Illegal move)
    if (newTiles.empty()) {
        res.message = "Must place at least 1 tile";
        return res;
    }

    // Match each planned new tile with a tile from the rack
    for (auto &nt : newTiles) {
        int idx = findTilesForLetter(rack, usedRack, nt.letter);
        if (idx == -1) {
            res.message = "You dont have required tiles in your rack";
            return res;
        }
        usedRack[idx] = true;
        nt.isBlank = (rack[idx].letter == '?');
        nt.rackIndex = idx;
    }

    // Connectivity/ "Red Domain" Check

    // Detecting if the board already has any tiles (pre-move)
    bool boardHasExistingTiles = false;
    for (int rr = 0; rr < BOARD_SIZE; ++rr) {
        for (int cc = 0; cc < BOARD_SIZE; ++cc) {
            if (letters[rr][cc] != ' ') {
                boardHasExistingTiles = true;
                break;
            }
        }
    }

    if (!boardHasExistingTiles) {

        // First move must cover the center square (H8 -> row 7, col 7)
        int centerRow = BOARD_SIZE / 2;
        int centerCol = BOARD_SIZE / 2;

        bool coversCenter = false;
        for (const auto &nt : newTiles) {
            if (nt.row == centerRow && nt.col == centerCol) {
                coversCenter = true;
                break;
            }
        }

        if (!coversCenter) {
            res.message = "First move must cover the centre square (H8)";
            return res;
        }

    } else {
        // for subsequent moves at least one new tile must touch an existing tile
        bool touchesExisting = false;

        static const int drs[4] = {-1, 1, 0, 0};
        static const int dcs[4] = {0, 0, -1, 1};

        for (const auto &nt : newTiles) {
            for (int d = 0; d < 4; ++d) {
                int nr = nt.row + drs[d];
                int nc = nt.col + dcs[d];

                if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE) {
                    continue;
                }

                if (letters[nr][nc] != ' ') {
                    touchesExisting = true;
                    break;
                }
            }
            if (touchesExisting) {
                break;
            }
        }

        if (!touchesExisting) {
            res.message = "Word must connect to the existing network of tiles";
            return res;
        }
    }

    // Place the letters on the board
    bool newTile[BOARD_SIZE][BOARD_SIZE] = {false};

    for (const auto &nt : newTiles) {
        letters[nt.row][nt.col] = nt.letter;
        blanks[nt.row][nt.col] = nt.isBlank;
        newTile[nt.row][nt.col] = true;
    }

    // Score the main word
    int mainScore = scoreMainWord(
        bonusBoard,
        letters,
        blanks,
        newTile,
        startRow,
        startCol,
        horizontal
    );

    // Score all cross-words created by each newly placed tile
    int crossScore = 0;
    for (const auto &nt : newTiles) {
        crossScore += scoreCrossWord(
            bonusBoard,
            letters,
            blanks,
            newTile,
            nt.row,
            nt.col,
            horizontal
        );
    }

    int totalScore = mainScore + crossScore;

    // Bingo: if 7 tiles used from the rack +50 bonus points
    int tilesUsedFromRack = 0;
    for (bool used: usedRack) {
        if (used) ++tilesUsedFromRack;
    }
    if (tilesUsedFromRack == 7) {
        totalScore += 50;
    }

    res.success = true;
    res.score = totalScore;

    // Remove used tiles from the rack (erasing from the back to keep indices valid
    vector<int> usedIndices;
    for (int i = 0; i < static_cast<int>(usedRack.size()); i++) {
        if (usedRack[i]) {
            usedIndices.push_back(i);
        }
    }

    //this sores vector usedIndices in descending order
    sort(usedIndices.begin(), usedIndices.end(), greater<int>());
    for (int idx: usedIndices) {
        rack.erase(rack.begin() + idx);
    }

    // Refill rack up to 7 tiles
    if (rack.size() < 7) {
        drawTiles(bag, rack, static_cast<int>(7 - rack.size()));
    }

    return res;
}