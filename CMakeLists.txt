cmake_minimum_required(VERSION 3.11)
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
        syzygy/tbconfig.h syzygy/tbconfig.cpp syzygy/fathom.h syzygy/fathom.cpp)
set(TEST_FILES testing/runner.cpp testing/util.h testing/util.cpp
        testing/test_bb.cpp
        testing/test_board.cpp
        testing/test_perft.cpp
        testing/test_see.cpp
        testing/test_hash.cpp)
set(TOPPLE_TUNE_FILES toppletuning/main.cpp
        toppletuning/game.cpp toppletuning/game.h
        toppletuning/toppletuner.cpp toppletuning/toppletuner.h
        toppletuning/ctpl_stl.h)
set(TEXEL_TUNE_FILES texeltuning/main.cpp
        texeltuning/texel.cpp texeltuning/texel.h)

# Add version definitions
add_definitions(-DTOPPLE_VER="${TOPPLE_VERSION}")

# Download dependencies
Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.7
)
FetchContent_Declare(
        Fathom
        GIT_REPOSITORY https://github.com/jdart1/Fathom.git
        GIT_TAG        85b69687088a8baf33f911cf0ce31611906fdd12
)
FetchContent_MakeAvailable(Catch2 Fathom)

# Set up Fathom to probe syzygy tablebases
include_directories(syzygy ${fathom_SOURCE_DIR}/src)
list(APPEND SOURCE_FILES ${fathom_SOURCE_DIR}/src/tbprobe.c)

# Enable unit tests
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(CTest)
include(Catch)
add_executable(ToppleTest ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(ToppleTest Catch2::Catch2)
target_compile_options(ToppleTest PUBLIC -march=native -O3)
catch_discover_tests(ToppleTest)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
add_executable(Topple ${SOURCE_FILES} main.cpp)
add_executable(ToppleTune ${SOURCE_FILES} ${TOPPLE_TUNE_FILES})
add_executable(ToppleTexelTune ${SOURCE_FILES} ${TEXEL_TUNE_FILES})

# Link pthreads on linux
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Topple Threads::Threads)
target_link_libraries(ToppleTune Threads::Threads)
target_link_libraries(ToppleTexelTune Threads::Threads)
target_link_libraries(ToppleTest Threads::Threads)

# Set -march for the Topple target, only enable asserts for tests
target_compile_options(Topple PUBLIC -march=native -O3 -DNDEBUG)
target_compile_options(ToppleTune PUBLIC -DTOPPLE_TUNE -O3 -march=native -DNDEBUG)
target_compile_options(ToppleTexelTune PUBLIC -DTEXEL_TUNE -O3 -march=native -DNDEBUG)

# Configure the "Release" target
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(Release)
    add_executable(Topple_${TOPPLE_VERSION}_legacy ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_legacy Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_legacy PUBLIC)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_legacy)

    add_executable(Topple_${TOPPLE_VERSION}_popcnt ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_popcnt Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_popcnt PUBLIC -march=nehalem)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_popcnt)

    add_executable(Topple_${TOPPLE_VERSION}_modern ${SOURCE_FILES} main.cpp)
    target_link_libraries(Topple_${TOPPLE_VERSION}_modern Threads::Threads)
    target_compile_options(Topple_${TOPPLE_VERSION}_modern PUBLIC -march=haswell)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_modern)
endif ()
