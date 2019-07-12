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

// 0.0577892
struct eval_params_t {
    /// Material
    int mat_mg[5] = {92, 367, 443, 583, 1341};
    int mat_eg[5] = {88, 370, 394, 646, 1233};
    int mat_exch_scale = -3;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {73, 62, 44}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            55, 63, 73, 76,
            46, 75, 80, 80,
            27, 28, 66, 77,
            -70, 43, 21, 52,
    };
    int q_pst_mg[16] = {
            -33, -54, -40, -54,
            -27, -7, -16, -21,
            -41, -58, 2, 4,
            -27, -16, -11, 5,
    };
    int n_pst_eg[16] = {
            -23, -6, 12, 16,
            -33, -19, -5, 3,
            -48, -18, -21, -13,
            -65, -58, -24, -26,
    };
    int q_pst_eg[16] = {
            15, 49, 28, 58,
            2, -15, 6, 16,
            -14, 10, -27, -21,
            -22, -20, -17, -27,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -87, -64, -132, -62,
            -62, -23, -44, -91,
            -48, -1, -9, -33,
            -9, -13, -17, -4,
            9, -25, -4, 1,
            -7, 21, 9, 3,
            7, 15, 23, -4,
            -27, -14, -18, -32,
    };
    int r_pst_mg[32] = {
            7, -8, -39, 7,
            -13, -7, 0, 4,
            -41, -16, 1, -16,
            -38, -22, -11, -22,
            -44, -41, -19, -29,
            -36, -16, -13, -28,
            -32, -13, -26, -44,
            -3, 4, -11, 8,
    };
    int b_pst_eg[32] = {
            -14, -23, -13, -24,
            -12, -17, -13, -21,
            -12, -26, -21, -15,
            -19, -19, -21, -16,
            -27, -15, -20, -12,
            -12, -24, -18, -19,
            -24, -36, -36, -25,
            -24, -28, -30, -18,
    };
    int r_pst_eg[32] = {
            25, 26, 36, 23,
            14, 18, 17, 14,
            17, 14, 7, 10,
            19, 13, 14, 13,
            21, 22, 19, 14,
            17, 13, 11, 10,
            17, 11, 17, 26,
            13, 13, 19, 2,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            107, 184, 67, 66,
            57, 69, 53, 42,
            24, 20, 8, 25,
            17, 2, 11, 21,
            6, -6, 10, 4,
            4, 15, 9, 11,
    };
    int p_pst_eg[24] = {
            113, 87, 115, 109,
            37, 35, 38, 28,
            20, 18, 23, 11,
            11, 13, 9, 2,
            5, 9, 1, -5,
            5, 2, 1, -15,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -126, -76, -87, -118,
            -64, -38, -65, -93,
            25, 0, -60, -86,
            40, 58, 1, -21,
    };
    int k_pst_eg[16] = {
            9, 13, 21, 22,
            7, 14, 18, 20,
            -17, 2, 21, 27,
            -49, -28, -1, -1,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-9, -20}; // [OPEN FILE]
    int isolated_eg[2] = {4, -6}; // [OPEN FILE]
    int backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -10}; // [OPEN FILE]
    int semi_backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int semi_backwards_eg[2] = {4, 3}; // [OPEN FILE]
    int paired_mg[2] = {9, 2}; // [OPEN FILE]
    int paired_eg[2] = {1, 5}; // [OPEN FILE]
    int detached_mg[2] = {-1, 0}; // [OPEN FILE]
    int detached_eg[2] = {-6, -5}; // [OPEN FILE]
    int doubled_mg[2] = {-17, 16}; // [OPEN FILE]
    int doubled_eg[2] = {-19, -25}; // [OPEN FILE]
    int chain_mg[5] = {19, 18, 28, 40, 230}; // [RANK - 2]
    int chain_eg[5] = {9, 2, 3, 22, -56}; // [RANK - 2]
    int passed_mg[6] = {-25, -15, -26, 2, 3, 13}; // [RANK - 1]
    int passed_eg[6] = {-31, -18, 20, 56, 134, 152}; // [RANK - 1]
    int candidate_mg[4] = {-21, -13, 9, 31}; // [RANK - 1]
    int candidate_eg[4] = {-3, -1, 5, 44}; // [RANK - 1]
    int king_tropism_eg[2] = {0, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-7, 13}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg[2] = {-12, 1}; // [OWN, OTHER]
    int blocked_eg[2] = {-6, -38}; // [OWN, OTHER]
    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 3;
    int pos_r_own_half_open_file_mg = 34; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = 0;
    int pos_r_other_half_open_file_mg = 19;
    int pos_r_other_half_open_file_eg = 12;

    /// King safety
    int ks_pawn_shield[4] = {-27, 0, 17, 21}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 4;
    int kat_open_file = 26;
    int kat_own_half_open_file = 23;
    int kat_other_half_open_file = 8;
    int kat_attack_weight[5] = {-1, 19, 7, 6, 14};
    int kat_defence_weight[5] = {11, 6, 6, 2, 2};

    int kat_table_scale = 38;
    int kat_table_translate = 79;
    int kat_table_max = 421;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 37;
    int pos_bishop_pair_eg = 61;

    int pos_r_trapped_mg = -75;
    int pos_r_behind_own_passer_eg = 15;
    int pos_r_behind_enemy_passer_eg = 52;
    int pos_r_xray_pawn_eg = 18;

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
