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
/*
// For keeping the input logic clean
void ClearInput() {
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
*/

Move HumanPlayer::getMove(const Board &bonusBoard,
                 const LetterBoard &letters,
                 const BlankBoard &blankBoard,
                 const TileBag &bag,
                 const Player &me,
                 const Player &opponent,
                 int playerNum) {

        Player meCopy = me;
        TileRack &myRack = meCopy.rack;

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

        if (!(cin >> input)) {
            return Move::Quit();
        }

        if (input.size() != 1) {
            cout << "Invalid input. Please enter only one character\n";
            continue;
        }

        char choice = static_cast<char>(toupper(static_cast<unsigned char>(input[0])));

        // Commands
        if (choice == 'B') {
            Renderer::printBoard(bonusBoard, letters);
            cout << "Scores: Player 1 = " << (playerNum == 1 ? me.score : opponent.score)
                 << " | Player 2 = " << (playerNum == 2 ? me.score : opponent.score) << endl;
            cout << "Rack:\n";
            Renderer::printRack(myRack);
            continue;
        }

        if (choice == 'T') {
            Player tempPlayers[2];

            int myIndex = playerNum - 1; //1-based to 0-based
            int opponentIndex = 1 - myIndex;
            tempPlayers[myIndex] = me;
            tempPlayers[opponentIndex] = opponent;

            Renderer::showUnseenTiles(bag, tempPlayers, myIndex);
            continue;
        }

        if (choice == 'R') {
            // C++ Learning
            // const_cast removes the const in const TileBag &bag
            Move rackMove = handleRackLogic(myRack, const_cast<TileBag &>(bag));
            if (rackMove.type == MoveType::EXCHANGE) {
                return rackMove;
            }
            continue;
        }

        // Passing
        if (choice == 'P') {
            return Move::Pass();
        }

        if (choice == 'Q') {
            return Move::Quit();
        }

        if (choice == 'C') {
            return Move::Challenge();
        }

        if (choice == 'M') {
            Move move = parseMoveInput(bonusBoard, letters, blankBoard, myRack, bag);
            if (move.type == MoveType::PLAY) {
                return move;
            }
        }

    }
};
Move HumanPlayer::getEndGameDecision() {
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

        if (choice == 'P') {
            return Move::Pass();
        }
        if (choice == 'C') {
            return Move::Challenge();
        }

        cout << "Invalid choice. Enter P to pass or C to challenge\n";
    }
}

Move HumanPlayer::handleRackLogic(TileRack &rack, TileBag &bag) {

    cout << R"(Enter rack command ("4-7" to swap, "0" to shuffle, "X" to exchange): )";
    string command;
    cin >> command;

    // After an exchange the turn is over
    if (command == "X" || command == "x") {
        cout << "Enter rack indices to exchange (ex: AE?N): ";
        string lettersStr;
        cin >> lettersStr;
        return Move::Exchange(lettersStr);
    }

    bool ok = applyRackCommand(bag, rack, command);

    // Swaps/Shuffles are local
    if (ok){
        cout << "\nRack Updated\n";
        Renderer::printRack(rack);
    } else {
        cout << "\nInvalid Rack command\n";
    }
    return {MoveType::NONE};
}

Move HumanPlayer::parseMoveInput(const Board &bonusBoard,
                                 const LetterBoard &letters,
                                 const BlankBoard &blankBoard,
                                 const TileRack &rack,
                                 const TileBag &bag) {

    cout << "Enter move in format <RowCol> <H/V> <WordFromRack>\n";
    cout << "Example: A10 V RETINAS\n\n";

    string pos, dirStr, word;
    cout << "RowCol: "; cin >> pos;
    cout << "Direction (H/V): "; cin >> dirStr;
    cout << "Word (from rack only): "; cin >> word;

    if (pos.size() < 2 || pos.size() > 3) {
        cout << "Invalid Position";
        return {MoveType::NONE};
    }

    char rowChar = static_cast<char>(toupper(static_cast<unsigned char>(pos[0])));
    int row = rowChar - 'A';
    int col = 0;

    // C++ Learning
    // pos.substr(1) takes the string from index 1 foward
    // ex : "A10" -> "10"
    // and stoi turns that to an int ( "10" -> 10)
    // catch block runs if stoi() throws as exception ( "AAA" )
    try {
        col = stoi(pos.substr(1)) - 1; // 1-based to 0-based
    } catch (...) { col = -1; }
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        cout << "Position out of bounds\n";
        return {MoveType::NONE};
    }

    bool horizontal = (toupper(static_cast<unsigned char>(dirStr[0])) == 'H');

    // Constructing a temporary state for the ref
    GameState previewState;
    previewState.board = letters;
    previewState.blanks = blankBoard;
    previewState.bag = bag;

    // Only populate current player's rack for validation purposes
    previewState.players[0].rack = rack;
    previewState.currentPlayerIndex = 0;

    // Construct the move
    Move moveAttempt = Move::Play(row, col, horizontal, word);
    MoveResult preview = Referee::validateMove(previewState, moveAttempt, bonusBoard, gDictionary);

    if (!preview.success) {
        cout << "Move Failed " << preview.message << endl;
        return {MoveType::NONE};
    }

    cout << "\nProposed move score (main word + cross words): " << preview.score << endl;

    /*
    cout << "\nPreview board:\n";
    printBoard(bonusBoard, previewLetters);

    cout << "\nPreview rack:\n";
    printRack(previewRack);
    */

    cout << "\nConfirm move? (y/n): ";
    char confirm;
    cin >> confirm;
    confirm = static_cast<char>(toupper(static_cast<unsigned char>(confirm)));

    if (confirm != 'Y') {
        cout << "Move cancelled.\n";
        return {MoveType::NONE};
    }

    return moveAttempt;

}