cmake_minimum_required(VERSION 3.8)
project(Topple)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-march=skylake -static -Ofast -pedantic -Wall")

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
        smp.h smp.cpp
        syzygy/tbcore.h
        syzygy/tbprobe.h syzygy/tbprobe.cpp syzygy/tbresolve.h syzygy/tbresolve.cpp)
set(TEST_FILES testing/catch.hpp testing/runner.cpp testing/util.h testing/util.cpp
        testing/tests/test_bb.cpp
        testing/tests/test_board.cpp
        testing/tests/test_perft.cpp
        testing/tests/test_see.cpp)
set(TUNE_FILES tuning/tunemain.cpp tuning/tuner.cpp tuning/tuner.h)

add_executable(Topple ${SOURCE_FILES} main.cpp)
add_executable(ToppleTest ${SOURCE_FILES} ${TEST_FILES})
add_executable(ToppleTune ${SOURCE_FILES} ${TUNE_FILES})

set_target_properties(ToppleTune PROPERTIES COMPILE_FLAGS "-DTOPPLE_TUNE")