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

// 0.0577006
struct eval_params_t {
    /// Material
    int mat_exch_knight = 29;
    int mat_exch_bishop = 31;
    int mat_exch_rook = 59;
    int mat_exch_queen = 135;
    int mat_opp_bishop[3] = {75, 60, 46}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            423, 430, 435, 439,
            412, 442, 447, 446,
            394, 391, 431, 443,
            305, 409, 390, 418,
    };
    int q_pst_mg[16] = {
            1302, 1282, 1296, 1281,
            1309, 1327, 1319, 1314,
            1294, 1275, 1335, 1338,
            1307, 1318, 1323, 1339,
    };
    int n_pst_eg[16] = {
            344, 362, 370, 376,
            335, 350, 363, 372,
            320, 351, 349, 357,
            299, 316, 345, 344,
    };
    int q_pst_eg[16] = {
            1242, 1278, 1256, 1289,
            1227, 1214, 1236, 1246,
            1216, 1242, 1203, 1210,
            1208, 1208, 1213, 1204,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            357, 380, 314, 379,
            382, 420, 399, 355,
            397, 440, 441, 411,
            432, 432, 428, 436,
            455, 416, 440, 442,
            439, 467, 456, 447,
            451, 459, 468, 440,
            416, 430, 426, 417,
    };
    int r_pst_mg[32] = {
            589, 575, 543, 588,
            567, 577, 584, 586,
            542, 565, 583, 565,
            548, 561, 572, 561,
            540, 540, 565, 556,
            547, 565, 568, 557,
            553, 571, 554, 538,
            580, 587, 573, 592,
    };
    int b_pst_eg[32] = {
            380, 371, 381, 369,
            384, 378, 383, 373,
            378, 366, 367, 376,
            373, 373, 370, 375,
            364, 377, 373, 376,
            382, 368, 376, 376,
            370, 361, 359, 369,
            370, 366, 364, 373,
    };
    int r_pst_eg[32] = {
            671, 670, 682, 669,
            660, 663, 662, 659,
            661, 658, 652, 656,
            663, 658, 660, 659,
            666, 667, 664, 658,
            662, 659, 657, 653,
            662, 655, 663, 669,
            658, 658, 663, 647,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            198, 280, 152, 161,
            150, 161, 146, 135,
            116, 112, 100, 117,
            109, 94, 103, 113,
            97, 86, 102, 96,
            96, 107, 101, 103,
    };
    int p_pst_eg[24] = {
            202, 175, 204, 200,
            124, 122, 126, 116,
            107, 107, 111, 100,
            99, 101, 97, 90,
            92, 97, 89, 83,
            91, 90, 89, 72,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -125, -67, -77, -118,
            -66, -38, -66, -94,
            25, 0, -60, -86,
            40, 58, 0, -22,
    };
    int k_pst_eg[16] = {
            7, 11, 17, 21,
            7, 13, 17, 19,
            -17, 2, 21, 26,
            -49, -28, -1, -2,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-9, -20}; // [OPEN FILE]
    int isolated_eg[2] = {3, -6}; // [OPEN FILE]
    int backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -9}; // [OPEN FILE]
    int semi_backwards_mg[2] = {0, -17}; // [OPEN FILE]
    int semi_backwards_eg[2] = {4, 3}; // [OPEN FILE]
    int paired_mg[2] = {10, 2}; // [OPEN FILE]
    int paired_eg[2] = {1, 5}; // [OPEN FILE]
    int detached_mg[2] = {-1, 0}; // [OPEN FILE]
    int detached_eg[2] = {-6, -5}; // [OPEN FILE]
    int doubled_mg[2] = {-16, 16}; // [OPEN FILE]
    int doubled_eg[2] = {-20, -26}; // [OPEN FILE]
    int chain_mg[5] = {19, 18, 29, 33, 216}; // [RANK - 2]
    int chain_eg[5] = {9, 2, 4, 27, -50}; // [RANK - 2]
    int passed_mg[6] = {-23, -18, -26, 2, 3, 13}; // [RANK - 1]
    int passed_eg[6] = {-33, -21, 17, 54, 134, 152}; // [RANK - 1]
    int candidate_mg[4] = {-21, -14, 7, 35}; // [RANK - 1]
    int candidate_eg[4] = {-2, -2, 5, 45}; // [RANK - 1]
    int king_tropism_eg[2] = {0, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-7, 14}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg[2] = {-13, 1}; // [OWN, OTHER]
    int blocked_eg[2] = {-4, -38}; // [OWN, OTHER]
    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 4;
    int pos_r_own_half_open_file_mg = 34; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = 0;
    int pos_r_other_half_open_file_mg = 19;
    int pos_r_other_half_open_file_eg = 12;

    // [KNIGHT, BISHOP]
    int outpost_mg[2] = {4, 9};
    int outpost_eg[2] = {27, 15};
    int outpost_hole_mg[2] = {36, 50};
    int outpost_hole_eg[2] = {-5, -6};
    int outpost_half_mg[2] = {3, 6};
    int outpost_half_eg[2] = {-8, -6};

    /// King safety
    int ks_pawn_shield[4] = {-27, 0, 17, 21}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 4;
    int kat_open_file = 25;
    int kat_own_half_open_file = 23;
    int kat_other_half_open_file = 8;
    int kat_attack_weight[5] = {0, 19, 6, 6, 14};
    int kat_defence_weight[5] = {11, 6, 6, 2, 2};

    int kat_table_scale = 38;
    int kat_table_translate = 79;
    int kat_table_max = 421;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 36;
    int pos_bishop_pair_eg = 62;

    int pos_r_trapped_mg = -76;
    int pos_r_behind_own_passer_eg = 15;
    int pos_r_behind_enemy_passer_eg = 54;
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
