//
// Created by Vincent on 27/09/2017.
//

#ifndef TOPPLE_TESTLIB_H
#define TOPPLE_TESTLIB_H

#include <array>
#include "../types.h"
#include "../bb.h"
#include "../move.h"

/**
 * Output operator for U64 bitboards
 *
 * @param Str stream
 * @param v bitboard
 * @return same stream with formatted bitboard
 */
std::string u64_to_string(U64 v);

/**
 * Create a U64 bitboard with an array of bits (corresponding to squares)
 * @param array bits
 * @return bitboard constant
 */
U64 c_u64(std::array<int, 64> array);


#endif //TOPPLE_TESTLIB_H
