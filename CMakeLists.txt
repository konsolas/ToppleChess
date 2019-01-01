cmake_minimum_required(VERSION 3.8)
project(Topple)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Ofast -std=c++17 -pedantic -Wall")

set(SOURCE_FILES
        board.h board.cpp
        endgame.h endgame.cpp
        bb.h bb.cpp types.h
        hash.h hash.cpp
        movegen.h movegen.cpp
        movesort.h movesort.cpp
        move.h
        eval.h eval.cpp
        search.h search.cpp
)
add_executable(Topple ${SOURCE_FILES} main.cpp)

set(TEST_FILES ${SOURCE_FILES} testing/catch.hpp testing/runner.cpp testing/util.h testing/util.cpp
        testing/tests/test_bb.cpp
        testing/tests/test_board.cpp
        testing/tests/test_perft.cpp
        testing/tests/test_see.cpp
)
add_executable(ToppleTest ${TEST_FILES})