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
    int mat_mg[5] = {95, 365, 441, 583, 1324};
    int mat_eg[5] = {102, 369, 391, 644, 1241};
    int mat_exch_scale = -5;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {73, 59, 44}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            53, 60, 70, 73,
            46, 73, 81, 78,
            25, 26, 64, 77,
            -66, 44, 21, 52,
    };
    int q_pst_mg[16] = {
            -34, -55, -39, -57,
            -27, -4, -16, -21,
            -40, -59, 2, 3,
            -28, -16, -11, 6,
    };
    int n_pst_eg[16] = {
            -20, -3, 14, 18,
            -33, -17, -7, 3,
            -47, -19, -23, -15,
            -67, -61, -26, -28,
    };
    int q_pst_eg[16] = {
            12, 54, 25, 61,
            -2, -20, 8, 18,
            -17, 15, -24, -17,
            -22, -20, -17, -25,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -80, -71, -137, -75,
            -62, -24, -45, -88,
            -54, -7, -8, -38,
            -14, -16, -20, -5,
            10, -28, -5, 1,
            -7, 23, 10, 3,
            7, 15, 21, -5,
            -26, -14, -18, -29,
    };
    int r_pst_mg[32] = {
            6, -19, -48, -2,
            -10, -7, 0, 5,
            -46, -19, 2, -14,
            -32, -25, -11, -21,
            -42, -38, -16, -28,
            -37, -20, -15, -31,
            -33, -13, -31, -46,
            -4, 4, -12, 8,
    };
    int b_pst_eg[32] = {
            -16, -23, -14, -23,
            -13, -18, -14, -23,
            -11, -21, -21, -14,
            -16, -16, -18, -15,
            -26, -11, -18, -12,
            -11, -26, -17, -20,
            -26, -36, -35, -24,
            -27, -30, -32, -20,
    };
    int r_pst_eg[32] = {
            27, 28, 39, 27,
            15, 20, 19, 16,
            23, 18, 9, 12,
            19, 17, 17, 15,
            23, 23, 20, 15,
            17, 16, 11, 10,
            16, 9, 16, 25,
            13, 11, 17, 1,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            112, 176, 70, 56,
            54, 62, 43, 33,
            18, 12, 1, 17,
            15, 0, 7, 18,
            8, -3, 12, 5,
            7, 17, 11, 11,
    };
    int p_pst_eg[24] = {
            122, 94, 107, 112,
            46, 38, 42, 29,
            23, 22, 25, 12,
            16, 15, 12, 5,
            7, 11, 3, -1,
            11, 9, 9, -6,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -96, -31, -51, -84,
            -50, -18, -42, -62,
            24, 6, -49, -78,
            34, 55, 2, -21,
    };
    int k_pst_eg[16] = {
            4, 20, 40, 46,
            0, 19, 34, 38,
            -30, -2, 25, 36,
            -75, -44, -13, -12,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-11, -24}; // [OPEN FILE]
    int isolated_eg[2] = {2, -6}; // [OPEN FILE]
    int backwards_mg[2] = {-1, -20}; // [OPEN FILE]
    int backwards_eg[2] = {-8, -10}; // [OPEN FILE]
    int semi_backwards_mg[2] = {-1, -15}; // [OPEN FILE]
    int semi_backwards_eg[2] = {3, 1}; // [OPEN FILE]
    int paired_mg[2] = {6, -2};
    int paired_eg[2] = {2, 7};
    int doubled_mg[2] = {-13, 20}; // [OPEN FILE]
    int doubled_eg[2] = {-18, -25}; // [OPEN FILE]
    int chain_mg[5] = {13, 16, 29, 43, 237}; // [RANK - 2]
    int chain_eg[5] = {13, 4, 3, 21, -72}; // [RANK - 2]
    int passed_mg[6] = {-12, -4, -15, 14, 12, 9}; // [RANK - 1]
    int passed_eg[6] = {7, 9, 39, 67, 136, 160}; // [RANK - 1]
    int candidate_mg[5] = {-24, -10, 16, 46, 85}; // [RANK - 1]
    int candidate_eg[5] = {1, 1, 1, 36, 38}; // [RANK - 1]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg = -12;
    int blocked_eg = -16;
    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 2;
    int pos_r_own_half_open_file_mg = 33; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = 1;
    int pos_r_other_half_open_file_mg = 18;
    int pos_r_other_half_open_file_eg = 13;

    /// King safety
    int ks_pawn_shield[4] = {-23, 1, 18, 21}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 16;
    int kat_open_file = 27;
    int kat_own_half_open_file = 24;
    int kat_other_half_open_file = 11;
    int kat_attack_weight[5] = {-4, 20, 8, 6, 15};
    int kat_defence_weight[5] = {12, 6, 6, 2, 1};

    int kat_table_scale = 39;
    int kat_table_translate = 78;
    int kat_table_max = 419;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 36;
    int pos_bishop_pair_eg = 64;

    int pos_r_trapped_mg = -77;
    int pos_r_behind_own_passer_eg = 13;
    int pos_r_behind_enemy_passer_eg = 45;
    int pos_r_xray_pawn_eg = 16;

    int pos_mob_mg[4] = {9, 9, 5, 4}; // [PIECE, ¬KING, ¬PAWN]
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
