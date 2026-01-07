#include "../include/player_controller.h"
#include "../include/human_player.h"
#include "../include/choices.h"
#include "../include/engine/referee.h"
#include "../include/engine/state.h"
#include "../include/interface/renderer.h"
#include <iostream>
#include <limits>
#include <algorithm>

using namespace std;

Move HumanPlayer::getMove(const GameState& state,
                          const Board &bonusBoard,
                          const LastMoveInfo& lastMove,
                          bool canChallenge)
{
    // Local copies for the menu loop logic
    Player meCopy = state.players[state.currentPlayerIndex];
    Player opponentCopy = state.players[1 - state.currentPlayerIndex];
    TileRack &myRack = meCopy.rack;

    // We use const_cast because handleRackLogic modifies the rack/bag
    // locally for swaps/shuffles before a move is committed.
    TileBag &mutableBag = const_cast<TileBag&>(state.bag);

    int playerNum = state.currentPlayerIndex + 1;

    while (true) {
        cout << "\nCommands (Player " << playerNum << "):\n"
             << " m -> play a move\n"
             << " r -> rack command (swap/shuffle/exchange)\n"
             << " c -> challenge last word\n"
             << " b -> show board\n"
             << " t -> show unseen tiles\n"
             << " p -> pass\n"
             << " q -> quit\n"
             << "Enter Choice: ";

        string input;
        if (!(cin >> input)) return Move(MoveType::QUIT);

        if (input.size() != 1) {
            cout << "Invalid input. Please enter only one character\n";
            continue;
        }

        char choice = static_cast<char>(toupper(static_cast<unsigned char>(input[0])));

        // Commands
        if (choice == 'B') {
            Renderer::printBoard(bonusBoard, state.board);
            cout << "Scores: Player 1 = " << state.players[0].score
                 << " | Player 2 = " << state.players[1].score << endl;
            cout << "Rack:\n";
            Renderer::printRack(myRack);
            continue;
        }

        if (choice == 'T') {
            // Reconstruct array for helper
            Player tempPlayers[2];
            tempPlayers[state.currentPlayerIndex] = meCopy;
            tempPlayers[1 - state.currentPlayerIndex] = opponentCopy;

            showTileSet(state.bag, tempPlayers, state.currentPlayerIndex);
            continue;
        }

        if (choice == 'R') {
            Move rackMove = handleRackLogic(myRack, mutableBag);
            if (rackMove.type == MoveType::EXCHANGE) {
                return rackMove;
            }
            // Swap/Shuffle are local, loop continues
            continue;
        }

        if (choice == 'P') return Move(MoveType::PASS);
        if (choice == 'Q') return Move(MoveType::QUIT);

        if (choice == 'C') {
            if (canChallenge) return Move(MoveType::CHALLENGE);
            cout << "Cannot challenge right now.\n";
            continue;
        }

        if (choice == 'M') {
            Move move = parseMoveInput(bonusBoard, state, state.board, state.blanks, myRack, state.bag);
            if (move.type == MoveType::PLAY) {
                return move;
            }
        }
    }
}

Move HumanPlayer::getEndGameResponse(const GameState& state, const LastMoveInfo& lastMove) {
    cout << "Your opponent played their last move\n"
         << "You can either PASS or CHALLENGE!" << endl
         << " p -> pass\n"
         << " c -> challenge last word\n"
         << "Enter Choice: ";

    while (true) {
        string input;
        if ((!(cin >> input)) || (input.size() != 1)) {
            cout << "Invalid input. Please enter one character (P/C)\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        char choice = static_cast<char>(toupper(static_cast<unsigned char>(input[0])));

        if (choice == 'P') return Move(MoveType::PASS);
        if (choice == 'C') return Move(MoveType::CHALLENGE);

        cout << "Invalid choice. Enter P to pass or C to challenge\n";
    }
}

Move HumanPlayer::handleRackLogic(TileRack &rack, TileBag &bag) {
    cout << R"(Enter rack command ("4-7" to swap, "0" to shuffle, "X" to exchange): )";
    string command;
    cin >> command;

    if (command == "X" || command == "x") {
        cout << "Enter rack indices to exchange (ex: AE?N): ";
        string lettersStr;
        cin >> lettersStr;
        Move m(MoveType::EXCHANGE);
        m.exchangeLetters = lettersStr;
        return m;
    }

    bool ok = applyRackCommand(bag, rack, command);

    if (ok){
        cout << "\nRack Updated\n";
        Renderer::printRack(rack);
    } else {
        cout << "\nInvalid Rack command\n";
    }
    // Return dummy PASS type (checked as != EXCHANGE in loop)
    return Move(MoveType::PASS);
}

Move HumanPlayer::parseMoveInput(const Board &bonusBoard,
                                 const GameState &state,
                                 const LetterBoard &letters,
                                 const BlankBoard &blankBoard,
                                 const TileRack &rack,
                                 const TileBag &bag)
{
    cout << "Enter move in format <RowCol> <H/V> <WordFromRack>\n";
    cout << "Example: A10 V RETINAS\n\n";

    string pos, dirStr, word;
    cout << "RowCol: "; cin >> pos;
    cout << "Direction (H/V): "; cin >> dirStr;
    cout << "Word (from rack only): "; cin >> word;

    if (pos.size() < 2 || pos.size() > 3) {
        cout << "Invalid Position\n";
        return Move(MoveType::PASS);
    }

    char rowChar = static_cast<char>(toupper(static_cast<unsigned char>(pos[0])));
    int row = rowChar - 'A';
    int col = 0;

    try {
        col = stoi(pos.substr(1)) - 1; // 1-based to 0-based
    } catch (...) { col = -1; }

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        cout << "Position out of bounds\n";
        return Move(MoveType::PASS);
    }

    bool horizontal = (toupper(static_cast<unsigned char>(dirStr[0])) == 'H');

    // Create a temporary move object for validation
    Move tempMove(MoveType::PLAY);
    tempMove.row = row;
    tempMove.col = col;
    tempMove.horizontal = horizontal;
    tempMove.word = word;
    // Normalize word to upper
    for(auto &c : tempMove.word) c = toupper(c);

    // PREVIEW via Referee (Using state from arguments)
    MoveResult preview = Referee::validateMove(state, tempMove, bonusBoard, gDictionary);

    if (!preview.success) {
        cout << "Move Failed: " << preview.message << endl;
        return Move(MoveType::PASS);
    }

    cout << "\nProposed move score (main word + cross words): " << preview.score << endl;

    cout << "\nConfirm move? (y/n): ";
    char confirm;
    cin >> confirm;
    confirm = static_cast<char>(toupper(static_cast<unsigned char>(confirm)));

    if (confirm != 'Y') {
        cout << "Move cancelled.\n";
        return Move(MoveType::PASS);
    }

    return tempMove;
}