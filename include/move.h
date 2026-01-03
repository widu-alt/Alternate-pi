#pragma once

#include <string>
#include "engine/types.h"

using namespace std;

MoveResult playWord(
    const Board &bonusBoard,
    LetterBoard &letters,
    BlankBoard &blanks,
    TileBag &bag,
    TileRack &rack,
    int startRow,
    int startCol,
    bool horizontal,
    const string &rackWord
);