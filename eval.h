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
    int mat_mg[5] = {86, 363, 425, 544, 1185,};
    int mat_eg[5] = {101, 372, 413, 680, 1335,};
    int mat_exch_scale = 6;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 10;
    int mat_opp_bishop[3] = {77, 60, 48,}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {41, 64, 73, 78, 39, 73, 81, 79, 9, 14, 58, 52, -90, 25, -3, 21,
    };
    int q_pst_mg[16] = {-18, -32, -21, -28, -9, 12, 0, -9, -35, -35, 15, 0, -23, -29, -24, 5,
    };
    int n_pst_eg[16] = {-13, 10, 33, 36, -26, -5, 7, 20, -44, -11, -10, 4, -70, -60, -19, -20,
    };
    int q_pst_eg[16] = {-8, 42, 32, 66, -24, -34, 6, 26, -32, -6, -32, -2, -36, -13, -9, -32,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-86, -52, -123, -60, -54, -4, -22, -92, -34, 22, 20, -18, 2, -13, -13, -4, 9, -10, 1, -2, -8, 16, 9, 3, -11,
                        -1, 3, -19, -50, -33, -41, -56,
    };
    int r_pst_mg[32] = {45, 19, -8, 47, 15, 17, 27, 44, -27, 12, 22, 12, -14, -6, 8, 7, -32, -32, 2, -10, -37, -14, -5, -28, -38, -6,
                        -19, -47, -9, -7, -4, 12,};
    int b_pst_eg[32] = {-14, -24, -14, -27, -11, -14, -9, -17, -4, -10, -9, -6, -1, 4, -2, 3, -8, -2, -5, 7, -1, -11, -6, -9, -11, -23, -23, -14, -23, -32, -38, -14,};
    int r_pst_eg[32] = {32, 33, 42, 27, 25, 28, 26, 19, 24, 17, 11, 10, 19, 17, 17, 13, 23, 24, 17, 15, 19, 13, 10, 13, 18, 10, 16, 29, 19, 21, 22, 8,};
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {248, 298, 191, 203, 46, 61, 44, 25, 33, 26, 15, 31, 16, 4, 11, 22, 1, -6, 11, -2, -5, 18, 9, -7,};
    int p_pst_eg[24] = {137, 115, 125, 126, 59, 50, 54, 43, 28, 26, 29, 18, 19, 17, 14, 7, 13, 13, 4, 3, 17, 11, 13, 1,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {-9, 16, 11, 7, 13, 13, 30, 1, 59, 59, 30, -37, -11, 26, 76, 96, 51, 93, 65, 27, 40, 61, 122, 110,};
    int k_pst_exposed_mg = 61;
    int k_pst_eg[16] = {
            22, 57, 73, 76, 39, 62, 75, 77, 7, 38, 63, 78, -41, -3, 24, 25,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-6, -35}, {-9, -32}, {-11, -22}, {-10, -18}, {1, 1}, {0, -43},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{0, -2}, {0, -9}, {-4, -15}, {-14, -17}, {-27, 6}, {0, 5},}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-2, -23,}; // [OPEN FILE]
    int backwards_eg[2] = {-9, -14,}; // [OPEN FILE]
    int doubled_mg[2] = {-4, 17,}; // [OPEN FILE]
    int doubled_eg[2] = {-21, -30,}; // [OPEN FILE]
    int blocked_mg[2] = {-16, -18,}; // [OPEN_FILE]
    int blocked_eg[2] = {-3, -1,}; // [OPEN_FILE]
    int chain_mg[5] = {11, 17, 22, 39, 186,}; // [RANK]
    int chain_eg[5] = {16, 6, 2, 23, -41,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {0, -1, -15, 0, 16, -101,}; // [RANK]
    int passed_eg[6] = {7, 15, 46, 74, 119, 144,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-38, -6, 18, 23,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 9;
    int kat_attack_weight[5] = {-2, 12, 8, 7, 9,};
    int kat_defence_weight[5] = {5, 3, 5, 1, -1,};

    int kat_table_scale = 14;
    int kat_table_translate = 39;
    int kat_table_max = 275;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 46;
    int pos_bishop_pair_eg = 66;

    int pos_r_trapped_mg = -90;
    int pos_r_open_file_mg = 33;
    int pos_r_open_file_eg = 10;
    int pos_r_behind_own_passer_mg = 4;
    int pos_r_behind_own_passer_eg = 1;
    int pos_r_behind_enemy_passer_mg = 4;
    int pos_r_behind_enemy_passer_eg = 0;
    int pos_r_xray_pawn_mg = 8;
    int pos_r_xray_pawn_eg = 9;

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
    evaluator_t(evaluator_t&&) = default;
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
