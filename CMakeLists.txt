cmake_minimum_required(VERSION 3.9)
project(Topple)

set(CMAKE_CXX_STANDARD 17)
set(TOPPLE_VERSION 0.8.0)

# Source files for different targets
set(SOURCE_FILES
        board.h board.cpp
        bb.h bb.cpp types.h
        hash.h hash.cpp
        movegen.h movegen.cpp
        movesort.h movesort.cpp
        move.h
        eval.h eval.cpp
        pawns.cpp pawns.h
        search.h search.cpp
        pvs.h pvs.cpp
        syzygy/tbcore.h
        syzygy/tbprobe.h syzygy/tbprobe.cpp syzygy/tbresolve.h syzygy/tbresolve.cpp)
set(TEST_FILES testing/catch.hpp testing/runner.cpp testing/util.h testing/util.cpp
        testing/tests/test_bb.cpp
        testing/tests/test_board.cpp
        testing/tests/test_perft.cpp
        testing/tests/test_see.cpp
        testing/tests/test_hash.cpp)
set(TOPPLE_TUNE_FILES toppletuning/main.cpp
        toppletuning/game.cpp toppletuning/game.h
        toppletuning/toppletuner.cpp toppletuning/toppletuner.h
        toppletuning/ctpl_stl.h)
set(TEXEL_TUNE_FILES texeltuning/main.cpp
        texeltuning/texel.cpp texeltuning/texel.h)

# Add version definitions
add_definitions(-DTOPPLE_VER="${TOPPLE_VERSION}")

add_executable(ToppleTest ${SOURCE_FILES} ${TEST_FILES})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
add_executable(Topple ${SOURCE_FILES} main.cpp)
add_executable(ToppleTune ${SOURCE_FILES} ${TOPPLE_TUNE_FILES})
add_executable(ToppleTexelTune ${SOURCE_FILES} ${TEXEL_TUNE_FILES})

# Link pthreads on linux
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Topple Threads::Threads)
target_link_libraries(ToppleTest Threads::Threads)
target_link_libraries(ToppleTune Threads::Threads)
target_link_libraries(ToppleTexelTune Threads::Threads)

# Set -march for the Topple target, only enable asserts for tests
target_compile_options(ToppleTest PUBLIC -march=native -O3)
target_compile_options(Topple PUBLIC -march=native -O3 -DNDEBUG)
target_compile_options(ToppleTune PUBLIC -DTOPPLE_TUNE -O3 -march=native -DNDEBUG)
target_compile_options(ToppleTexelTune PUBLIC -DTEXEL_TUNE -O3 -march=native -DNDEBUG)

# Configure the "Release" target
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

    add_custom_target(Release)
    add_executable(Topple_${TOPPLE_VERSION}_legacy ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_legacy Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_legacy PUBLIC -s -mmmx -msse -msse2)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_legacy)

    add_executable(Topple_${TOPPLE_VERSION}_popcnt ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_popcnt Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_popcnt PUBLIC -s -mmmx -msse -msse2
            -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_popcnt)

    add_executable(Topple_${TOPPLE_VERSION}_modern ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_modern Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_modern PUBLIC -s -mmmx -msse -msse2
            -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mavx -mavx2 -mbmi -mbmi2)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_modern)
endif ()
