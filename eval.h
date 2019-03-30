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
    int mat_mg[5] = {94, 387, 454, 582, 1293,};
    int mat_eg[5] = {101, 367, 399, 644, 1300,};
    int mat_exch_scale = 0;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 10;
    int mat_opp_bishop[3] = {75, 59, 46,}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {48, 70, 83, 85, 47, 82, 89, 86, 13, 24, 65, 59, -80, 32, 10, 30,
    };
    int q_pst_mg[16] = {-17, -33, -17, -26, -12, 13, -1, -9, -35, -40, 15, 0, -25, -28, -23, 4,
    };
    int n_pst_eg[16] = {-21, -2, 19, 23, -38, -14, -4, 9, -51, -21, -19, -5, -72, -63, -30, -27,
    };
    int q_pst_eg[16] = {-7, 44, 26, 57, -17, -32, 7, 24, -26, 3, -32, 1, -27, -9, -7, -24,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-77, -54, -120, -55, -49, -2, -19, -84, -27, 27, 28, -8, 9, -4, -4, 1, 20, -1, 11, 8, 2, 26, 18, 13, 0, 10, 13, -10, -39, -24, -34, -45,
    };
    int r_pst_mg[32] = {25, 15, -21, 25, 18, 24, 31, 45, -22, 13, 14, 6, -14, -8, 9, 1, -35, -27, -7, -15, -35, -12, -8, -28, -33, -8
            , -29, -50, -4, 1, -13, 10,};
    int b_pst_eg[32] = {-15, -23, -13, -25, -10, -12, -10, -18, -7, -14, -13, -9, -4, 0, -5, 0, -12, -5, -7, 3, -6, -16, -10, -10, -10, -27, -26, -15, -24, -32, -32, -16,};
    int r_pst_eg[32] = {35, 32, 43, 32, 19, 22, 21, 13, 19, 16, 11, 9, 16, 16, 12, 10, 20, 19, 18, 11, 14, 9, 9, 9, 16, 9, 16, 28, 19
            , 20, 23, 7,};
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {219, 288, 174, 168, 52, 73, 51, 37, 32, 24, 14, 29, 17, 3, 9, 22, 4, -5, 12, 1, -5, 17, 7, -7,};
    int p_pst_eg[24] = {115, 87, 103, 107, 53, 42, 47, 36, 28, 27, 29, 18, 19, 17, 13, 6, 9, 12, 3, 0, 16, 10, 12, -1,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {-13, -7, 5, 3, 2, 2, 22, -7, 55, 60, 20, -40, -16, 17, 72, 91, 51, 89, 59, 23, 41, 54, 116, 104,};
    int k_pst_exposed_mg = 45;
    int k_pst_eg[16] = {
            23, 56, 71, 73, 40, 60, 72, 74, 7, 36, 61, 74, -37, -5, 24, 23,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-9, -33}, {-14, -36}, {-10, -22}, {-6, -17}, {-4, 7}, {-201, -35},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{4, -3}, {3, -6}, {-3, -14}, {-14, -17}, {-18, 1}, {576, 0},}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-5, -26,}; // [OPEN FILE]
    int backwards_eg[2] = {-7, -12,}; // [OPEN FILE]
    int doubled_mg[2] = {-8, 11,}; // [OPEN FILE]
    int doubled_eg[2] = {-27, -31,}; // [OPEN FILE]
    int blocked_mg = -17; // [OPEN_FILE]
    int blocked_eg = 2; // [OPEN_FILE]
    int chain_mg[5] = {9, 15, 24, 46, 228,}; // [RANK]
    int chain_eg[5] = {16, 6, -1, 19, -54,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-4, -1, -16, 3, 6, -81,}; // [RANK]
    int passed_eg[6] = {6, 10, 40, 67, 119, 155,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-28, -2, 19, 22,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 16;
    int kat_own_half_open_file = 13;
    int kat_other_half_open_file = 14;
    int kat_attack_weight[5] = {-3, 10, 6, 4, 7,};
    int kat_defence_weight[5] = {5, 3, 4, 1, 0,};

    int kat_table_scale = 16;
    int kat_table_translate = 37;
    int kat_table_max = 366;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 43;
    int pos_bishop_pair_eg = 61;

    int pos_r_trapped_mg = -77;
    int pos_r_behind_own_passer_mg = 5;
    int pos_r_behind_own_passer_eg = 14;
    int pos_r_behind_enemy_passer_mg = -5;
    int pos_r_behind_enemy_passer_eg = 3;
    int pos_r_xray_pawn_mg = -7;
    int pos_r_xray_pawn_eg = 14;
    int pos_r_open_file_mg = 78;
    int pos_r_open_file_eg = 20;
    int pos_r_own_half_open_file_mg = 12;
    int pos_r_own_half_open_file_eg = 17;
    int pos_r_other_half_open_file_mg = 44;
    int pos_r_other_half_open_file_eg = 16;

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
    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);
};

#endif //TOPPLE_EVAL_H
