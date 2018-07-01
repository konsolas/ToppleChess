//
// Created by Vincent on 22/09/2017.
//

#ifndef TOPPLE_BB_H
#define TOPPLE_BB_H

#include "types.h"

#include <string>

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
U64 find_moves(Team side, uint8_t square, U64 occupied);
U64 find_moves(Piece type, Team side, uint8_t square, U64 occupied);

/**
 * Generates a bitboard of diagonal pawn captures from a pawn of team side, at the given square. Assumes the board is
 * fully occupied.
 *
 * @param side side which owns the pawn
 * @param square start square of the pawn
 * @return possible captures of the pawn
 */
U64 pawn_caps(Team side, uint8_t square);

/**
 * Generate a bitboard with a single bit set, corresponding to the given argument
 *
 * @param square bit to set
 * @return bitboard with the bit at {@code square} set
 */
U64 single_bit(uint8_t square);

/**
 * Generate a bitboard containing the squares between two squares, excluding the square themselves.
 *
 * If the arguments are not on the same rank or diagonal, an empty bitboard is returned.
 *
 * @param a first square
 * @param b second square
 * @return bitboard containing bits between the two squares
 */
U64 bits_between(uint8_t a, uint8_t b);

/**
 * Generates a bitboard containing a line that crosses both squares.
 *
 * If the arguments are not on the same rank or diagonal, an empty bitboard is returned.
 *
 * @param a first square
 * @param b second square
 * @return bitboard containing a line which crosses the two squares
 */
U64 line(uint8_t a, uint8_t b);

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
U64 ray(uint8_t origin, uint8_t direction);

/**
 * Determine whether squares a and b are the same colour
 *
 * @param a square a
 * @param b square b
 * @return true if the squares are of the same colour
 */
bool same_colour(uint8_t a, uint8_t b);

/**
 * Determine whether squares a and b are on the same line
 * @param a square a
 * @param b square b
 * @return true if the squares are aligned
 */
bool aligned(uint8_t a, uint8_t b);

/**
 * Determine whether squares a, b and c are on the same line
 * @param a square a
 * @param b square b
 * @param c square c
 * @return true if the squares are aligned
 */
bool aligned(uint8_t a, uint8_t b, uint8_t c);

/**
 * Find the distance (in king moves) between two squares
 *
 * @param a first square
 * @param b second square
 * @return
 */
int distance(uint8_t a, uint8_t b);

/**
 * Convert a file and a rank to a square index in the board
 *
 * @param file file
 * @param rank rank
 * @return square index
 */
uint8_t square_index(uint8_t file, uint8_t rank);

// Opposite of the above
uint8_t rank_index(uint8_t sq_index);
uint8_t file_index(uint8_t sq_index);

/**
 * Returns the index of the least significant bit in the given bitboard, and then removes it from the board.
 *
 * @param team type of popping: 0 (WHITE) for LSB, 1 (BLACK) for MSB
 * @param bb bitboard to pop from
 * @return index of LSB
 */
uint8_t pop_bit(U64 &bb);

/**
 * Returns the index of the least significant bit in the given bitboard without removing it from the board
 *
 * @param bb bitboard to search
 * @return index of LSB
 */
uint8_t bit_scan(U64 bb);

/**
 * Returns the number of '1' bits in the given bitboard.
 *
 * @param bb bits to count
 * @return number of active bits
 */
int pop_count(U64 bb);

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
