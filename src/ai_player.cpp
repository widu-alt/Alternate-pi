#include "../include/ai_player.h"
#include "../include/heuristics.h"
#include "../include/tile_tracker.h"
#include "../include/spectre/move_generator.h"
#include "../include/spectre/vanguard.h"
#include "../include/engine/dictionary.h"
#include "../include/modes/PvE/pve.h"
#include "../include/spectre/judge.h"
#include "../include/engine/mechanics.h"

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
    // ---------------------------------------------------------
    // 1. THE VALIDATION CHECK (Runs for BOTH Bots)
    // ---------------------------------------------------------
    // Note: canChallenge is ONLY true in PvE (Human vs Bot).
    // It is FALSE in AiAi (Bot vs Bot), ensuring bots never challenge each other.
    if (canChallenge && lastMove.exists) {
        bool foundInvalid = false;
        string invalidWord = "";

        for (const auto& word : lastMove.formedWords) {
            if (!gDictionary.isValidWord(word)) {
                foundInvalid = true;
                invalidWord = word;
                break;
            }
        }

        if (foundInvalid) {
            cout << getName() << "\nDETECTED INVALID WORD: " << invalidWord << endl;

            // THE RULE OF COOL
            challengePhrase();
            std::this_thread::sleep_for(std::chrono::seconds(3));

            return Move(MoveType::CHALLENGE);
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
                int boardScore = Mechanics::calculateTrueScore(cand, state.board, bonusBoard);
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
        // CUTIE_PI LOGIC
        const Player& me = state.players[state.currentPlayerIndex];
        const Player& opp = state.players[1 - state.currentPlayerIndex];

        // Update Spy
        spy.updateGroundTruth(state.board, me.rack, state.bag);

        // DECISION FORK: ENDGAME vs MIDGAME
        if (state.bag.empty()) {
            // >>> THE JUDGE (Endgame Solver) <<<
            // We need to infer opponent rack one last time
            std::vector<char> inferredOpp = spy.generateWeightedRack();
            TileRack oppRack;
            for(char c : inferredOpp) { Tile t; t.letter=c; t.points=0; oppRack.push_back(t); }

            // Convert Spectre Move to Engine Move directly inside Judge or here
            Move jMove = Judge::solveEndgame(state.board, bonusBoard, me.rack, oppRack, gDictionary);
            return jMove;
        }
        else {
            // >>> THE VANGUARD (Midgame Strategy) <<<
            int scoreDiff = me.score - opp.score;
            int bagSize = state.bag.size();

            bestMove = Vanguard::search(
                state.board,
                bonusBoard,
                me.rack,
                spy,
                gDictionary,
                3000,
                bagSize,
                scoreDiff
            );
        }
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

        string s = getTilesToExchange(me.rack);
        strncpy(ex.exchangeLetters, s.c_str(), 7);
        ex.exchangeLetters[7] = '\0';

        return ex;
    }

    DifferentialMove diff = calculateDifferential(state.board, bestMove);
    if (diff.row == -1) return Move(MoveType::PASS);

    Move result;
    result.type = MoveType::PLAY;
    result.row = diff.row;
    result.col = diff.col;

    strncpy(result.word, diff.word.c_str(), 15);
    result.word[15] = '\0';

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