cmake_minimum_required(VERSION 3.9)
project(Topple)

set(CMAKE_CXX_STANDARD 17)
set(TOPPLE_VERSION 0.7.5)

# Source files for different targets
set(SOURCE_FILES
        board.h board.cpp
        endgame.h endgame.cpp
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
set(TUNE_FILES tuning/tunemain.cpp tuning/tuner.cpp tuning/tuner.h)

# Add version definitions
add_definitions(-DTOPPLE_VER="${TOPPLE_VERSION}")

add_executable(ToppleTest ${SOURCE_FILES} ${TEST_FILES})
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
add_executable(Topple ${SOURCE_FILES} main.cpp)
add_executable(ToppleTune ${SOURCE_FILES} ${TUNE_FILES})

# Link pthreads on linux
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Topple Threads::Threads)
target_link_libraries(ToppleTest Threads::Threads)
target_link_libraries(ToppleTune Threads::Threads)

# Need static compile for MinGW on windows, but cannot compile statically on unix.
if (MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif ()

# Set -march for the Topple target
target_compile_options(Topple PUBLIC -march=native -O3 -DNDEBUG) # NDEBUG to disable asserts
target_compile_options(ToppleTune PUBLIC -DTOPPLE_TUNE -O3 -march=native -DNDEBUG)

# Configure the "Release" target
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(Release)
    add_executable(Topple_${TOPPLE_VERSION}_legacy ${SOURCE_FILES} main.cpp)
    target_compile_options(Topple_${TOPPLE_VERSION}_legacy PUBLIC -s)
    add_dependencies(Release Topple_${TOPPLE_VERSION}_legacy)
    foreach(RELEASE_ARCH core2 nehalem westmere sandybridge ivybridge haswell broadwell skylake bdver4 znver1 znver2)
        add_executable(Topple_${TOPPLE_VERSION}_${RELEASE_ARCH} ${SOURCE_FILES} main.cpp)
        target_compile_options(Topple_${TOPPLE_VERSION}_${RELEASE_ARCH} PUBLIC -s -march=${RELEASE_ARCH})
        add_dependencies(Release Topple_${TOPPLE_VERSION}_${RELEASE_ARCH})
    endforeach(RELEASE_ARCH)
endif ()
