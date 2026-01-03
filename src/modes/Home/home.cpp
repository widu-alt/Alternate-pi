#include <iostream>
#include <limits>

#include "../../../include/modes/Home/home.h"

#include "../../../include/board.h"
#include "../../../include/tiles.h"
#include "../../../include/rack.h"
#include "../../../include/dict.h"

using namespace std;

void showAboutScreen() {
    clearScreen();


    cout << R"(
                                   :::::
                                 :::::::::
                                :::::::::::::
                                    :::::::::::
                                     :::::::::::
                                    ::::::::: :::::
                                    :::   ::::::::::::
                                  ::: ::::::::::::::::::::
                                 ::::::::::::::::::::::::::::
                      :::::::::  ::::::     ::::::::::::::::::
                   ::::::::::::::::::      :: ::::::::  :::::::
                  ::::::::::: ::::::      ::: :::::::::::::::::
                  :::::::::: ::::::       :::  :::::::::::::::
                 :::::::::: :::::::      :::::::::::::::::::::
                :::::::::  :::::::    :::::::::: :::::::::::
                ::::::    ::::::  ::::::::: ::::::
                :::::     :::::::::::::   ::::::::        :::::        :::
                ::::      :::::::::::     ::::::::          ::        ::
                :::::     :::::::::::    ::::::::            :::    :::
                 ::::      :::::::::     ::::::                ::  ::
                  ::::     :::::::::                             ::
                   ::::     :::::::::                          ::  ::
                    ::::     ::::::::                        :::    :::
                      :::     ::::::::                      ::        ::
                        ::::   ::::::::                   :::         ::::
                            ::  ::::::::
                                 ::::::::                 :::
                                   ::::::::            :::::
                                      :::::: ::     ::::::
                                     :::::: ::::::::::::::
                                      :::::::::::::::::::::            :
                                      :::::::::::::::::::::::::     :::
                                        ::::::::::::::::::::::::::::::
                                          ::::::::::::::::::::::::::
                                             :::::::::::::::::::::
                                               ::::::::::::::::::
                                                     :::::::
)";

    cout << R"(

    Few words from the developer:

    For all the years I had to tolerate living I managed to never cross paths with a single crossword game other
    than the puzzels I saw in my favorite newspaper back in the day. It was called "Wijaya", and if you were to collect
    all those rare educational and actually worth while social media reels and pile it up to a 25ish amount of papers
    and ship one out every single week that would basically be that newspaper. But I was too lazy to actually fill it
    up, partially because it was in Sinhala, a language that would be a nightmare for a native english speaker to learn
    to write but to be fair the performance of the locals puts our forefathers to shame.

    The game that starts with the letter S (for legal reasons I cannot mention its name) was introduced to me by a
    friend who is happened to be a prime nerd phenotype. I didnt think much of it when I heard its about making silly
    little words to get points. But eventually I understood the complexity of this bag of tiles as I was keep playing.
    When your opponent play some word that is so niche that its only ever used in 5 instances of the entierity of
    english literature, it is less of a frustration and more of a dissapointment towards humanity for not making use
    of a language's absolute potential. But like on the second thought, I consider combo swearing as the peak of word
    saladry (ex: Sybau).

    I still dont think me or any of my peers fully understand all the laws in this game. Two weeks ago when I was
    participating to the freshers scrabble tournement representing my faculty, I had to face the same guy from the
    faculty of medicing (which I beat on first round) and at the middle of the game I was clarified that you swap
    the timer as soon as you begin exchanging, which I was taught the exact oppossite of, and which I taught the two
    other players before this match of. God knows how long they will carry that misinformation. Because I can tell that
    those two do not give two dry f***s about this game to google its rules, from their performance. I mean fair
    enough, I wouldnt either if I were forced to participate just to fill a blank. Also I lost to the medicing dude
    on that round. Now we both have one win per each but I still has net positive margin (which no one cares about, but
    still helps to hide my shame).

    Anyways, I hooked into this game enough to bother my girlfriend into playing this game with me every night and
    my toxic competitive self that was unhealed from my pvp shooter addiction started to emerge back again. She is
    not a person interested in wasting her lifespan and health by screwing up her sleep schedule, so she usually only
    wants to play a single match. But in a case where I lose, my toxicity itches my soul to not to end the night on a
    loss so I starts to bother her again for a rematch and to absolutly no ones suprise she hated me for that.
    The happiness and pride that sparked withing my primitive brain after seeing her improve and become a better player
    apperantly wasnt enough to overshadow that competitivenes. Well as any sane person would do I accepted that flaw
    and did some growing up. And as we played we improved enough to be decent players as both of us had a good mental
    vocabulary from consuming pop cultural elements (two different paths I might add).

    LEXI-PI is not a word that I added just because it rolls out of the tongue nicely. Pi is short for Pisces, which is
    an astrological sign where you can see two fish swimming in opposite directions. I am not a beliver of Zordiac
    signs by any means by the way. The Logo for this program is inspired by this in case you havent already figured out.
    Her name doesnt not have any inherit meaning but the words its derived form has meanings, which would point
    to a fish if you twitch your eyes hard enough. There is my reason.

    ---(2025/12/5)---

    LEXI_PI

    A labor of love for my amazing girlfriend.

    +----------------------------------------------------------------------------------------------------------------+

    Copyright © 2025 widuruwana. All rights reserved.

    The contents of this project are proprietary and confidential. Unauthorized copying, transferring, or reproduction
    of the contents of this project, via any medium, is strictly prohibited. The receipt or possession of the source
    code does not convey any right to use it for any purpose other than viewing it on GitHub.

    )";

    waitForQuitKey();
    clearScreen();
}

void printTitleScreen() {
    clearScreen();

    cout << R"(
.----------------.  .----------------.  .----------------.  .----------------.   .----------------.  .----------------.
| .--------------. || .--------------. || .--------------. || .--------------. | | .--------------. || .--------------. |
| |   _____      | || |  _________   | || |  ____  ____  | || |     _____    | | | |   ______     | || |     _____    | |
| |  |_   _|     | || | |_   ___  |  | || | |_  _||_  _| | || |    |_   _|   | | | |  |_   __ \   | || |    |_   _|   | |
| |    | |       | || |   | |_  \_|  | || |   \ \  / /   | || |      | |     | | | |    | |__) |  | || |      | |     | |
| |    | |   _   | || |   |  _|  _   | || |    > `' <    | || |      | |     | | | |    |  ___/   | || |      | |     | |
| |   _| |__/ |  | || |  _| |___/ |  | || |  _/ /'`\ \_  | || |     _| |_    | | | |   _| |_      | || |     _| |_    | |
| |  |________|  | || | |_________|  | || | |____||____| | || |    |_____|   | | | |  |_____|     | || |    |_____|   | |
| |              | || |              | || |              | || |              | | | |              | || |              | |
| '--------------' || '--------------' || '--------------' || '--------------' | | '--------------' || '--------------' |
'----------------'  '----------------'  '----------------'  '----------------'   '----------------'  '----------------'
)" << endl;

}

void showHowToPlayScreen() {
    clearScreen();

    // Set color for the whole manual (bright cyan)
    cout << "\033[1;36m";

    cout << R"(
+==============================================================================+
|                                 HOW TO PLAY                                  |
|                              CROSSWORD GAME (PvP)                            |
+==============================================================================+
|                           >> OBJECTIVE OF THE GAME <<                        |
|                                                                              |
|  Outscore your opponent by forming valid words on a 15x15 crossword board.   |
|  Words earn points based on letter values, premium squares, and crosswords.  |
|                                                                              |
+------------------------------------------------------------------------------+
|                         >> GAME FLOW AND TURN ORDER <<                       |
|                                                                              |
|  - Players alternate turns.                                                  |
|  - On your turn, you may perform ONE primary action:                         |
|      * Place a word                                                          |
|      * Manage rack (swap / shuffle / exchange)                               |
|      * Pass                                                                  |
|      * Challenge your opponent's last move                                   |
|      * Quit / Resign                                                         |
|                                                                              |
|  - After your chosen action, the turn passes to the opponent (except for     |
|    challenges, swaps and shuffles which do not consume a turn).              |
|                                                                              |
+------------------------------------------------------------------------------+
|                            >> BOARD COORDINATES <<                           |
|                                                                              |
|  - Rows:    A to O  (15 rows)                                                |
|  - Columns: 1 to 15 (15 columns)                                             |
|                                                                              |
|  Example placement:                                                          |
|      H8 H RAIN                                                               |
|                                                                              |
|  This places the word "RAIN" horizontally at row H, column 8.                |
|                                                                              |
+------------------------------------------------------------------------------+
|                         >> WORD PLACEMENT RULES <<                           |
|                                                                              |
|  - The first word must pass through the center square.                       |
|  - Every new word must connect to at least one existing tile.                |
|  - Words must be found in your chosen English dictionary                     |
|    (no dictionary file is included with the game).                           |
|  - Place [dictionary].txt file at \build\Release\data                        |
|  - Tiles you place must form:                                                |
|      1) A valid main word                                                    |
|      2) All valid crosswords created by your new tiles                       |
|  - Words must be contiguous with no gaps.                                    |
|                                                                              |
+------------------------------------------------------------------------------+
|                            >> SCORING SYSTEM <<                              |
|                                                                              |
|  - Each letter tile has a point value (A = 1, high-value letters are more).  |
|  - Premium squares:                                                          |
|      DL = Double Letter     TL = Triple Letter                               |
|      DW = Double Word       TW = Triple Word                                 |
|                                                                              |
|  - Premiums apply only when you place a tile on that square.                 |
|  - Every new crossword formed during your move adds to your score.           |
|                                                                              |
|  - Full Rack Bonus: using all 7 tiles in a single move gives +50 points.     |
|                                                                              |
+------------------------------------------------------------------------------+
|                             >> RACK ACTIONS <<                               |
|                                                                              |
|  - Swap positions in rack:   "4-7"  swaps tile #4 with tile #7               |
|  - Shuffle rack order:       "0"    randomizes tile order                    |
|  - Exchange tiles:           "X"    return selected tiles and draw new       |
|                               (uses your turn and closes challenge window)   |
|                                                                              |
+------------------------------------------------------------------------------+
|                              >> CHALLENGES <<                                |
|                                                                              |
|  After your opponent makes a move:                                           |
|    - You may challenge before doing anything else.(except swaps and shuffles)|
|    - Challenge checks the main word and all crosswords formed.               |
|                                                                              |
|  - If ANY word is invalid: the entire move is undone.                        |
|  - If ALL words are valid: you lose the challenge and your opponent gains    |
|    +5 points.                                                                |
|                                                                              |
|  - A challenge does NOT consume your turn.                                   |
|  - The challenge opportunity closes if you pass, exchange, or play a move.   |
|                                                                              |
+------------------------------------------------------------------------------+
|                         >> TILE TRACKING SYSTEM <<                           |
|                                                                              |
|  The game provides a realistic tile-tracking style view:                     |
|                                                                              |
|  - Before endgame:                                                           |
|      UNSEEN TILES = bag contents + opponent rack                             |
|      (exact letters in opponent rack are hidden)                             |
|                                                                              |
|  - Endgame (when 7 or fewer unseen tiles remain):                            |
|      The exact content of the opponent's rack is revealed,                   |
|                                                                              |
+------------------------------------------------------------------------------+
|                               >> ENDGAME <<                                  |
|                                                                              |
|  The game ends when:                                                         |
|                                                                              |
|  1) A player plays their last tile AND the tile bag is empty.                |
|     - The opponent then gets one chance to PASS or CHALLENGE.                |
|                                                                              |
|  2) Six consecutive scoreless turns occur (passes or exchanges).             |
|                                                                              |
|  3) A player resigns (Quit).                                                 |
|                                                                              |
|  Endgame scoring:                                                            |
|    - The player who goes out receives doubles the total point value of       |
|      tiles in the opponent's rack.                                           |
|    - In the six-scoreless-turn endgame, both players lose double their       |
|      own rack points.                                                        |
|                                                                              |
+------------------------------------------------------------------------------+
|                       >> IN-GAME CONTROLS (SUMMARY) <<                       |
|                                                                              |
|  M  -> Place a word                                                          |
|  R  -> Rack commands (swap / shuffle / exchange)                             |
|  C  -> Challenge opponent's last move                                        |
|  B  -> Show board and scores                                                 |
|  T  -> Show unseen tiles (tile tracking view)                                |
|  P  -> Pass                                                                  |
|  Q  -> Resign / quit game                                                    |
+==============================================================================+
)";

    // Reset the color
    cout << "\033[0m";

    waitForQuitKey();
    clearScreen();
}

void wordWizard(const string &word) {
    clearScreen();

    cout << R"(
                     .----------------.  .----------------.  .----------------.  .----------------.
                    | .--------------. || .--------------. || .--------------. || .--------------. |
                    | | _____  _____ | || |     ____     | || |  _______     | || |  ________    | |
                    | ||_   _||_   _|| || |   .'    `.   | || | |_   __ \    | || | |_   ___ `.  | |
                    | |  | | /\ | |  | || |  /  .--.  \  | || |   | |__) |   | || |   | |   `. \ | |
                    | |  | |/  \| |  | || |  | |    | |  | || |   |  __ /    | || |   | |    | | | |
                    | |  |   /\   |  | || |  \  `--'  /  | || |  _| |  \ \_  | || |  _| |___.' / | |
                    | |  |__/  \__|  | || |   `.____.'   | || | |____| |___| | || | |________.'  | |
                    | |              | || |              | || |              | || |              | |
                    | '--------------' || '--------------' || '--------------' || '--------------' |
                     '----------------'  '----------------'  '----------------'  '----------------'
 .----------------.  .----------------.  .----------------.  .----------------.  .----------------.  .----------------.
| .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |
| | _____  _____ | || |     _____    | || |   ________   | || |      __      | || |  _______     | || |  ________    | |
| ||_   _||_   _|| || |    |_   _|   | || |  |  __   _|  | || |     /  \     | || | |_   __ \    | || | |_   ___ `.  | |
| |  | | /\ | |  | || |      | |     | || |  |_/  / /    | || |    / /\ \    | || |   | |__) |   | || |   | |   `. \ | |
| |  | |/  \| |  | || |      | |     | || |     .'.' _   | || |   / ____ \   | || |   |  __ /    | || |   | |    | | | |
| |  |   /\   |  | || |     _| |_    | || |   _/ /__/ |  | || | _/ /    \ \_ | || |  _| |  \ \_  | || |  _| |___.' / | |
| |  |__/  \__|  | || |    |_____|   | || |  |________|  | || ||____|  |____|| || | |____| |___| | || | |________.'  | |
| |              | || |              | || |              | || |              | || |              | || |              | |
| '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------'
)" << endl;

    int wordNo = 0;;

    while (true) {

        int amount;

        cout << "Enter the number of the words to be challenged: " << endl;

        if (!(cin >> amount)) {
            cout << "Invalid Input. Please enter the desired number\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (amount <= 0) {
            cout << "Please enter a positive number.\n";
            continue;
        }

        wordNo = amount;
        break;

    }

    vector<string> challengedWords;
    challengedWords.reserve(wordNo);

    cout << "Enter words to be challenged: " << endl;
    for (int i = 0; i < wordNo; i++) {
        cout << "----> ";
        string input;
        cin >> input;
        challengedWords.push_back(input);
    }

    bool playIsValid = true;

    for ( string &wo: challengedWords) {
        if (!isValidWord(wo)) {
            playIsValid = false;
            break;
        }
    }

    if (playIsValid) {
        cout << "\033[32m" << R"(
`7MM"""Mq.`7MMF'            db   `YMM'   `MM'    `7MMF' .M"""bgd     `7MMF'   `7MF' db      `7MMF'      `7MMF'`7MM"""Yb.
  MM   `MM. MM             ;MM:    VMA   ,V        MM  ,MI    "Y       `MA     ,V  ;MM:       MM          MM    MM    `Yb.
  MM   ,M9  MM            ,V^MM.    VMA ,V         MM  `MMb.            VM:   ,V  ,V^MM.      MM          MM    MM     `Mb
  MMmmdM9   MM           ,M  `MM     VMMP          MM    `YMMNq.         MM.  M' ,M  `MM      MM          MM    MM      MM
  MM        MM      ,    AbmmmqMA     MM           MM  .     `MM         `MM A'  AbmmmqMA     MM      ,   MM    MM     ,MP
  MM        MM     ,M   A'     VML    MM           MM  Mb     dM          :MM;  A'     VML    MM     ,M   MM    MM    ,dP'
.JMML.    .JMMmmmmMMM .AMA.   .AMMA..JMML.       .JMML.P"Ybmmd"            VF .AMA.   .AMMA..JMMmmmmMMM .JMML..JMMmmmdP'
)" << "\033[0m" << endl;
    } else {
        cout << "\033[31m" << R"(
`7MM"""Mq.`7MMF'            db   `YMM'   `MM'    `7MMF' .M"""bgd     `7MMF'`7MN.   `7MF'`7MMF'   `7MF' db      `7MMF'      `7MMF'`7MM"""Yb.
  MM   `MM. MM             ;MM:    VMA   ,V        MM  ,MI    "Y       MM    MMN.    M    `MA     ,V  ;MM:       MM          MM    MM    `Yb.
  MM   ,M9  MM            ,V^MM.    VMA ,V         MM  `MMb.           MM    M YMb   M     VM:   ,V  ,V^MM.      MM          MM    MM     `Mb
  MMmmdM9   MM           ,M  `MM     VMMP          MM    `YMMNq.       MM    M  `MN. M      MM.  M' ,M  `MM      MM          MM    MM      MM
  MM        MM      ,    AbmmmqMA     MM           MM  .     `MM       MM    M   `MM.M      `MM A'  AbmmmqMA     MM      ,   MM    MM     ,MP
  MM        MM     ,M   A'     VML    MM           MM  Mb     dM       MM    M     YMM       :MM;  A'     VML    MM     ,M   MM    MM    ,dP'
.JMML.    .JMMmmmmMMM .AMA.   .AMMA..JMML.       .JMML.P"Ybmmd"      .JMML..JML.    YM        VF .AMA.   .AMMA..JMMmmmmMMM .JMML..JMMmmmdP'
)" << "\033[0m" << endl;
    }

    waitForQuitKey();
    clearScreen();
}

