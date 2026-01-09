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
#include <map>

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

Move calculateDifferential(const Board& board, const spectre::MoveCandidate& cand) {
    Move m;
    m.type = MoveType::PLAY;
    m.row = cand.row;
    m.col = cand.col;
    m.horizontal = cand.isHorizontal;
    m.score = cand.score;

    int r = cand.row;
    int c = cand.col;
    int dr = cand.isHorizontal ? 0 : 1;
    int dc = cand.isHorizontal ? 1 : 0;

    for (int i = 0; cand.word[i] != '\0'; ++i) {
        if (board.boundsCheck(r, c)) {
            if (!board.hasTile(r, c)) {
                m.word += cand.word[i];
            }
        }
        r += dr;
        c += dc;
    }
    return m;
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

Move AIPlayer::getMove(const GameState& state, const Board& bonusBoard, const LastMoveInfo& lastMove, bool isFirstTurn) {

    // ---------------------------------------------------------
    // BRAIN 1: SPEEDI_PI (Greedy) - Logic for P1 if applicable
    // ---------------------------------------------------------
    if (state.currentPlayerIndex == 0) {
        spectre::MoveCandidate cand = spectre::Vanguard::search(
             (const spectre::LetterBoard&)state.board,
             bonusBoard,
             state.players[0].rack,
             spy,
             gDictionary,
             3000,
             (int)state.bag.size(),
             0
        );

        if (cand.word[0] != '\0') return calculateDifferential(state.board, cand);
        return Move(MoveType::PASS);
    }

    // ---------------------------------------------------------
    // BRAIN 2: CUTIE_PI (Spectre Engine)
    // ---------------------------------------------------------

    const Player& me = state.players[state.currentPlayerIndex];
    const Player& opp = state.players[1 - state.currentPlayerIndex];

    // 1. Context & Spy Updates
    int scoreDiff = me.score - opp.score;
    int bagSize = state.bag.size();

    spy.observeOpponentMove(lastMove, (const spectre::LetterBoard&)state.board);
    spy.updateGroundTruth((const spectre::LetterBoard&)state.board, me.rack, state.bag);

    spectre::MoveCandidate bestCand;

    // 2. Select Brain (Endgame vs Midgame)
    if (state.bag.empty()) {
        // >>> THE JUDGE <<<
        std::vector<char> inferredOpp = spy.generateWeightedRack();
        spectre::TileRack oppRack;
        for(char c : inferredOpp) { spectre::Tile t; t.letter=c; t.points=0; oppRack.push_back(t); }

        // Judge returns a global 'Move' object, we convert to Candidate for consistent handling below
        spectre::Move jMove = spectre::Judge::solveEndgame(
            (const spectre::LetterBoard&)state.board,
            bonusBoard,
            me.rack,
            oppRack,
            gDictionary
        );

        bestCand.row = jMove.row;
        bestCand.col = jMove.col;
        bestCand.isHorizontal = jMove.horizontal;
        bestCand.score = 1000;

        size_t len = jMove.word.length();
        for(size_t i=0; i<len && i<15; i++) bestCand.word[i] = jMove.word[i];
        bestCand.word[len] = '\0';
    }
    else {
        // >>> VANGUARD <<<
        bestCand = spectre::Vanguard::search(
            (const spectre::LetterBoard&)state.board,
            bonusBoard,
            me.rack,
            spy,
            gDictionary,
            3000,
            bagSize,
            scoreDiff
        );
    }

    // ---------------------------------------------------------
    // SAFETY VALVE: EXCHANGE LOGIC
    // ---------------------------------------------------------
    // If Vanguard returns a PASS (empty word) in Midgame, we MUST Exchange.

    if (bestCand.word[0] == '\0' && state.bag.size() >= 7) {
        std::string toxic = "QJZXV";
        std::string toExchange = "";
        std::map<char, int> counts;

        // A. Mark Toxic Tiles
        for (const auto& tile : me.rack) {
            counts[tile.letter]++;
            if (toxic.find(tile.letter) != std::string::npos) {
                toExchange += tile.letter;
            }
        }

        // B. Mark Duplicates
        if (toExchange.empty()) {
            for (const auto& tile : me.rack) {
                if (counts[tile.letter] > 2) {
                    toExchange += tile.letter;
                    counts[tile.letter]--;
                }
            }
        }

        // C. Forced Cycle (Dump 1 tile)
        if (toExchange.empty() && !me.rack.empty()) {
             toExchange += me.rack[0].letter;
        }

        if (!toExchange.empty()) {
            Move exchangeMove;
            exchangeMove.type = MoveType::EXCHANGE;
            exchangeMove.word = toExchange;
            return exchangeMove;
        }
    }

    // ---------------------------------------------------------
    // EXECUTE PLAY
    // ---------------------------------------------------------
    if (bestCand.word[0] != '\0') {
        return calculateDifferential(state.board, bestCand);
    }

    return Move(MoveType::PASS);
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