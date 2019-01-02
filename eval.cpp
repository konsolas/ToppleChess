//
// Created by Vincent on 30/09/2017.
//

#include "board.h"
#include "endgame.h"
#include <sstream>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation datatypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int material[6] = {100, 300, 300, 500, 900, 0};
int control[6] = {9, 5, 3, 3, 1, 0};
int hole = 2;
int passed = 5;
int occupied_critical = 5;

// Reading eval tables
template<typename T>
T read(T *table, int sq, Team side) {
    if (side) {
        return table[sq];
    } else {
        return table[MIRROR_TABLE[sq]];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utility tables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

U64 BB_PASSED[2][64] = {{0}};
U64 BB_ISOLATED[2][64] = {{0}};
U64 BB_IN_FRONT[2][64] = {{0}};
U64 BB_HOLE[2][64] = {{0}};

U64 BB_KING_CIRCLE[64] = {0};

U64 BB_CENTRE = 0;

void eval_init() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        BB_HOLE[WHITE][sq] = 0xffffffffffffffff;

        for (uint8_t rank = rank_index(sq) + uint8_t(1); rank < 8; rank++) {
            // Passed
            if (file_index(sq) > 0) BB_PASSED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) - 1), rank));
            if (file_index(sq) < 7) BB_PASSED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) + 1), rank));
            BB_PASSED[WHITE][sq] |= single_bit(square_index(file_index(sq), rank));

            // Candidate
            BB_IN_FRONT[WHITE][sq] |= single_bit(square_index(file_index(sq), rank));
        }

        // Hole
        BB_HOLE[WHITE][sq] ^= BB_PASSED[WHITE][sq];
        BB_HOLE[WHITE][sq] ^= BB_IN_FRONT[WHITE][sq];

        // Isolated
        for (uint8_t rank = 0; rank < 8; rank++) {
            if (file_index(sq) > 0)
                BB_ISOLATED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) - 1), rank));
            if (file_index(sq) < 7)
                BB_ISOLATED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) + 1), rank));
        }

        // Centre
        if (file_index(sq) <= 4 && file_index(sq) >= 3 && rank_index(sq) <= 4 && rank_index(sq) >= 3) {
            BB_CENTRE |= single_bit(sq);
        }

        // King circle
        BB_KING_CIRCLE[sq] |= find_moves<KING>(WHITE, sq, 0);
        BB_KING_CIRCLE[sq] |= find_moves<KNIGHT>(WHITE, sq, 0);
    }

    // Mirror
    for (uint8_t i = 0; i < 64; i++) {
        // Tables
        // ... none yet

        for (uint8_t square = 0; square < 64; square++) {
            if (BB_PASSED[WHITE][i] & single_bit(square))
                BB_PASSED[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_ISOLATED[WHITE][i] & single_bit(square))
                BB_ISOLATED[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_IN_FRONT[WHITE][i] & single_bit(square))
                BB_IN_FRONT[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_HOLE[WHITE][i] & single_bit(square))
                BB_HOLE[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int eval(const board_t &board) {
    // Check endgame
    eg_eval_t eg_eval = eval_eg(board);
    if (eg_eval.valid) {
        return board.record[board.now].next_move ? -eg_eval.eval : +eg_eval.eval;
    }

    U64 critical = BB_CENTRE
                   | BB_KING_CIRCLE[bit_scan(board.bb_pieces[WHITE][KING])]
                   | BB_KING_CIRCLE[bit_scan(board.bb_pieces[BLACK][KING])];

    // Evaluate material
    int pst[64];
    for(uint8_t i = 0; i < 64; i++) {
        sq_data_t data = board.sq_data[i];
        pst[i] = data.occupied * material[data.piece];

        if(data.team) {
            pst[i] = -pst[i];
        }
    }

    // Evaluate control
    for(uint8_t i = 0; i < 64; i++) {
        pst[i] += control[PAWN] * pop_count(pawn_caps(WHITE, i) & board.bb_pieces[WHITE][PAWN]);
        pst[i] += control[KNIGHT] * pop_count(find_moves<KNIGHT>(BLACK, i, board.bb_all) & board.bb_pieces[WHITE][KNIGHT]);
        pst[i] += control[BISHOP] * pop_count(find_moves<BISHOP>(BLACK, i, board.bb_all) & board.bb_pieces[WHITE][BISHOP]);
        pst[i] += control[ROOK] * pop_count(find_moves<ROOK>(BLACK, i, board.bb_all) & board.bb_pieces[WHITE][ROOK]);
        pst[i] += control[QUEEN] * pop_count(find_moves<QUEEN>(BLACK, i, board.bb_all) & board.bb_pieces[WHITE][QUEEN]);

        pst[i] -= control[PAWN] * pop_count(pawn_caps(WHITE, i) & board.bb_pieces[BLACK][PAWN]);
        pst[i] -= control[KNIGHT] * pop_count(find_moves<KNIGHT>(WHITE, i, board.bb_all) & board.bb_pieces[BLACK][KNIGHT]);
        pst[i] -= control[BISHOP] * pop_count(find_moves<BISHOP>(WHITE, i, board.bb_all) & board.bb_pieces[BLACK][BISHOP]);
        pst[i] -= control[ROOK] * pop_count(find_moves<ROOK>(WHITE, i, board.bb_all) & board.bb_pieces[BLACK][ROOK]);
        pst[i] -= control[QUEEN] * pop_count(find_moves<QUEEN>(WHITE, i, board.bb_all) & board.bb_pieces[BLACK][QUEEN]);
    }

    // Evaluate holes and pawn structure
    for (uint8_t i = 8; i < 56; i++) {
        if(!(BB_HOLE[WHITE][i] & board.bb_pieces[BLACK][PAWN])) {
            pst[i] += hole;

            if(single_bit(uint8_t(i + rel_offset(WHITE, D_N))) & board.bb_pieces[BLACK][PAWN]) {
                critical = critical | single_bit(i);
            }
        }

        if(!(BB_HOLE[BLACK][i] & board.bb_pieces[WHITE][PAWN])) {
            if(single_bit(uint8_t(i + rel_offset(BLACK, D_N))) & board.bb_pieces[WHITE][PAWN]) {
                critical = critical | single_bit(i);
            }
        }
    }

    // Final evaluation
    int total = 0;

    for(uint8_t i = 0; i < 64; i++) {
        bool square_occupied = board.sq_data[i].occupied;
        bool square_critical = (single_bit(i) & critical) != 0;
        if(square_occupied || square_critical) {
            total += pst[i];
        }

        if(square_occupied && square_critical) {
            total += ((pst[i] > 0) - (pst[i] < 0)) * occupied_critical;
        }
    }

    return board.record[board.now].next_move ? -total : total;
}
