//
// Created by Vincent on 22/09/2017.
//

#ifndef TOPPLE_BB_H
#define TOPPLE_BB_H

#include "types.h"

#include <string>
#include <cassert>
#include <array>

#ifdef __BMI2__
#include <x86intrin.h> // Use hardware PEXT/PDEP if available
#endif

/**
 * Generate a bitboard with a single bit set, corresponding to the given argument
 *
 * @param square bit to set
 * @return bitboard with the bit at {@code square} set
 */
constexpr U64 single_bit(uint8_t square) {
    return 0x1ull << square;
}

/*
 * Varions methods for shifting bitboards, without wrapping bits around edges
 */
namespace bb_shifts {
    constexpr U64 not_A = 0xfefefefefefefefe; // ~0x0101010101010101 : everything except the A-file
    constexpr U64 not_H = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080 : everything except the H-file

    /**
     * Shifts a given bitboard by 1 square in the given direction
     *
     * @tparam D direction to shift in
     * @param bb bitboard to shift
     * @return shifted bitboard
     */
    template<Direction D>
    constexpr U64 shift(U64 bb);

    template<>
    constexpr U64 shift<D_N>(U64 b) { return b << 8u; }

    template<>
    constexpr U64 shift<D_S>(U64 b) { return b >> 8u; }

    template<>
    constexpr U64 shift<D_E>(U64 b) { return (b << 1u) & not_A; }

    template<>
    constexpr U64 shift<D_NE>(U64 b) { return (b << 9u) & not_A; }

    template<>
    constexpr U64 shift<D_SE>(U64 b) { return (b >> 7u) & not_A; }

    template<>
    constexpr U64 shift<D_W>(U64 b) { return (b >> 1u) & not_H; }

    template<>
    constexpr U64 shift<D_SW>(U64 b) { return (b >> 9u) & not_H; }

    template<>
    constexpr U64 shift<D_NW>(U64 b) { return (b << 7u) & not_H; }

    /**
     * Fill a bitboard in the forward direction of a given team.
     * Computes the union of bb with a forward shift of it until no further shifts are possible.
     *
     * @tparam team direction to use
     * @param bb bitboard to fill
     * @return
     */
    template<Team team> constexpr U64 fill_forward(U64 bb);

    template<> constexpr U64 fill_forward<WHITE>(U64 bb) {
        bb |= (bb << 8u);
        bb |= (bb << 16u);
        bb |= (bb << 32u);
        return bb;
    }

    template<> constexpr U64 fill_forward<BLACK>(U64 bb) {
        bb |= (bb >> 8u);
        bb |= (bb >> 16u);
        bb |= (bb >> 32u);
        return bb;
    }

    /**
     * Fill a bitboard in a given direction, but only in a given open region.
     * A '0' in the open region will not be crossed by the fill operation:
     * {@code ~open} is therefore treated as occluding space for the operation
     *
     * @tparam D direction to use
     * @param bb bitboard to fill
     * @param open open space for filling
     * @return filled bitboard
     */
    template<Direction D>
    constexpr U64 fill_occluded(U64 bb, U64 open);

    template<>
    constexpr U64 fill_occluded<D_N>(U64 bb, U64 open) {
        bb |= open & (bb << 8u);
        open &= (open << 8u);
        bb |= open & (bb << 16u);
        open &= (open << 16u);
        bb |= open & (bb << 32u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_S>(U64 bb, U64 open) {
        bb |= open & (bb >> 8u);
        open &= (open >> 8u);
        bb |= open & (bb >> 16u);
        open &= (open >> 16u);
        bb |= open & (bb >> 32u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_E>(U64 bb, U64 open) {
        open &= not_A;
        bb |= open & (bb << 1u);
        open &= (open << 1u);
        bb |= open & (bb << 2u);
        open &= (open << 2u);
        bb |= open & (bb << 4u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_NE>(U64 bb, U64 open) {
        open &= not_A;
        bb |= open & (bb << 9u);
        open &= (open << 9u);
        bb |= open & (bb << 18u);
        open &= (open << 18u);
        bb |= open & (bb << 36u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_SE>(U64 bb, U64 open) {
        open &= not_A;
        bb |= open & (bb >> 7u);
        open &= (open >> 7u);
        bb |= open & (bb >> 14u);
        open &= (open >> 14u);
        bb |= open & (bb >> 28u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_W>(U64 bb, U64 open) {
        open &= not_H;
        bb |= open & (bb >> 1u);
        open &= (open >> 1u);
        bb |= open & (bb >> 2u);
        open &= (open >> 2u);
        bb |= open & (bb >> 4u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_SW>(U64 bb, U64 open) {
        open &= not_H;
        bb |= open & (bb >> 9u);
        open &= (open >> 9u);
        bb |= open & (bb >> 18u);
        open &= (open >> 18u);
        bb |= open & (bb >> 36u);
        return bb;
    }

    template<>
    constexpr U64 fill_occluded<D_NW>(U64 bb, U64 open) {
        open &= not_H;
        bb |= open & (bb << 7u);
        open &= (open << 7u);
        bb |= open & (bb << 14u);
        open &= (open << 14u);
        bb |= open & (bb << 28u);
        return bb;
    }
}

/*
 * CPU operations with no suitable equivalent in the c++17 standard library
 */
namespace bb_intrin {
    /**
     * Returns the index of the least significant bit in a bitboard
     *
     * @param b bitboard to check (nonzero)
     * @return index of the least significant bit
     */
    inline uint8_t lsb(U64 b) {
        assert(b);
        return Square(__builtin_ctzll(b)); // 'count trailing zeroes long long'
    }
    /**
     * Returns the index of the most significant bit in a bitboard
     *
     * @param b bitboard to check (nonzero)
     * @return index of the least significant bit
     */
    inline uint8_t msb(U64 b) { // unused
        assert(b);
        return Square(63 - __builtin_clzll(b)); // 'count leading zeroes long long'
    }

    /**
     * Returns the number of set bits in the given bitboard
     *
     * @param b bitboard to check
     * @return number of set bits
     */
    inline int pop_count(U64 b) {
        return __builtin_popcountll(b); // std::bitset<64>(b).count() might also be optimised to this
    }

    /**
     * Implements the parallel bits extract operation.
     *
     * Removes all bits in source that do not correspond to a set bit in mask,
     * shifting other bits to fill the gap.
     * Leaves those that do consecutively in the lower bits of the result.
     *
     * e.g. pext(01101010b, 01000110b) = 00000101b
     *
     * @param source
     * @param mask
     * @return result
     */
    inline U64 pext(U64 source, U64 mask) { // unused
#ifdef __BMI2__
        return _pext_u64(source, mask);
#else
        // Software emulation of pext
        U64 res = 0;
        for(U64 bb = 1; mask != 0; bb += bb) {
            if(source & mask & -mask) {
                res |= bb;
            }
            mask &= mask - 1;
        }
        return res;
#endif
    }

    /**
     * Implements the parallel bits deposit operation.
     *
     * Takes the lower order bits of source and sequentially assigns them to
     * bits in the result that correspond to set bits in mask, leaving zeroes in between.
     *
     * e.g. pdep(00000101b, 01000110b) = 01000010b
     *
     * @param source
     * @param mask
     * @return result
     */
    inline U64 pdep(U64 source, U64 mask) {
#ifdef __BMI2__
        return _pdep_u64(source, mask);
#else
        // Software emulation of pdep
        U64 res = 0;
        for (U64 bb = 1; mask; bb += bb) {
            if (source & bb) {
                res |= mask & -mask;
            }
            mask &= mask - 1;
        }
        return res;
#endif
    }
}

/*
 * Lookup functions for sliding move generation.
 * Uses fixed-shift magic bitboard lookups - see bb.cpp for computation of lookup table
 */
namespace bb_sliders {
    struct sq_entry_t {
        U64 mask;
        U64 magic;
        U64 *base;
    };

    extern sq_entry_t b_table[64];
    extern sq_entry_t r_table[64];

    /**
     * Compute the bitboard of squares that a bishop on sq can move to, if
     * occupancy is the set of squares occupied by pieces. Includes captures.
     *
     * @param sq bishop square
     * @param occupancy occupied squares on the board
     * @return set of possible bishop moves
     */
    inline U64 bishop_moves(uint8_t sq, U64 occupancy) {
        sq_entry_t entry = b_table[sq];
        return entry.base[((occupancy & entry.mask) * entry.magic) >> (64u - 9u)];
    }

    /**
     * Compute the bitboard of squares that a rook on sq can move to, if
     * occupancy is the set of squares occupied by pieces. Includes captures.
     *
     * @param sq bishop square
     * @param occupancy occupied squares on the board
     * @return set of possible rook moves
     */
    inline U64 rook_moves(uint8_t sq, U64 occupancy) {
        sq_entry_t entry = r_table[sq];
        return entry.base[((occupancy & entry.mask) * entry.magic) >> (64u - 12u)];
    }
}

/*
 * Potentially useful bitboard lookup tables
 */
namespace bb_util {
    extern U64 between[64][64]; // between[a][b] is the bitboard between a and b, or 0 if they are unaligned
    extern U64 line[64][64]; // line[a][b] is the bitboard of the line through a and b, or 0 if they are unaligned
    extern U64 ray[64][64]; // ray[a][b] is the bitboard of the ray from a through b, or 0 if they are unaligned
    extern U64 file[8]; // file[f] is the bitboard of file number f
}

/*
 * Non-sliding move generation lookup tables
 */
namespace bb_normal_moves {
    extern U64 king_moves[64];
    extern U64 knight_moves[64];
    extern U64 pawn_moves_x1[2][64]; // Normal pawn advances
    extern U64 pawn_moves_x2[2][64]; // Double pawn advances on the 2nd or 7th rank
    extern U64 pawn_caps[2][64]; // Pawn captures
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
 * @tparam TYPE piece type
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
    return bb_sliders::bishop_moves(square, occupied);
}

template<>
inline U64 find_moves<ROOK>(Team side, uint8_t square, U64 occupied) {
    return bb_sliders::rook_moves(square, occupied);
}

template<>
inline U64 find_moves<QUEEN>(Team side, uint8_t square, U64 occupied) {
    return bb_sliders::bishop_moves(square, occupied) | bb_sliders::rook_moves(square, occupied);
}

template<>
inline U64 find_moves<KING>(Team side, uint8_t square, U64 occupied) {
    return bb_normal_moves::king_moves[square];
}

/**
 * A variant of find_moves where the piece is not a template parameter. This can be used in loops if necessary, but
 * should be avoided if possible.
 *
 * @param type piece type
 * @param side side which owns the piece
 * @param square square start square of piece
 * @param occupied bitboard of occupied squares
 * @return possible moves of the piece
 */
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

/**
 * Generates a bitboard representing all the squares in the file at the given file index
 *
 * @param file_index index of file
 * @return bitboard of the file
 */

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
constexpr bool same_colour(uint8_t a, uint8_t b) {
    return ((uint8_t) (9 * (a ^ b)) & uint8_t(8)) == 0;
}

/**
 * Convert a file and a rank to a square index in the board
 *
 * @param file file
 * @param rank rank
 * @return square index
 */
constexpr uint8_t square_index(uint8_t file, uint8_t rank) {
    return (rank << 3u) + file;
}

/**
 * Extract a square's file index
 *
 * @param sq_index square
 * @return file index
 */
constexpr uint8_t rank_index(uint8_t sq_index) {
    return sq_index >> 3u;
}

/**
 * Extract a square's rank index
 *
 * @param sq_index square
 * @return rank index
 */
constexpr uint8_t file_index(uint8_t sq_index) {
    return sq_index & 7u;
}

/**
 * Get the distance of a file to the edge of the board
 *
 * @param file file index
 * @return distance to the edge of the board
 */
constexpr uint8_t file_edge_distance(uint8_t file) {
    return file < 4 ? file : 7 - file;
}

/**
 * Find the distance (in king moves) between two squares.
 * This is the Chebyshev distance metric.
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
