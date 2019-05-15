//
// Created by Vincent on 22/09/2017.
//

#ifndef TOPPLE_BB_H
#define TOPPLE_BB_H

#include "types.h"

#include <string>
#include <cassert>

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include <intrin.h>
#endif

namespace bb_magics {
    extern const unsigned int b_shift[64];
    extern const U64 b_magics[64];
    extern const U64 b_mask[64];
    extern const U64 *b_index[64];

    extern const unsigned int r_shift[64];
    extern const U64 r_magics[64];
    extern const U64 r_mask[64];
    extern const U64 *r_index[64];

    inline U64 bishop_moves(const uint8_t &square, const U64 &occupancy) {
        return *(b_index[square] + (((occupancy & b_mask[square]) * b_magics[square])
                >> b_shift[square]));
    }

    inline U64 rook_moves(const uint8_t &square, const U64 &occupancy) {
        return *(r_index[square] + (((occupancy & r_mask[square]) * r_magics[square])
                >> r_shift[square]));
    }
}

namespace bb_util {
    extern uint8_t sq_index[8][8];
    extern uint8_t file_index[64];
    extern uint8_t rank_index[64];

    extern U64 between[64][64];
    extern U64 line[64][64];
    extern U64 ray[64][64];
    extern U64 file[8];
}

namespace bb_normal_moves {
    extern U64 king_moves[64];
    extern U64 knight_moves[64];
    extern U64 pawn_moves_x1[2][64];
    extern U64 pawn_moves_x2[2][64];
    extern U64 pawn_caps[2][64];
}

namespace bb_intrin {
    inline uint8_t lsb(U64 b) {
        assert(b);
#if defined(__GNUC__)
        return Square(__builtin_ctzll(b));
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        unsigned long idx;
        _BitScanForward64(&idx, b);
        return Square(idx);
#else
#error "No intrinsic lsb/bitscanforward/ctz available"
#endif
    }

    inline uint8_t msb(U64 b) {
        assert(b);
#if defined(__GNUC__)
        return Square(63 - __builtin_clzll(b));
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        unsigned long idx;
        _BitScanReverse64(&idx, b);
        return Square(idx);
#else
#error "No intrinsic msb/bitscanreverse/clz available"
#endif
    }

    inline int pop_count(const U64 &b) {
#if defined(__GNUC__)
        return __builtin_popcountll(b);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        return (int) _mm_popcnt_u64(b);
#else
#error "No intrinsic popcnt available"
#endif
    }
}

/**
 * Initialise bitboard tables. Must be called before use of any other functions in this header.
 */
void init_tables();

/**
 * Generate a bitboard of possible moves from the {@code square}, assuming that {@code occupied} is a bitboard which
 * represents the occupied square on the board.
 *
 * For non-sliding piece types (KNIGHT, KING), this function returns all possible moves regardless of occupied squares.
 * For sliding piece types (ROOK, BISHOP, QUEEN), this function returns a bitboard of sliding attacks, stopping whenever
 * the piece hits an occupied square. The resulting bitboard will include the occupied squares that were hit.
 * For PAWNs, the resulting bitboard will include capture (diagonal) moves if the target square is in the occupied
 * array. Normal moves will only be returned if the path to the target square is not blocked.
 *
 * The {@code side} parameter is irrelevant for all pieces apart from pawns.
 *
 * @param type piece type
 * @param side side which owns the piece
 * @param square start square of piece
 * @param occupied bitboard of occupied squares
 * @return possible moves of the piece
 */
template<Piece TYPE>
inline U64 find_moves(Team side, uint8_t square, U64 occupied);

template<>
inline U64 find_moves<PAWN>(Team side, uint8_t square, U64 occupied) {
    U64 result = 0;
    result |= occupied & bb_normal_moves::pawn_caps[side][square];
    if (!(occupied & bb_normal_moves::pawn_moves_x1[side][square])) {
        result |= bb_normal_moves::pawn_moves_x1[side][square];
        if (!(occupied & bb_normal_moves::pawn_moves_x2[side][square])) {
            result |= bb_normal_moves::pawn_moves_x2[side][square];
        }
    }
    return result;
}

template<>
inline U64 find_moves<KNIGHT>(Team side, uint8_t square, U64 occupied) {
    return bb_normal_moves::knight_moves[square];
}

template<>
inline U64 find_moves<BISHOP>(Team side, uint8_t square, U64 occupied) {
    return bb_magics::bishop_moves(square, occupied);
}

template<>
inline U64 find_moves<ROOK>(Team side, uint8_t square, U64 occupied) {
    return bb_magics::rook_moves(square, occupied);
}

template<>
inline U64 find_moves<QUEEN>(Team side, uint8_t square, U64 occupied) {
    return bb_magics::bishop_moves(square, occupied) | bb_magics::rook_moves(square, occupied);
}

template<>
inline U64 find_moves<KING>(Team side, uint8_t square, U64 occupied) {
    return bb_normal_moves::king_moves[square];
}

inline U64 find_moves(Piece type, Team side, uint8_t square, U64 occupied) {
    switch (type) {
        case PAWN:
            return find_moves<PAWN>(side, square, occupied);
        case KNIGHT:
            return find_moves<KNIGHT>(side, square, occupied);
        case BISHOP:
            return find_moves<BISHOP>(side, square, occupied);
        case ROOK:
            return find_moves<ROOK>(side, square, occupied);
        case QUEEN:
            return find_moves<QUEEN>(side, square, occupied);
        case KING:
            return find_moves<KING>(side, square, occupied);
        default:
            return 0;
    }
}

/**
 * Generates a bitboard of diagonal pawn captures from a pawn of team side, at the given square. Assumes the board is
 * fully occupied.
 *
 * @param side side which owns the pawn
 * @param square start square of the pawn
 * @return possible captures of the pawn
 */
inline U64 pawn_caps(Team side, uint8_t square) {
    return bb_normal_moves::pawn_caps[side][square];
}

/**
 * Generate a bitboard with a single bit set, corresponding to the given argument
 *
 * @param square bit to set
 * @return bitboard with the bit at {@code square} set
 */
inline U64 single_bit(uint8_t square) {
    return 0x1ull << square;
}

/**
 * Returns true if more than one bit is set on the given bitboard
 *
 * @param bb bitboard to check
 * @return true if more than one bit is set.
 */
inline bool multiple_bits(U64 bb) {
    return (bb & (bb - 1)) != 0;
}

/**
 * Generate a bitboard containing the squares between two squares, excluding the square themselves.
 *
 * If the arguments are not on the same rank or diagonal, an empty bitboard is returned.
 *
 * @param a first square
 * @param b second square
 * @return bitboard containing bits between the two squares
 */
inline U64 bits_between(uint8_t a, uint8_t b) {
    return bb_util::between[a][b];
}

/**
 * Generates a bitboard containing a line that crosses both squares.
 *
 * If the arguments are not on the same rank or diagonal, an empty bitboard is returned.
 *
 * @param a first square
 * @param b second square
 * @return bitboard containing a line which crosses the two squares
 */
inline U64 line(uint8_t a, uint8_t b) {
    return bb_util::line[a][b];
}

/**
 * Generates a bitboard containing a line that starts at the origin and continues to the edge of the board in the given
 * direction.
 *
 * If the arguments are not on the same rank or diagonal, an empty bitboard is returned.
 *
 * @param origin origin of line
 * @param direction direction of line
 * @return bitboard containing the ray
 */
inline U64 ray(uint8_t origin, uint8_t direction) {
    return bb_util::ray[origin][direction];
}

inline U64 file_mask(uint8_t file_index) {
    return bb_util::file[file_index];
}

/**
 * Determine whether squares a and b are the same colour
 *
 * @param a square a
 * @param b square b
 * @return true if the squares are of the same colour
 */
inline bool same_colour(uint8_t a, uint8_t b) {
    return ((uint8_t) (9 * (a ^ b)) & uint8_t(8)) == 0;
}

/**
 * Convert a file and a rank to a square index in the board
 *
 * @param file file
 * @param rank rank
 * @return square index
 */
inline uint8_t square_index(uint8_t file, uint8_t rank) {
    return bb_util::sq_index[file][rank];
}

// Opposite of the above

inline uint8_t rank_index(uint8_t sq_index) {
    return bb_util::rank_index[sq_index];
}

inline uint8_t file_index(uint8_t sq_index) {
    return bb_util::file_index[sq_index];
}

inline uint8_t file_edge_distance(uint8_t file) {
    return file < 4 ? file : 7 - file;
}

/**
 * Find the distance (in king moves) between two squares
 *
 * @param a first square
 * @param b second square
 * @return
 */
inline int distance(uint8_t a, uint8_t b) {
    return std::max(std::abs(rank_index(a) - rank_index(b)), std::abs(file_index(a) - file_index(b)));
}

/**
 * Determine whether squares a and b are on the same line
 * @param a square a
 * @param b square b
 * @return true if the squares are aligned
 */
inline bool aligned(uint8_t a, uint8_t b) {
    return distance(a, b) <= 1 || bits_between(a, b) != 0;
}

/**
 * Determine whether squares a, b and c are on the same line
 * @param a square a
 * @param b square b
 * @param c square c
 * @return true if the squares are aligned
 */
inline bool aligned(uint8_t a, uint8_t b, uint8_t c) {
    return aligned(a, b) && aligned(b, c) && aligned(a, c);
}


/**
 * Returns the index of the least significant bit in the given bitboard, and then removes it from the board.
 *
 * @param team type of popping: 0 (WHITE) for LSB, 1 (BLACK) for MSB
 * @param bb bitboard to pop from
 * @return index of LSB
 */
inline uint8_t pop_bit(U64 &bb) {
    const uint8_t s = bb_intrin::lsb(bb);
    bb &= bb - 1;
    return s;
}

/**
 * Returns the index of the least significant bit in the given bitboard without removing it from the board
 *
 * @param bb bitboard to search
 * @return index of LSB
 */
inline uint8_t bit_scan(U64 bb) {
    return bb_intrin::lsb(bb);
}

/**
 * Returns the number of '1' bits in the given bitboard.
 *
 * @param bb bits to count
 * @return number of active bits
 */
inline int pop_count(U64 bb) {
    return bb_intrin::pop_count(bb);
}

/**
 * Converts two characters representing a square (e.g. 'a' '5') would be translated into the index for the square a5.
 * Opposite of {@link from_sq}
 *
 * @param file file character (from 'a' to 'h')
 * @param rank rank character (from '1' to '8')
 * @return square index for the file and rank
 */
uint8_t to_sq(char file, char rank);

/**
 * Converts a square index (0-63) into a string representing that square (e.g. Square#F4) would be translated into "f4".
 * @return
 */
std::string from_sq(uint8_t);

#endif //TOPPLE_BB_H
