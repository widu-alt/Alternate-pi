#include "../include/tiles.h"
#include "../include/rack.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

TileBag createStandardTileBag() {
    TileBag bag;

    /*C++ LEARNING
        [&bag] means the lambda wants to acces to the variable bag and it wants access by reference.
        so the lambda can modify the baf directly.
        letter -> The letter to be added
        points -> The number of points the tile is worth
        count -> How many of those tiles to be added
        ex: addTiles('A', 1, 9) means 9 tiles of letter A each worth 1 point are added to the bag.
        bag.push_back(Tile{'A', 1}); x 9 times
     */
    auto addTiles = [&bag](char letter, int points, int count) {
        for (int i = 0; i < count; i++) {
            bag.push_back(Tile{letter, points});
        }
    };

    // Letter distribution:
    // E×12; A,I×9; O×8; N,R,T×6; L,S,U,D×4; G×3;
    // B,C,M,P,F,H,V,W,Y×2; K,J,X,Q,Z×1; blanks×2

    //1-point letters
    addTiles('E', 1, 12);
    addTiles('A', 1, 9);
    addTiles('I', 1,  9);
    addTiles('O', 1,  8);
    addTiles('N', 1,  6);
    addTiles('R', 1,  6);
    addTiles('T', 1,  6);
    addTiles('L', 1,  4);
    addTiles('S', 1,  4);
    addTiles('U', 1,  4);

    // 2-point letters
    addTiles('D', 2, 4);
    addTiles('G', 2, 3);

    // 3-point letters
    addTiles('B', 3, 2);
    addTiles('C', 3, 2);
    addTiles('M', 3, 2);
    addTiles('P', 3, 2);

    // 4-point letters
    addTiles('F', 4, 2);
    addTiles('H', 4, 2);
    addTiles('V', 4, 2);
    addTiles('W', 4, 2);
    addTiles('Y', 4, 2);

    // 5-point letter
    addTiles('K', 5, 1);

    // 8-point letters
    addTiles('J', 8, 1);
    addTiles('X', 8, 1);

    // 10-point letters
    addTiles('Q',10, 1);
    addTiles('Z',10, 1);

    // Blanks: use '?' as the letter, 0 points
    addTiles('?', 0, 2);

    return bag;
}

//Suffels the tile bag (use once at game start)
void shuffleTileBag(TileBag &bag) {
    static mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    shuffle(bag.begin(), bag.end(), rng);
}
