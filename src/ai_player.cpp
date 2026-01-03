#include "../include/ai_player.h"
#include "../include/heuristics.h"
#include "../include/tile_tracker.h"
#include "../include/spectre/move_generator.h"
#include "../include/spectre/vanguard.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace spectre;
using namespace std;
using namespace std::chrono;

const int SEPERATOR = 26;

// --- CONSTRUCTOR & IDENTITY ---
AIPlayer::AIPlayer(AIStyle style) : style(style) {}

string AIPlayer::getName() const {
    return (style == AIStyle::SPEEDI_PI) ? "Speedi_Pi" : "Cutie_Pi";
}

// --- NEW: WIRE THE BRAIN ---
void AIPlayer::observeMove(const Move& move, const LetterBoard& board) {
    if (style == AIStyle::CUTIE_PI) {
        // Feed opponent's move into the Spy to update probability models
        spy.observeOpponentMove(move, board);
    }
}

// --- HELPERS (Keep these for Speedi_Pi and general logic) ---

// Optimization: Prune the search space
struct SearchRange {
    int minRow, maxRow, minCol, maxCol;
    bool isEmpty;
};

// Helper to calculate score for a specific move
int calculateTrueScore(const spectre::MoveCandidate &move, const LetterBoard& letters, const Board &bonusBoard) {

    int totalScore = 0;
    int mainWordScore = 0;
    int mainWordMultiplier = 1;
    int tilesPlacedCount = 0;

    int r = move.row;
    int c = move.col;
    int dr = move.isHorizontal ? 0 : 1;
    int dc = move.isHorizontal ? 1 : 0;

    for (char letter : move.word) {
        if (letter == '\0') break;

        if (r < 0 || r > 14 || c < 0 || c > 14) return -1000;

        int letterScore = Heuristics::getTileValue(letter);
        bool isNewlyPlaced = (letters[r][c] == ' ');

        if (isNewlyPlaced) {
            tilesPlacedCount++;
            CellType bonus = bonusBoard[r][c];

            if (bonus == CellType::DLS) letterScore *= 2;
            else if (bonus == CellType::TLS) letterScore *= 3;

            if (bonus == CellType::DWS) mainWordMultiplier *= 2;
            else if (bonus == CellType::TWS) mainWordMultiplier *= 3;
        }

        mainWordScore += letterScore;

        if (isNewlyPlaced) {
            int pdr = move.isHorizontal ? 1 : 0;
            int pdc = move.isHorizontal ? 0 : 1;
            bool hasNeighbour = false;

            int checkR1 = r - pdr;
            int checkC1 = c - pdc;
            if (checkR1 >= 0 && checkR1 < 15 && checkC1 >= 0 && checkC1 < 15 && letters[checkR1][checkC1] != ' ') hasNeighbour = true;

            int checkR2 = r + pdr;
            int checkC2 = c + pdc;
            if (checkR2 >= 0 && checkR2 < 15 && checkC2 >= 0 && checkC2 < 15 && letters[checkR2][checkC2] != ' ') hasNeighbour = true;

            if (hasNeighbour) {
                int currR = r, currC = c;
                while (currR - pdr >= 0 && currC - pdc >= 0 && letters[currR-pdr][currC-pdc] != ' ') {
                    currR -= pdr;
                    currC -= pdc;
                }

                int crossScore = 0;
                int crossMult = 1;

                while (currR < 15 && currC < 15) {
                    char cellLetter = letters[currR][currC];
                    if (currR == r && currC == c) {
                        int crossLetterScore = Heuristics::getTileValue(letter);
                        CellType crossBonus = bonusBoard[currR][currC];
                        if (crossBonus == CellType::DLS) crossLetterScore *= 2;
                        else if (crossBonus == CellType::TLS) crossLetterScore *= 3;
                        if (crossBonus == CellType::DWS) crossMult *= 2;
                        else if (crossBonus == CellType::TWS) crossMult *= 3;
                        crossScore += crossLetterScore;
                    } else if (cellLetter != ' ') {
                        crossScore += Heuristics::getTileValue(cellLetter);
                    } else {
                        break;
                    }
                    currR += pdr;
                    currC += pdc;
                }
                totalScore += (crossScore * crossMult);
            }
        }
        r += dr;
        c += dc;
    }

    if (strlen(move.word) > 1) {
        totalScore += (mainWordScore * mainWordMultiplier);
    }
    if (tilesPlacedCount == 7) {
        totalScore += 50;
    }

    return totalScore;
}

AIPlayer::DifferentialMove AIPlayer::calculateEngineMove(const LetterBoard &letters, const spectre::MoveCandidate &bestMove) {
    DifferentialMove diff;
    diff.row = -1;
    diff.col = -1;
    diff.word = "";

    int r = bestMove.row;
    int c = bestMove.col;
    int dr = bestMove.isHorizontal ? 0 : 1;
    int dc = bestMove.isHorizontal ? 1 : 0;

    for (char letter : bestMove.word) {
        if (letter == '\0') break;
        if (letters[r][c] == ' ') {
            if (diff.row == -1 && diff.col == -1) {
                diff.row = r;
                diff.col = c;
            }
            diff.word += letter;
        }
        r += dr;
        c += dc;
    }
    return diff;
}

bool AIPlayer::isRackBad(const TileRack &rack) {
    int vowels = 0, consonants = 0;
    bool hasQ = false, hasU = false;
    int counts[26] = {0};

    for (const Tile &t : rack) {
        if (t.letter == '?') { vowels++; consonants++; continue; }
        if (strchr("AEIOU", t.letter)) vowels++; else consonants++;
        if (t.letter == 'Q') hasQ = true;
        if (t.letter == 'U') hasU = true;
        if (isalpha(t.letter)) counts[toupper(t.letter) - 'A']++;
    }
    if (hasQ && !hasU) return true;
    if (rack.size() >= 5 && (vowels == 0 || consonants == 0)) return true;
    for (int i=0; i<26; i++) if (counts[i] >= 3) return true;
    return false;
}

string AIPlayer::getTilesToExchange(const TileRack &rack) {
    string toExchange = "";
    bool hasQ = false;
    for(auto t : rack) if(t.letter=='Q') hasQ=true;

    for (const Tile &t : rack) {
        bool keep = false;
        if (t.letter == '?' || t.letter == 'S') keep = true;
        if (t.letter == 'U' && hasQ) keep = true;
        string retain = "RETINA";
        if (retain.find(t.letter) != string::npos && !keep) keep = true;
        if (!keep) toExchange += t.letter;
    }
    if (toExchange.empty() && !rack.empty()) {
        for(auto t : rack) toExchange += t.letter;
    }
    return toExchange;
}

bool AIPlayer::shouldChallenge(const Move &opponentMove, const LetterBoard &board) const {
    if (opponentMove.type != MoveType::PLAY) return false;

    int startR = opponentMove.row;
    int startC = opponentMove.col;
    int dr = opponentMove.horizontal ? 0 : 1;
    int dc = opponentMove.horizontal ? 1 : 0;

    int r = startR;
    int c = startC;
    while (r - dr >= 0 && c - dc >= 0 && board[r - dr][c - dc] != ' ') {
        r -= dr;
        c -= dc;
    }

    string mainWord = "";
    int scanR = r;
    int scanC = c;
    while (scanR < 15 && scanC < 15 && board[scanR][scanC] != ' ') {
        mainWord += board[scanR][scanC];
        scanR += dr;
        scanC += dc;
    }

    if (mainWord.length() > 1 && !gDawg.isValidWord(mainWord)) {
        cout << "\n[AI] Challenging! Main word '" << mainWord << "' is invalid." << endl;
        return true;
    }

    scanR = r;
    scanC = c;
    int pdr = opponentMove.horizontal ? 1 : 0;
    int pdc = opponentMove.horizontal ? 0 : 1;

    for (size_t i = 0; i < mainWord.length(); ++i) {
        bool hasNeighbor = false;
        if (scanR - pdr >= 0 && scanC - pdc >= 0 && board[scanR - pdr][scanC - pdc] != ' ') hasNeighbor = true;
        if (scanR + pdr < 15 && scanC + pdc < 15 && board[scanR + pdr][scanC + pdc] != ' ') hasNeighbor = true;

        if (hasNeighbor) {
            int crossR = scanR;
            int crossC = scanC;
            while (crossR - pdr >= 0 && crossC - pdc >= 0 && board[crossR - pdr][crossC - pdc] != ' ') {
                crossR -= pdr;
                crossC -= pdc;
            }

            string crossWord = "";
            while (crossR < 15 && crossC < 15 && board[crossR][crossC] != ' ') {
                crossWord += board[crossR][crossC];
                crossR += pdr;
                crossC += pdc;
            }

            if (crossWord.length() > 1 && !gDawg.isValidWord(crossWord)) {
                cout << "[AI] Challenging! Cross-word '" << crossWord << "' is invalid." << endl;
                return true;
            }
        }
        scanR += dr;
        scanC += dc;
    }

    return false;
}

Move AIPlayer::getMove(const Board &bonusBoard,
                       const LetterBoard &letters,
                       const BlankBoard &blankBoard,
                       const TileBag &bag,
                       const Player &me,
                       const Player &opponent,
                       int PlayerNum) {

    candidates.clear();
    const TileRack &myRack = me.rack;
    spectre::MoveCandidate bestMove;
    bestMove.word[0] = '\0';
    bestMove.score = -1;

    // ---------------------------------------------------------
    // BRANCH 1: SPEEDI_PI (The Speedster)
    // ---------------------------------------------------------
    if (style == AIStyle::SPEEDI_PI) {
        findAllMoves(letters, myRack);

        if (!candidates.empty()) {
            // Restore Heuristics
            TileTracker tracker;
            for(int r=0; r<15; r++) for(int c=0; c<15; c++)
                if(letters[r][c]!=' ') {
                     if (blankBoard[r][c]) tracker.markSeen('?');
                     else tracker.markSeen(letters[r][c]);
                }
            for(const auto& t : myRack) tracker.markSeen(t.letter);
            Heuristics::updateWeights(tracker);

            for (auto &cand : candidates) {
                int boardPoints = calculateTrueScore(cand, letters, bonusBoard);
                float leaveVal = 0.0f;
                for (int i=0; cand.leave[i] != '\0'; i++) {
                    leaveVal += Heuristics::getLeaveValue(cand.leave[i]);
                }
                cand.score = boardPoints + (int)leaveVal;
            }

            std::sort(candidates.begin(), candidates.end(),
                [](const spectre::MoveCandidate &a, const spectre::MoveCandidate &b) {
                    return a.score > b.score;
            });

            bestMove = candidates[0];
        }
    }
    // ---------------------------------------------------------
    // BRANCH 2: CUTIE_PI (The Champion)
    // ---------------------------------------------------------
    else {
        // 1. Update the Spy's ground truth (What tiles are definitively known)
        // This calculates the belief matrix for the remaining tiles (Bag + Opponent Rack)
        spy.updateGroundTruth(letters, myRack, bag);

        // 2. Run Vanguard MCTS using the Persistent Spy
        bestMove = spectre::Vanguard::search(
            letters,
            bonusBoard,
            myRack,
            spy, // <--- PASS THE PERSISTENT SPY (The Brain)
            gDawg,
            500
        );
    }

    // ---------------------------------------------------------
    // COMMON EXECUTION
    // ---------------------------------------------------------

    bool shouldExchange = false;
    if (bestMove.word[0] == '\0') {
        shouldExchange = true;
    }
    else if (bestMove.score < 14 && isRackBad(myRack) && bag.size() >= 7) {
        shouldExchange = true;
    }

    if (shouldExchange) {
        if (bag.size() < 7) {
            if (bestMove.word[0] == '\0') {
                Move p; p.type=MoveType::PASS; return p;
            }
        } else {
            Move exMove;
            exMove.type = MoveType::EXCHANGE;
            exMove.exchangeLetters = getTilesToExchange(myRack);
            exMove.word = "";
            return exMove;
        }
    }

    DifferentialMove engineMove = calculateEngineMove(letters, bestMove);
    if (engineMove.row == -1 || engineMove.word.empty()) {
        Move passMove; passMove.type = MoveType::PASS; return passMove;
    }

    Move result;
    result.type = MoveType::PLAY;
    result.row = engineMove.row;
    result.col = engineMove.col;
    result.word = engineMove.word;
    result.horizontal = bestMove.isHorizontal;
    return result;
}

Move AIPlayer::getEndGameDecision() {
    Move m; m.type = MoveType::PASS; return m;
}

void AIPlayer::findAllMoves(const LetterBoard &letters, const TileRack &rack) {
    this->candidates = spectre::MoveGenerator::generate(letters, rack, gDawg);
}