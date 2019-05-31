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
    int mat_mg[5] = {95, 365, 436, 583, 1324,};
    int mat_eg[5] = {101, 369, 389, 645, 1237,};
    int mat_exch_scale = -7;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {76, 64, 52,}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            62, 65, 70, 79,
            56, 76, 70, 69,
            31, 28, 60, 70,
            -59, 52, 19, 49,
    };
    int q_pst_mg[16] = {
            -24, -49, -38, -54,
            -15, -2, -19, -26,
            -30, -57, -2, -7,
            -23, -16, -14, 6,
    };
    int n_pst_eg[16] = {
            -21, -2, 13, 17,
            -37, -16, -7, 4,
            -47, -18, -19, -12,
            -65, -59, -26, -28,
    };
    int q_pst_eg[16] = {
            11, 43, 15, 42,
            -7, -30, -4, 7,
            -26, 3, -29, -16,
            -26, -19, -20, -28,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -80, -71, -141, -67,
            -63, -26, -46, -94,
            -47, -2, -7, -31,
            -13, -12, -16, -7,
            6, -29, -5, -5,
            -7, 19, 4, 2,
            -1, 9, 16, -12,
            -24, -12, -14, -27,
    };
    int r_pst_mg[32] = {
            9, -3, -50, 6,
            -2, 1, 10, 13,
            -39, -4, 10, -14,
            -29, -21, -9, -15,
            -43, -37, -18, -25,
            -41, -19, -18, -34,
            -37, -15, -32, -49,
            -7, -3, -16, 7,
    };
    int b_pst_eg[32] = {
            -15, -21, -9, -25,
            -11, -19, -15, -21,
            -10, -22, -18, -17,
            -16, -15, -19, -15,
            -26, -13, -17, -13,
            -13, -28, -19, -18,
            -21, -35, -34, -22,
            -26, -31, -33, -20,
    };
    int r_pst_eg[32] = {
            33, 30, 46, 29,
            21, 23, 21, 19,
            22, 17, 8, 14,
            19, 17, 17, 13,
            23, 21, 19, 15,
            17, 10, 9, 10,
            16, 7, 14, 24,
            14, 14, 18, 1,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            116, 187, 79, 75,
            45, 59, 37, 22,
            15, 10, -4, 14,
            13, -2, 6, 16,
            9, -4, 11, 5,
            9, 22, 15, 9,
    };
    int p_pst_eg[24] = {
            121, 90, 104, 105,
            49, 44, 46, 36,
            27, 26, 30, 16,
            18, 15, 12, 6,
            7, 11, 3, -1,
            10, 7, 8, -7,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -92, -32, -40, -70,
            -48, -19, -43, -55,
            25, 5, -48, -78,
            33, 53, -5, -18,
    };
    int k_pst_eg[16] = {
            3, 20, 37, 44,
            -1, 16, 32, 37,
            -29, -1, 25, 37,
            -74, -42, -11, -12,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-12, -30}; // [RANK][OPEN FILE]
    int isolated_eg[2] = {1, -8}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-2, -23}; // [OPEN FILE]
    int backwards_eg[2] = {-10, -13,}; // [OPEN FILE]
    int doubled_mg[2] = {-9, 13,}; // [OPEN FILE]
    int doubled_eg[2] = {-27, -32}; // [OPEN FILE]
    int blocked_mg = -18; // [OPEN_FILE]
    int blocked_eg = 3; // [OPEN_FILE]
    int chain_mg[5] = {9, 13, 21, 41, 247,}; // [RANK]
    int chain_eg[5] = {17, 6, 1, 16, -79,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-8, -3, -16, 8, 8, -7,}; // [RANK]
    int passed_eg[6] = {7, 9, 39, 65, 134, 160,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-22, 1, 18, 24,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 13;
    int kat_own_half_open_file = 10;
    int kat_other_half_open_file = 12;
    int kat_attack_weight[5] = {-2, 10, 4, 3, 7};
    int kat_defence_weight[5] = {6, 3, 3, 1, 0};

    int kat_table_scale = 18;
    int kat_table_translate = 38;
    int kat_table_max = 416;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 36;
    int pos_bishop_pair_eg = 64;

    int pos_r_trapped_mg = -69;
    int pos_r_behind_own_passer_eg = 16;
    int pos_r_behind_enemy_passer_eg = 3;
    int pos_r_xray_pawn_eg = 15;

    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 3;
    int pos_r_own_half_open_file_mg = 17;
    int pos_r_own_half_open_file_eg = 9;
    int pos_r_other_half_open_file_mg = 34;
    int pos_r_other_half_open_file_eg = 8;

    int pos_mob_mg[4] = {10, 11, 6, 6}; // [PIECE, ¬KING, ¬PAWN]
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
