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

// qlc: 0.0582706 lqa: 0.0647049
struct eval_params_t {
    /// Material
    int mat_exch_knight = 26;
    int mat_exch_bishop = 33;
    int mat_exch_rook = 48;
    int mat_exch_queen = 155;

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            404, 421, 429, 432, 387, 413, 422, 427, 387, 385, 396, 415, 275, 372, 393, 387,
    };
    int n_pst_eg[16] = {
            375, 378, 383, 388, 372, 381, 386, 390, 362, 380, 383, 381, 312, 364, 365, 379,
    };
    int q_pst_mg[16] = {
            1318, 1321, 1316, 1310, 1333, 1329, 1324, 1315, 1326, 1315, 1327, 1331, 1327, 1318, 1314, 1328,
    };
    int q_pst_eg[16] = {
            1255, 1271, 1266, 1298, 1219, 1253, 1275, 1274, 1228, 1246, 1243, 1249, 1201, 1223, 1239, 1227,
    };
    int b_pst_mg[16] = {
            412, 406, 420, 426, 427, 438, 435, 430, 425, 438, 435, 429, 408, 432, 412, 413,
    };
    int b_pst_eg[16] = {
            382, 388, 383, 377, 378, 385, 387, 389, 364, 378, 375, 386, 376, 376, 383, 390,
    };
    int r_pst_mg[16] = {
            572, 578, 570, 576, 579, 591, 577, 580, 573, 577, 576, 583, 580, 585, 590, 591,
    };
    int r_pst_eg[16] = {
            679, 673, 678, 673, 672, 671, 677, 671, 684, 689, 688, 686, 688, 680, 682, 676,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            205, 158, 149, 121, 132, 145, 133, 135, 110, 108, 99, 99, 106, 100, 100, 111, 94, 83, 96, 88, 95, 94, 101, 86
    };
    int p_pst_eg[24] = {
            184, 204, 206, 183, 122, 138, 133, 111, 109, 113, 114, 104, 103, 105, 104, 95, 99, 100, 94, 92, 98, 98, 92, 86,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -55, -80, -51, -73, 26, -10, -23, -46, 29, -15, -58, -80, 54, 37, 26, 0,
    };
    int k_pst_eg[16] = {
            8, 30, 24, 24, 6, 20, 17, 18, -13, 4, 17, 21, -42, -19, -2, 3,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-4, -17}; // [OPEN FILE]
    int isolated_eg[2] = {-2, 0}; // [OPEN FILE]
    int backwards_mg[2] = {1, -17}; // [OPEN FILE]
    int backwards_eg[2] = {-4, -14}; // [OPEN FILE]
    int semi_backwards_mg[2] = {5, -17}; // [OPEN FILE]
    int semi_backwards_eg[2] = {3, -2}; // [OPEN FILE]
    int paired_mg[2] = {10, 0}; // [OPEN FILE]
    int paired_eg[2] = {3, 4}; // [OPEN FILE]
    int detached_mg[2] = {-5, -3}; // [OPEN FILE]
    int detached_eg[2] = {-6, -7}; // [OPEN FILE]
    int doubled_mg[2] = {-16, 5}; // [OPEN FILE]
    int doubled_eg[2] = {-21, -32}; // [OPEN FILE]
    int chain_mg[5] = {16, 8, 15, 30, 209}; // [RANK - 2]
    int chain_eg[5] = {17, 12, 16, 33, -3}; // [RANK - 2]
    int passed_mg[6] = {-23, -31, -20, 10, 14, 12}; // [RANK - 1]
    int passed_eg[6] = {-40, -17, 21, 59, 116, 124}; // [RANK - 1]
    int candidate_mg[4] = {-14, -6, 10, 56}; // [RANK - 1]
    int candidate_eg[4] = {-11, 1, 15, 48}; // [RANK - 1]
    int king_tropism_eg[2] = {-1, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-8, 15}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg[2] = {-7, -5}; // [OWN, OTHER]
    int blocked_eg[2] = {-5, -33}; // [OWN, OTHER]
    int pos_r_open_file_mg = 41;
    int pos_r_open_file_eg = 13;
    int pos_r_own_half_open_file_mg = 24; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = -5;
    int pos_r_other_half_open_file_mg = 16;
    int pos_r_other_half_open_file_eg = 4;

    // [KNIGHT, BISHOP]
    int outpost_mg[2] = {5, -2};
    int outpost_eg[2] = {7, 9};
    int outpost_hole_mg[2] = {33, 37};
    int outpost_hole_eg[2] = {17, -5};
    int outpost_half_mg[2] = {-3, 4};
    int outpost_half_eg[2] = {-6, 1};

    /// King safety
    int ks_pawn_shield[4] = {-18, -3, 14, 34}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 8;
    int kat_open_file = 24;
    int kat_own_half_open_file = 17;
    int kat_other_half_open_file = 14;
    int kat_attack_weight[5] = {8, 15, 11, 9, 11};
    int kat_defence_weight[5] = {8, 4, 4, 2, 1};

    int kat_table_scale = 23;
    int kat_table_translate = 77;
    int kat_table_max = 430;
    int kat_table_offset = 10;

    /// Threats
    int undefended_mg[5] = {-9, -17, -13, -3, -12}; // [¬KING]
    int undefended_eg[5] = {6, -1, -5, -7, -6}; // [¬KING]
    int threat_matrix_mg[4][5] = {
            {20, 50, 46, 49, 40},
            {-9, 24, 24, 61, 35},
            {1, 28, -11, 19, 26},
            {-3, 12, 27, 42, 68},
    };
    int threat_matrix_eg[4][5] = {
            {-2, -10, 32, -43, -57},
            {6, 159, 24, -11, -45},
            {13, 26, -37, -1, 107},
            {17, 17, 14, 3, -1},
    };

    /// Other positional
    int pos_bishop_pair_mg = 25;
    int pos_bishop_pair_eg = 67;
    int mat_opp_bishop[3] = {53, 64, 69}; // [PAWN ADVANTAGE]

    int pos_r_trapped_mg = -68;
    int pos_r_behind_own_passer_eg = -4;
    int pos_r_behind_enemy_passer_eg = 59;

    int pos_mob_mg[4] = {7, 7, 3, 3}; // [PIECE, ¬KING, ¬PAWN]
    int pos_mob_eg[4] = {5, 6, 4, 6};

    // TODO: Evaluate better
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

    double game_phase(const board_t &board) const; // returns tapering factor 0-1
private:
    struct eval_data_t {
        int king_pos[2];
        U64 king_circle[2];
        int king_danger[2];

        U64 team_attacks[2];
        U64 attacks[2][6];
        U64 double_attacks[2];

        void update_attacks(Team team, Piece piece, U64 bb) {
            double_attacks[team] = team_attacks[team] & bb;
            attacks[team][piece] |= bb;
            team_attacks[team] |= bb;
        }
    };

    void eval_pawns(const board_t &board, int &mg, int &eg, eval_data_t &data);
    void eval_pieces(const board_t &board, int &mg, int &eg, eval_data_t &data);
    void eval_threats(const board_t &board, int &mg, int &eg, eval_data_t &data);
    void eval_positional(const board_t &board, int &mg, int &eg, eval_data_t &data);
};

#endif //TOPPLE_EVAL_H
