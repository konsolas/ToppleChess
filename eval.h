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
    int mat_mg[5] = {81, 362, 418, 529, 1175,};
    int mat_eg[5] = {100, 367, 410, 688, 1310,};
    int mat_exch_scale = 3;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 3;
    int mat_exch_rook = 5;
    int mat_exch_queen = 13;
    int mat_opp_bishop[3] = {73, 56, 44}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {38, 60, 70, 74, 36, 68, 78, 72, 5, 13, 55, 48, -93, 24, -5, 16,
    };
    int q_pst_mg[16] = {-19, -31, -18, -25, -14, 10, -4, -10, -33, -34, 13, 0, -22, -31, -24, 4,
    };
    int n_pst_eg[16] = {-17, 6, 28, 33, -30, -8, 2, 18, -47, -17, -17, 0, -72, -67, -23, -23,
    };
    int q_pst_eg[16] = {-15, 32, 16, 50, -18, -31, 6, 16, -35, -15, -32, -8, -39, -14, -15, -33,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-83, -43, -85, -46, -54, -4, -18, -103, -29, 30, 19, -19, 3, -13, -18, -4, 9, -13, 2, -4, -12, 16, 6, 2, -11,
                        -2, 2, -23, -53, -31, -43, -56,
    };
    int r_pst_mg[32] = {74, 29, 9, 47, 45, 42, 57, 65, -8, 24, 40, 20, 0, -1, 9, 14, -30, -29, 4, -10, -35, -13, -3, -26, -40, -5, -14, -47, -11, -12, -3, 9,};
    int b_pst_eg[32] = {-18, -32, -26, -34, -14, -17, -17, -16, -10, -18, -13, -7, -6, 0, -3, 0, -11, -2, -5, 5, -3, -15, -9, -8, -11
            , -29, -25, -14, -24, -37, -43, -18,};
    int r_pst_eg[32] = { 34, 41, 49, 40, 27, 30, 29, 25, 20, 14, 8, 10, 15, 14, 18, 12, 23, 21, 16, 16, 17, 9, 8, 13, 18, 9, 12, 29, 16, 20, 20, 11,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {230, 283, 185, 190, 37, 56, 37, 21, 29, 25, 15, 28, 14, 3, 10, 20, -1, -5, 11, -2, -6, 21, 9, -6,};
    int p_pst_eg[24] = {131, 108, 119, 119, 57, 48, 52, 42, 26, 22, 26, 16, 18, 16, 13, 5, 12, 12, 4, 3, 16, 8, 12, 1,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {15, 3, 3, 0, 10, 8, 30, 17, 58, 55, 22, -44, -22, 18, 71, 101, 63, 91, 62, 26, 35, 55, 120, 121,};
    int k_pst_exposed_mg = 81;
    int k_pst_eg[16] = {
            20, 51, 67, 69, 38, 60, 72, 73, 12, 38, 60, 76, -33, 3, 28, 24,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-7, -35}, {-8, -33}, {-11, -23}, {-8, -21}, {6, -15}, {0, -61},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{2, -3}, {-1, -9}, {-3, -17}, {-13, -16}, {-28, 10}, {0, 10},}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-2, -20,}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -13,}; // [OPEN FILE]
    int doubled_mg[2] = {-4, 23,}; // [OPEN FILE]
    int doubled_eg[2] = {-23, -32,}; // [OPEN FILE]
    int blocked_mg[2] = {-15, -16,}; // [OPEN_FILE]
    int blocked_eg[2] = {-3, -1,}; // [OPEN_FILE]
    int chain_mg[5] = {11, 18, 23, 35, 94,}; // [RANK]
    int chain_eg[5] = {14, 3, 0, 25, -15,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-1, 0, -17, -2, 21, -99,}; // [RANK]
    int passed_eg[6] = {5, 11, 43, 70, 112, 140,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-38, -5, 18, 22,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 19;
    int kat_attack_weight[5] = {-3, 13, 8, 8, 10,};
    int kat_defence_weight[5] = {4, 3, 4, 0, -1,};

    int kat_table_scale = 13;
    int kat_table_translate = 32;
    int kat_table_max = 267;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 53;
    int pos_bishop_pair_eg = 51;

    int pos_r_trapped_mg = -90;
    int pos_r_open_file_mg = 17;
    int pos_r_open_file_eg = -23;
    int pos_r_behind_own_passer_mg = 4;
    int pos_r_behind_own_passer_eg = 8;
    int pos_r_behind_enemy_passer_mg = 5;
    int pos_r_behind_enemy_passer_eg = 1;
    int pos_r_xray_pawn_mg = 6;
    int pos_r_xray_pawn_eg = 7;

    // TODO: Square colour weaknesses, mobility, etc.
};

class evaluator_t {
    static constexpr size_t bucket_size = 4;
    struct pawn_entry_t {
        U64 hash;
        uint32_t hits;

        uint32_t eval_mg;
        uint32_t eval_eg;

        U64 passers[2];
        U64 defended[2];
        U64 open_files;
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

    /// Initialise generic evaluation tables
    static void eval_init();

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t *eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);
};

#endif //TOPPLE_EVAL_H
