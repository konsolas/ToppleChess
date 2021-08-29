//
// Created by Vincent on 29/08/2021.
//
#include "../bb.h"

extern "C" uint64_t topple_knight_attacks(unsigned int square) {
    return find_moves<KNIGHT>(WHITE, square, 0);
}

extern "C" uint64_t topple_king_attacks(unsigned int square) {
    return find_moves<KING>(WHITE, square, 0);
}

extern "C" uint64_t topple_rook_attacks(unsigned int square, uint64_t occ) {
    return find_moves<ROOK>(WHITE, square, occ);
}

extern "C" uint64_t topple_bishop_attacks(unsigned int square, uint64_t occ) {
    return find_moves<BISHOP>(WHITE, square, occ);
}

extern "C" uint64_t topple_queen_attacks(unsigned int square, uint64_t occ) {
    return find_moves<QUEEN>(WHITE, square, occ);
}

extern "C" uint64_t topple_pawn_attacks(unsigned int square, int color) {
    return pawn_caps(color ? WHITE : BLACK, square);
}

extern "C" unsigned topple_pop_count(uint64_t bb) {
    return pop_count(bb);
}

extern "C" unsigned topple_lsb(uint64_t bb) {
    return bit_scan(bb);
}
