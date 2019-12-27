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
    int mat_exch_bishop = 34;
    int mat_exch_rook = 48;
    int mat_exch_queen = 156;
    int mat_opp_bishop[3] = {53, 63, 71}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            412, 420, 431, 431,
            392, 423, 427, 437,
            397, 392, 411, 423,
            278, 383, 407, 400,
    };
    int q_pst_mg[16] = {
            1308, 1309, 1307, 1304,
            1328, 1328, 1325, 1314,
            1317, 1306, 1328, 1333,
            1322, 1320, 1320, 1331,
    };
    int n_pst_eg[16] = {
            366, 374, 377, 381,
            362, 370, 375, 381,
            353, 368, 371, 370,
            299, 348, 352, 368,
    };
    int q_pst_eg[16] = {
            1254, 1282, 1280, 1300,
            1216, 1249, 1267, 1270,
            1221, 1250, 1237, 1235,
            1194, 1220, 1230, 1226,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            352, 362, 364, 337,
            381, 392, 391, 382,
            417, 436, 427, 441,
            433, 428, 425, 425,
            439, 421, 430, 441,
            438, 449, 448, 444,
            443, 443, 456, 428,
            426, 424, 426, 427,
    };
    int r_pst_mg[32] = {
            594, 585, 608, 608,
            573, 566, 587, 615,
            583, 593, 592, 596,
            568, 579, 585, 587,
            563, 552, 578, 577,
            567, 576, 590, 578,
            575, 573, 577, 574,
            581, 586, 585, 590,
    };
    int b_pst_eg[32] = {
            389, 388, 384, 390,
            374, 379, 378, 377,
            369, 368, 367, 357,
            366, 372, 364, 376,
            372, 373, 377, 370,
            386, 385, 378, 376,
            372, 368, 370, 374,
            379, 376, 363, 370,
    };
    int r_pst_eg[32] = {
            681, 690, 683, 687,
            672, 685, 682, 666,
            669, 669, 675, 661,
            673, 671, 673, 668,
            678, 680, 668, 671,
            673, 669, 667, 668,
            676, 672, 677, 673,
            679, 678, 678, 669,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            207, 155, 148, 116,
            139, 152, 142, 137,
            121, 111, 102, 105,
            107, 102, 102, 114,
            95, 85, 96, 89,
            95, 97, 101, 88,
    };
    int p_pst_eg[24] = {
            181, 202, 203, 183,
            116, 131, 127, 107,
            102, 109, 110, 99,
            98, 101, 100, 90,
            95, 96, 92, 87,
            94, 92, 89, 81,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -120, -79, -57, -82,
            -26, -1, -24, -47,
            28, -11, -58, -83,
            53, 44, 24, -10,
    };
    int k_pst_eg[16] = {
            13, 28, 25, 26,
            7, 15, 16, 18,
            -20, 2, 15, 21,
            -48, -24, -2, 4,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-4, -19}; // [OPEN FILE]
    int isolated_eg[2] = {-1, 1}; // [OPEN FILE]
    int backwards_mg[2] = {2, -16}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -12}; // [OPEN FILE]
    int semi_backwards_mg[2] = {3, -20}; // [OPEN FILE]
    int semi_backwards_eg[2] = {2, 2}; // [OPEN FILE]
    int paired_mg[2] = {9, -2}; // [OPEN FILE]
    int paired_eg[2] = {3, 8}; // [OPEN FILE]
    int detached_mg[2] = {-4, -1}; // [OPEN FILE]
    int detached_eg[2] = {-7, -8}; // [OPEN FILE]
    int doubled_mg[2] = {-17, 4}; // [OPEN FILE]
    int doubled_eg[2] = {-18, -29}; // [OPEN FILE]
    int chain_mg[5] = {17, 11, 20, 35, 207}; // [RANK - 2]
    int chain_eg[5] = {14, 7, 10, 27, -13}; // [RANK - 2]
    int passed_mg[6] = {-24, -34, -22, 7, 11, 22}; // [RANK - 1]
    int passed_eg[6] = {-39, -14, 23, 60, 117, 120}; // [RANK - 1]
    int candidate_mg[4] = {-20, -5, 15, 61}; // [RANK - 1]
    int candidate_eg[4] = {-5, 0, 13, 48}; // [RANK - 1]
    int king_tropism_eg[2] = {-1, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-7, 15}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg[2] = {-7, -5}; // [OWN, OTHER]
    int blocked_eg[2] = {-4, -34}; // [OWN, OTHER]
    int pos_r_open_file_mg = 48;
    int pos_r_open_file_eg = 13;
    int pos_r_own_half_open_file_mg = 26; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = -3;
    int pos_r_other_half_open_file_mg = 20;
    int pos_r_other_half_open_file_eg = 1;

    // [KNIGHT, BISHOP]
    int outpost_mg[2] = {6, 9};
    int outpost_eg[2] = {12, 8};
    int outpost_hole_mg[2] = {41, 43};
    int outpost_hole_eg[2] = {13, -3};
    int outpost_half_mg[2] = {-3, 4};
    int outpost_half_eg[2] = {-7, 1};

    /// King safety
    int ks_pawn_shield[4] = {-20, -5, 14, 35}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 6;
    int kat_open_file = 23;
    int kat_own_half_open_file = 19;
    int kat_other_half_open_file = 14;
    int kat_attack_weight[5] = {8, 15, 11, 9, 11};
    int kat_defence_weight[5] = {10, 3, 3, 3, 1};

    int kat_table_scale = 23;
    int kat_table_translate = 77;
    int kat_table_max = 441;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 25;
    int pos_bishop_pair_eg = 69;

    int pos_r_trapped_mg = -73;
    int pos_r_behind_own_passer_eg = -2;
    int pos_r_behind_enemy_passer_eg = 58;
    int pos_r_xray_pawn_eg = 17;

    int pos_mob_mg[4] = {6, 7, 3, 2}; // [PIECE, ¬KING, ¬PAWN]
    int pos_mob_eg[4] = {5, 6, 3, 6};

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

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    void eval_pawns(const board_t &board, int &mg, int &eg);
    void eval_pst(const board_t &board, int &mg, int &eg);
    void eval_movement(const board_t &board, int &mg, int &eg);
    void eval_positional(const board_t &board, int &mg, int &eg);
};

#endif //TOPPLE_EVAL_H
