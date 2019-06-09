//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_TYPES_H
#define TOPPLE_TYPES_H

#include <cstdint>
#include <chrono>

#define INF 32767
#define TO_MATE_SCORE(ply) (INF - (ply))
#define TO_MATE_PLY(score) (INF - (score))
#define MINCHECKMATE (TO_MATE_SCORE(MAX_TB_PLY))

#define MAX_TB_PLY 1024
#define MAX_PLY 128

const int TIMEOUT = -INF * 2;

constexpr unsigned int MB = 1048576;

/**
 * BitBoard type.
 */
typedef uint64_t U64;
typedef std::chrono::steady_clock engine_clock;

const U64 ONES = ~U64(0);

#define CHRONO_DIFF(start, finish) std::chrono::duration_cast<std::chrono::milliseconds>((finish) - (start)).count()

enum Piece {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Team {
    WHITE, BLACK
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};

const uint8_t MIRROR_TABLE[64] = {
        56, 57, 58, 59, 60, 61, 62, 63,
        48, 49, 50, 51, 52, 53, 54, 55,
        40, 41, 42, 43, 44, 45, 46, 47,
        32, 33, 34, 35, 36, 37, 38, 39,
        24, 25, 26, 27, 28, 29, 30, 31,
        16, 17, 18, 19, 20, 21, 22, 23,
        8, 9, 10, 11, 12, 13, 14, 15,
        0, 1, 2, 3, 4, 5, 6, 7
};

enum Direction {
    D_SW = -9, D_S = -8, D_SE = -7, D_W = -1, D_X = 0, D_E = 1, D_NW = 7, D_N = 8, D_NE = 9
};

constexpr Direction rel_offset(Team side, Direction dir) {
    return side ? Direction(-dir) : dir;
}

constexpr uint8_t rel_sq(Team side, uint8_t square) {
    return side ? MIRROR_TABLE[square] : square;
}

constexpr uint8_t rel_rank(Team side, uint8_t rank) {
    return side ? uint8_t(7 - rank) : rank;
}

#endif //TOPPLE_TYPES_H
