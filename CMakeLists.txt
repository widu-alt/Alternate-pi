cmake_minimum_required(VERSION 4.0)
project(scrabblePi)

set(CMAKE_CXX_STANDARD 20)

add_executable(scrabblePi src/board.cpp
        include/board.h
        include/tiles.h
        src/tiles.cpp
        include/rack.h
        src/rack.cpp
        src/main.cpp
        include/move.h
        src/dict.cpp
        include/dict.h
        src/choices.cpp
        include/choices.h
        src/modes/PvP/pvp.cpp
        include/modes/PvP/pvp.h
        src/modes/Home/home.cpp
        include/modes/Home/home.h
        include/player_controller.h
        src/human_player.cpp
        include/human_player.h
        include/dictionary.h
        src/dictionary.cpp
        include/heuristics.h
        include/fast_constraints.h
        src/fast_constraints.cpp
        include/ai_player.h
        src/ai_player.cpp
        include/modes/PvE/pve.h
        src/modes/PvE/pve.cpp
        src/modes/AiAi/aiai.cpp
        include/modes/AiAi/aiai.h
        include/tile_tracker.h
        include/spectre/move_generator.h
        src/spectre/move_generator.cpp
        include/spectre/vanguard.h
        src/spectre/vanguard.cpp
        include/spectre/spy.h
        src/spectre/spy.cpp
        include/spectre/judge.h
        src/spectre/judge.cpp
        include/spectre/logger.h
        include/engine/types.h
        include/engine/state.h
        include/engine/referee.h
        src/engine/referee.cpp
        include/interface/renderer.h
        src/interface/renderer.cpp)
