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
    int mat_mg[5] = {92, 367, 443, 583, 1342};
    int mat_eg[5] = {88, 370, 394, 646, 1231};
    int mat_exch_scale = -4;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {74, 61, 46}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            54, 64, 73, 75,
            46, 74, 80, 80,
            24, 32, 67, 77,
            -69, 44, 24, 52,
    };
    int q_pst_mg[16] = {
            -33, -54, -40, -54,
            -28, -7, -16, -21,
            -40, -58, 3, 4,
            -26, -14, -12, 6,
    };
    int n_pst_eg[16] = {
            -18, -5, 13, 18,
            -33, -18, -5, 3,
            -46, -22, -23, -14,
            -65, -58, -26, -28,
    };
    int q_pst_eg[16] = {
            15, 50, 29, 59,
            3, -15, 5, 16,
            -13, 10, -27, -20,
            -22, -21, -18, -27,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -87, -64, -128, -66,
            -63, -23, -43, -91,
            -50, -2, -7, -34,
            -9, -15, -19, -5,
            9, -26, -4, 2,
            -6, 22, 10, 2,
            7, 15, 23, -5,
            -28, -14, -18, -32,
    };
    int r_pst_mg[32] = {
            6, -8, -41, 7,
            -12, -6, -1, 4,
            -42, -16, 0, -15,
            -36, -22, -9, -22,
            -43, -39, -19, -31,
            -36, -15, -13, -28,
            -32, -14, -28, -45,
            -3, 4, -11, 8,
    };
    int b_pst_eg[32] = {
            -15, -24, -16, -25,
            -13, -18, -13, -22,
            -12, -23, -21, -16,
            -17, -16, -17, -15,
            -25, -13, -19, -11,
            -12, -23, -17, -18,
            -24, -36, -37, -24,
            -24, -28, -30, -19,
    };
    int r_pst_eg[32] = {
            26, 25, 37, 22,
            14, 18, 17, 14,
            18, 16, 9, 10,
            20, 15, 15, 15,
            23, 23, 21, 18,
            18, 12, 11, 10,
            17, 11, 17, 26,
            13, 13, 18, 1,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            109, 172, 66, 66,
            58, 68, 52, 41,
            23, 19, 8, 25,
            17, 2, 11, 21,
            6, -5, 10, 4,
            4, 15, 9, 11,
    };
    int p_pst_eg[24] = {
            113, 91, 113, 111,
            38, 35, 38, 28,
            20, 18, 23, 11,
            12, 13, 9, 2,
            5, 9, 1, -5,
            5, 2, 1, -15,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -115, -74, -87, -120,
            -64, -37, -64, -94,
            26, 0, -59, -85,
            40, 58, 3, -21,
    };
    int k_pst_eg[16] = {
            10, 16, 23, 24,
            7, 15, 21, 20,
            -18, 1, 21, 27,
            -49, -28, -2, -3,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-9, -20}; // [OPEN FILE]
    int isolated_eg[2] = {4, -6}; // [OPEN FILE]
    int backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -11}; // [OPEN FILE]
    int semi_backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int semi_backwards_eg[2] = {4, 1}; // [OPEN FILE]
    int paired_mg[2] = {9, 2}; // [OPEN FILE]
    int paired_eg[2] = {1, 5}; // [OPEN FILE]
    int detached_mg[2] = {-1, 0}; // [OPEN FILE]
    int detached_eg[2] = {-6, -5}; // [OPEN FILE]
    int doubled_mg[2] = {-17, 17}; // [OPEN FILE]
    int doubled_eg[2] = {-17, -23}; // [OPEN FILE]
    int chain_mg[5] = {18, 17, 28, 43, 249}; // [RANK - 2]
    int chain_eg[5] = {9, 2, 3, 21, -66}; // [RANK - 2]
    int passed_mg[6] = {-16, -15, -24, 3, 3, 12}; // [RANK - 1]
    int passed_eg[6] = {-29, -15, 22, 57, 134, 152}; // [RANK - 1]
    int candidate_mg[4] = {-22, -12, 9, 32}; // [RANK - 1]
    int candidate_eg[4] = {-2, 0, 5, 46}; // [RANK - 1]
    int king_tropism_eg[2] = {0, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-7, 12}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg = -10;
    int blocked_eg = -17;
    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 3;
    int pos_r_own_half_open_file_mg = 34; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = 0;
    int pos_r_other_half_open_file_mg = 19;
    int pos_r_other_half_open_file_eg = 13;

    /// King safety
    int ks_pawn_shield[4] = {-27, 0, 18, 21}; // 0, 1, 2, and 3 pawns close to the king

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
    int pos_bishop_pair_eg = 62;

    int pos_r_trapped_mg = -75;
    int pos_r_behind_own_passer_eg = 15;
    int pos_r_behind_enemy_passer_eg = 49;
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
