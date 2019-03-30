//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include "types.h"
#include "board.h"

enum GamePhase {
    MG, EG
};

struct eval_params_t {
    /// Material
    int mat_mg[5] = {97, 375, 445, 592, 1328,};
    int mat_eg[5] = {99, 367, 390, 646, 1255,};
    int mat_exch_scale = -9;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {75, 60, 46,}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {60, 67, 73, 81, 55, 76, 73, 73, 33, 27, 61, 70, -60, 50, 18, 47,
    };
    int q_pst_mg[16] = {-18, -44, -35, -48, -10, 4, -12, -22, -27, -50, 4, -2, -15, -10, -10, 10,
    };
    int n_pst_eg[16] = {-17, 0, 17, 20, -34, -12, -4, 7, -48, -14, -16, -9, -65, -55, -21, -26,
    };
    int q_pst_eg[16] = {4, 38, 14, 36, -11, -34, -12, 4, -24, -3, -32, -17, -33, -25, -20, -33,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-83, -69, -139, -65, -61, -26, -45, -87, -43, 0, -2, -26, -11, -9, -14, -3, 9, -24, -2, -4, -5, 20, 8, 5, 2,
                        10, 18, -10, -28, -11, -14, -29,
    };
    int r_pst_mg[32] = {18, 4, -44, 15, -6, 10, 12, 23, -36, 2, 13, -4, -20, -15, 5, -10, -39, -33, -9, -23, -34, -16, -12, -30, -34,
                        -12, -26, -47, -5, 0, -12, 11,};
    int b_pst_eg[32] = {-14, -22, -9, -24, -11, -15, -12, -20, -10, -20, -17, -17, -14, -13, -16, -13, -25, -13, -15, -11, -12, -25,
                        -18, -18, -19, -34, -32, -20, -22, -29, -33, -17,};
    int r_pst_eg[32] = {32, 30, 46, 28, 23, 21, 23, 17, 23, 17, 10, 12, 17, 17, 12, 14, 24, 21, 18, 15, 14, 9, 7, 10, 17, 9, 14, 26,
                        15, 16, 19, 3,};
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {198, 264, 155, 147, 46, 59, 40, 22, 17, 12, -2, 17, 13, -2, 5, 16, 7, -4, 10, 3, 6, 21, 14, 6,};
    int p_pst_eg[24] = {112, 90, 103, 103, 55, 47, 49, 39, 30, 28, 31, 20, 19, 17, 14, 7, 8, 11, 3, 0, 13, 8, 10, -5,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {-13, -7, -1, -2, 3, 0, 18, -2, 53, 48, 18, -33, -9, 12, 68, 93, 39, 89, 57, 17, 44, 53, 118, 99,};
    int k_pst_exposed_mg = 44;
    int k_pst_eg[16] = {
            25, 56, 71, 72, 39, 60, 72, 74, 7, 37, 61, 73, -37, -7, 25, 26,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-11, -31}, {-18, -35}, {-8, -25}, {0, -16}, {9, 11}, {-291, -34},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{4, -3}, {4, -6}, {-3, -14}, {-15, -17}, {-25, -2}, {551, 0},}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-2, -24,}; // [OPEN FILE]
    int backwards_eg[2] = {-7, -12,}; // [OPEN FILE]
    int doubled_mg[2] = {-10, 10,}; // [OPEN FILE]
    int doubled_eg[2] = {-26, -31,}; // [OPEN FILE]
    int blocked_mg = -17; // [OPEN_FILE]
    int blocked_eg = 2; // [OPEN_FILE]
    int chain_mg[5] = {10, 15, 21, 50, 249,}; // [RANK]
    int chain_eg[5] = {16, 5, 0, 15, -64,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-10, -5, -16, 6, -6, -77,}; // [RANK]
    int passed_eg[6] = {7, 11, 41, 67, 123, 157,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-28, -4, 15, 21,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 13;
    int kat_own_half_open_file = 10;
    int kat_other_half_open_file = 12;
    int kat_attack_weight[5] = {-2, 10, 4, 3, 7,};
    int kat_defence_weight[5] = {6, 3, 3, 1, 0,};

    int kat_table_scale = 18;
    int kat_table_translate = 38;
    int kat_table_max = 416;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 37;
    int pos_bishop_pair_eg = 64;

    int pos_r_trapped_mg = -67;
    int pos_r_behind_own_passer_eg = 13;
    int pos_r_behind_enemy_passer_eg = 4;
    int pos_r_xray_pawn_eg = 14;

    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 10;
    int pos_r_own_half_open_file_mg = 16;
    int pos_r_own_half_open_file_eg = 16;
    int pos_r_other_half_open_file_mg = 34;
    int pos_r_other_half_open_file_eg = 14;

    int pos_in_hole_mg[4][6] = { // [PIECE, ¬KING, ¬PAWN][RANK]
            {-74, 111, -8, -105, -51, -131, },
            {51, -18, 27, -7, -58, -33, },
            {-12, 8, 4, 45, -68, -78, },
            {0, 1, -12, -5, -12, 16, },
    };

    int pos_mob_mg[4] = {9, 10, 5, 6}; // [PIECE, ¬KING, ¬PAWN]
    int pos_mob_eg[4] = {1, 3, 3, 5};

    // TODO: Square colour weaknesses, mobility, etc.
};

class evaluator_t {
    struct pawn_entry_t {
        U64 hash;

        int32_t eval_mg;
        int32_t eval_eg;

        U64 passers[2];
        U64 defended[2];
        U64 half_open_files[2]; // [side with the pawn]
        U64 open_files;
        U64 advance_squares[2];
        U64 attackable[2];
    };

    size_t pawn_hash_entries;
    pawn_entry_t *pawn_hash_table;

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    int kat_table[96] = {};

    eval_params_t params;
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_size);
    evaluator_t(const evaluator_t&) = delete;
    evaluator_t& operator=(const evaluator_t &) = delete;
    ~evaluator_t();

    int evaluate(const board_t &board);
    void prefetch(U64 pawn_hash);

    /// Initialise generic evaluation tables
    static void eval_init();

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    pawn_entry_t eval_pawns(const board_t &board);

    void eval_pst(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);

    void eval_movement(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);
};

#endif //TOPPLE_EVAL_H
