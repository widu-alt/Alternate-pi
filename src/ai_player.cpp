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

AIPlayer::DifferentialMove AIPlayer::calculateDifferential(const LetterBoard &letters, const spectre::MoveCandidate &bestMove) {
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

bool AIPlayer::isRackBad(const TileRack& rack) {
    int v=0, c=0;
    for (auto t : rack) {
        if (strchr("AEIOU", t.letter)) {
            v++;
        }else if (t.letter != '?') {
            c++;
        }
    }
    return (v == 0 || c == 0) && rack.size() >= 5;
}

string AIPlayer::getTilesToExchange(const TileRack& rack) {
    string s; for (auto t : rack) if (t.letter != '?' && t.letter != 'S') s += t.letter;
    return s.empty() ? "A" : s;
}

Move AIPlayer::getMove(const GameState& state,
                       const Board& bonusBoard,
                       const LastMoveInfo& lastMove,
                       bool canChallenge)
{
    // 1. CUTIE_PI: Check for Invalid Moves (The "Evil" Logic)
    if (style == AIStyle::CUTIE_PI && canChallenge && lastMove.exists) {
        // Iterate known formed words provided by Director
        for (const auto& word : lastMove.formedWords) {
            if (!gDictionary.isValidWord(word)) return Move(MoveType::CHALLENGE);
        }
    }

    candidates.clear();
    spectre::MoveCandidate bestMove;
    bestMove.word[0] = '\0';
    bestMove.score = -10000;

    // ---------------------------------------------------------
    // BRAIN 1: SPEEDI_PI (Static Heuristics Only)
    // ---------------------------------------------------------
    if (style == AIStyle::SPEEDI_PI) {
        // Direct call to MoveGenerator (No Vanguard class overhead)
        findAllMoves(state.board, state.players[state.currentPlayerIndex].rack);

        if (!candidates.empty()) {
            for (auto& cand : candidates) {
                int boardScore = calculateTrueScore(cand, state.board, bonusBoard);
                float leavePenalty = 0.0f;
                for (int i = 0; cand.leave[i] != '\0'; i++) {
                    leavePenalty += Heuristics::getLeaveValue(cand.leave[i]);
                }
                cand.score = boardScore + (int)leavePenalty;
            }
            // Sort by score
            std::sort(candidates.begin(), candidates.end(),
                [](const MoveCandidate& a, const MoveCandidate& b) { return a.score > b.score; });
            bestMove = candidates[0];
        }
    }
    // ---------------------------------------------------------
    // BRAIN 2: CUTIE_PI (Spectre Engine)
    // ---------------------------------------------------------
    else {
        const Player& me = state.players[state.currentPlayerIndex];
        spy.updateGroundTruth(state.board, me.rack, state.bag);

        bestMove = Vanguard::search(state.board, bonusBoard, me.rack, spy, gDictionary, 500);
    }

    // ---------------------------------------------------------
    // EXECUTION & TRANSLATION
    // ---------------------------------------------------------
    const Player& me = state.players[state.currentPlayerIndex];
    bool shouldExchange = (bestMove.word[0] == '\0') ||
                          (bestMove.score < 14 && isRackBad(me.rack) && state.bag.size() >= 7);

    if (shouldExchange) {
        if (state.bag.size() < 7) return Move(MoveType::PASS);
        Move ex; ex.type = MoveType::EXCHANGE;
        ex.exchangeLetters = getTilesToExchange(me.rack);
        return ex;
    }

    DifferentialMove diff = calculateDifferential(state.board, bestMove);
    if (diff.row == -1) return Move(MoveType::PASS);

    Move result;
    result.type = MoveType::PLAY;
    result.row = diff.row;
    result.col = diff.col;
    result.word = diff.word;
    result.horizontal = bestMove.isHorizontal;
    return result;
}

Move AIPlayer::getEndGameResponse(const GameState& state, const LastMoveInfo& lastMove) {
    // If opponent played an invalid word, CHALLENGE.
    for (const auto& word : lastMove.formedWords) {
        if (!gDictionary.isValidWord(word)) return Move(MoveType::CHALLENGE);
    }
    // Speedi_Pi accepts fate. Cutie_Pi *might* challenge if it thinks it can win, but for now PASS.
    return Move(MoveType::PASS);
}

void AIPlayer::findAllMoves(const LetterBoard &letters, const TileRack &rack) {
    candidates = MoveGenerator::generate(letters, rack, gDictionary);
}