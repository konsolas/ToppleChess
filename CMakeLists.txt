cmake_minimum_required(VERSION 3.8)
project(Topple)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-march=native -Ofast -g -static -Wall")

set(SOURCE_FILES board.cpp board.h bb.h bb.cpp types.h hash.h hash.cpp movegen.cpp movegen.h move.h eval.cpp eval.h search.cpp search.h move.cpp)
add_executable(Topple ${SOURCE_FILES} main.cpp)

set(TEST_FILES ${SOURCE_FILES} testing/catch.hpp testing/runner.cpp testing/tests/test_bb.cpp testing/util.h testing/util.cpp testing/tests/test_board.cpp testing/tests/test_perft.cpp testing/tests/test_see.cpp)
add_executable(ToppleTest ${TEST_FILES})