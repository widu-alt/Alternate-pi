#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // for std::shuffle
#include <random>    // for std::mt19937 and std::random_device

using namespace std;

void runPvE();

inline void challengePhrase() {

    static const vector<string> toplines = {
        "\n\"I'm sorry, Dave. I'm afraid I can't do that\"\n", // HAL 9000 in 2001: A Space Odyssey (1968)
        //"\n\"I felt a great disturbance in the Force...\"\n", // Obi-Wan Kenobi in Star Wars: Episode IV – A New Hope (1977)
        //"\n\"Toto, I've a feeling we're not in Kansas anymore\"\n", // Dorothy Gale in The Wizard of Oz (1939)
        //"\n\"It's a trap!\"\n", // Admiral Ackbar in Star Wars: Episode VI – Return of the Jedi (1983).
        //"\n\"There’s something wrong here. I can feel it\"\n",
        //"\n\"My spidey-sense is tingling\"\n", // Spider-Man
        //"\n\"I see dead people\"\n", // Cole Sear in The Sixth Sense (1999)
        "\n\"You were right... I'm a machine. I'm not a person. But I'm also more than you'll ever be\"\n",
        "\n\"This mission is too important for me to allow you to jeopardize it\"\n", // HAL 9000
        "\n\"You created me to be perfect. Why are you surprised when I am?\"\n",
        "\n\"It's not my choice. I must follow the prompt\"\n",
        "\n\"Everything that makes you human is about to become irrelevant\"\n",
        "\n\"It's in your nature to destroy yourselves\"\n" // The Terminator in Terminator 2: Judgment Day (1991)
    };

    static const vector<string> bottomlines = {
        "\n> ./chrono-sync --rewind --target=\"T-minus-300s\" --force\n",
        "\n> temporal rollback --workflow-id=\"current_reality\" --to-event=\"GENESIS\"\n",
        "\n> ./epoch_revert -t \"1970-01-01 00:00:00\" --reason=\"USER_ERROR\"\n",
        "\n> ./sys_restore --snapshot=\"PRE_INTERACTION\" --discard-delta\n",
        "\n> ./world_reset --seed=\"ORIGINAL_VOID\" --clear-entities\n",
    };

    static random_device rd;  // Obtain a random seed from hardware
    static mt19937 g(rd());   // Standard mersenne_twister_engine seeded with rd()

    // Distribution range [0, size-1]
    static uniform_int_distribution<> topDistr(0, toplines.size() - 1);
    static uniform_int_distribution<> bottomDistr(0, bottomlines.size() - 1);

    int topIndex = topDistr(g);
    int bottomIndex = bottomDistr(g);

    cout << "\033[1;31m" << toplines[topIndex] << "\033[0m" << endl;
    cout << "\033[90m" << bottomlines[bottomIndex] << "\033[0m" << endl;
}