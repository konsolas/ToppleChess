//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

enum GamePhase {
    MG, EG
};

struct eval_params_t {
    /// Material
    int mat_mg[5] = {95, 365, 440, 583, 1324};
    int mat_eg[5] = {102, 369, 391, 644, 1239};
    int mat_exch_scale = -5;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {74, 59, 51}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            53, 60, 70, 71,
            47, 75, 81, 78,
            24, 27, 64, 78,
            -72, 43, 21, 50,
    };
    int q_pst_mg[16] = {
            -35, -59, -41, -61,
            -27, -3, -18, -22,
            -41, -59, 0, 2,
            -28, -17, -10, 9,
    };
    int n_pst_eg[16] = {
            -20, -3, 14, 19,
            -34, -18, -7, 3,
            -45, -19, -21, -14,
            -65, -60, -25, -28,
    };
    int q_pst_eg[16] = {
            11, 53, 22, 61,
            -3, -28, 8, 16,
            -17, 6, -25, -14,
            -25, -21, -21, -35,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -91, -67, -137, -74,
            -64, -26, -43, -91,
            -56, -7, -7, -39,
            -15, -16, -20, -12,
            9, -27, -5, -3,
            -5, 22, 11, 4,
            8, 14, 20, -4,
            -26, -14, -18, -29,
    };
    int r_pst_mg[32] = {
            -3, -15, -55, -3,
            -9, -9, -2, 3,
            -46, -17, 5, -17,
            -32, -22, -10, -19,
            -42, -37, -16, -25,
            -38, -18, -17, -31,
            -33, -13, -31, -46,
            -5, 4, -13, 8,
    };
    int b_pst_eg[32] = {
            -14, -22, -11, -21,
            -11, -19, -14, -22,
            -9, -21, -20, -15,
            -15, -15, -17, -13,
            -26, -12, -17, -10,
            -12, -25, -17, -20,
            -29, -34, -35, -24,
            -26, -30, -33, -20,
    };
    int r_pst_eg[32] = {
            30, 30, 44, 28,
            18, 22, 20, 17,
            24, 19, 8, 13,
            19, 17, 17, 14,
            24, 21, 21, 15,
            18, 14, 11, 11,
            17, 10, 17, 25,
            14, 12, 18, 1,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            118, 190, 68, 74,
            53, 59, 46, 28,
            17, 10, -2, 16,
            15, -2, 6, 17,
            8, -4, 11, 5,
            10, 21, 14, 12,
    };
    int p_pst_eg[24] = {
            123, 93, 110, 108,
            48, 41, 41, 33,
            25, 23, 26, 14,
            17, 15, 11, 5,
            8, 11, 3, -1,
            13, 9, 9, -6,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -98, -31, -51, -84,
            -49, -17, -42, -62,
            23, 5, -49, -78,
            35, 55, 3, -21,
    };
    int k_pst_eg[16] = {
            4, 20, 40, 46,
            0, 19, 34, 38,
            -30, -1, 25, 36,
            -75, -43, -14, -10,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-12, -27}; // [OPEN FILE]
    int isolated_eg[2] = {2, -7}; // [OPEN FILE]
    int backwards_mg[2] = {-3, -24}; // [OPEN FILE]
    int backwards_eg[2] = {-8, -11}; // [OPEN FILE]
    int doubled_mg[2] = {-12, 20}; // [OPEN FILE]
    int doubled_eg[2] = {-17, -28}; // [OPEN FILE]
    int chain_mg[5] = {9, 15, 28, 44, 238}; // [RANK - 2]
    int chain_eg[5] = {13, 4, 2, 18, -76}; // [RANK - 2]
    int passed_mg[6] = {-9, -2, -14, 15, 12, 5}; // [RANK - 1]
    int passed_eg[6] = {8, 10, 40, 67, 136, 161}; // [RANK - 1]
    int candidate_mg[5] = {-23, -9, 17, 45, -36}; // [RANK - 1]
    int candidate_eg[5] = {4, 9, 11, 44, -10}; // [RANK - 1]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg = -13;
    int blocked_eg = -14;
    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 2;
    int pos_r_own_half_open_file_mg = 33; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = 3;
    int pos_r_other_half_open_file_mg = 18;
    int pos_r_other_half_open_file_eg = 13;

    /// King safety
    int ks_pawn_shield[4] = {-23, 0, 18, 23}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 14;
    int kat_own_half_open_file = 12;
    int kat_other_half_open_file = 5;
    int kat_attack_weight[5] = {-2, 10, 4, 3, 7};
    int kat_defence_weight[5] = {6, 3, 3, 1, 0};

    int kat_table_scale = 19;
    int kat_table_translate = 39;
    int kat_table_max = 418;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 36;
    int pos_bishop_pair_eg = 66;

    int pos_r_trapped_mg = -77;
    int pos_r_behind_own_passer_eg = 15;
    int pos_r_behind_enemy_passer_eg = 41;
    int pos_r_xray_pawn_eg = 16;

    int pos_mob_mg[4] = {9, 9, 5, 5}; // [PIECE, ¬KING, ¬PAWN]
    int pos_mob_eg[4] = {1, 3, 3, 4};

    // TODO: Square colour weaknesses, mobility, etc.
};

struct processed_params_t : public eval_params_t {
    explicit processed_params_t(const eval_params_t &params);

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    int kat_table[128] = {};
};

class alignas(64) evaluator_t {
    size_t pawn_hash_entries;
    std::vector<pawns::structure_t> pawn_hash_table;

    const processed_params_t &params;
public:
    evaluator_t(const processed_params_t &params, size_t pawn_hash_size);

    int evaluate(const board_t &board);
    void prefetch(U64 pawn_hash);

    /// Initialise generic evaluation tables
    static void eval_init();

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    void eval_pawns(const board_t &board, int &mg, int &eg);
    void eval_pst(const board_t &board, int &mg, int &eg);
    void eval_movement(const board_t &board, int &mg, int &eg);
    void eval_positional(const board_t &board, int &mg, int &eg);
};

#endif //TOPPLE_EVAL_H
