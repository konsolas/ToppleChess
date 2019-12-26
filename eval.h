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

// qlc: 0.058373 lqa: 0.0649299
struct eval_params_t {
    /// Material
    int mat_exch_knight = 27;
    int mat_exch_bishop = 35;
    int mat_exch_rook = 52;
    int mat_exch_queen = 146;
    int mat_opp_bishop[3] = {54, 64, 74}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            418, 422, 435, 435,
            392, 424, 431, 439,
            399, 397, 412, 427,
            277, 386, 410, 399
    };
    int q_pst_mg[16] = {
            1313, 1311, 1308, 1306,
            1331, 1329, 1325, 1316,
            1312, 1305, 1330, 1335,
            1314, 1324, 1321, 1333,
    };
    int n_pst_eg[16] = {
            364, 371, 373, 378,
            362, 370, 372, 377,
            352, 367, 371, 366,
            305, 341, 350, 366,
    };
    int q_pst_eg[16] = {
            1260, 1283, 1274, 1290,
            1220, 1253, 1267, 1269,
            1221, 1247, 1235, 1236,
            1202, 1207, 1227, 1219,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            357, 372, 378, 334,
            381, 397, 399, 394,
            419, 443, 432, 448,
            441, 433, 432, 430,
            445, 425, 431, 445,
            440, 453, 451, 447,
            445, 447, 461, 432,
            427, 425, 428, 428,
    };
    int r_pst_mg[32] = {
            584, 576, 605, 597,
            572, 566, 588, 616,
            588, 602, 595, 603,
            568, 578, 588, 588,
            563, 551, 578, 575,
            567, 575, 592, 579,
            574, 567, 576, 572,
            582, 588, 586, 589,
    };
    int b_pst_eg[32] = {
            389, 387, 384, 395,
            373, 378, 377, 377,
            369, 368, 368, 357,
            365, 373, 364, 374,
            372, 372, 378, 372,
            387, 384, 378, 380,
            372, 366, 368, 373,
            378, 374, 360, 368,
    };
    int r_pst_eg[32] = {
            679, 685, 680, 682,
            670, 682, 678, 663,
            668, 665, 672, 658,
            670, 670, 670, 665,
            675, 677, 667, 669,
            670, 667, 664, 666,
            674, 670, 674, 673,
            676, 673, 674, 666,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            207, 153, 159, 123,
            145, 157, 148, 145,
            122, 113, 105, 107,
            109, 102, 102, 116,
            95, 85, 99, 90,
            95, 98, 99, 91,
    };
    int p_pst_eg[24] = {
            181, 201, 201, 178,
            116, 135, 127, 106,
            102, 110, 111, 98,
            97, 101, 99, 90,
            95, 96, 91, 87,
            93, 92, 88, 80,
    };

    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    int k_pst_mg[16] = {
            -135, -99, -73, -93,
            -66, -56, -69, -87,
            30, -2, -43, -67,
            50, 48, 17, -21
    };
    int k_pst_eg[16] = {
            8, 25, 24, 26,
            5, 16, 16, 21,
            -20, 3, 18, 24,
            -50, -24, -3, 4,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[2] = {-5, -19}; // [OPEN FILE]
    int isolated_eg[2] = {-1, 0}; // [OPEN FILE]
    int backwards_mg[2] = {3, -17}; // [OPEN FILE]
    int backwards_eg[2] = {-6, -14}; // [OPEN FILE]
    int semi_backwards_mg[2] = {4, -18}; // [OPEN FILE]
    int semi_backwards_eg[2] = {2, 0}; // [OPEN FILE]
    int paired_mg[2] = {10, -1}; // [OPEN FILE]
    int paired_eg[2] = {2, 7}; // [OPEN FILE]
    int detached_mg[2] = {-3, -1}; // [OPEN FILE]
    int detached_eg[2] = {-8, -6}; // [OPEN FILE]
    int doubled_mg[2] = {-16, 5}; // [OPEN FILE]
    int doubled_eg[2] = {-19, -30}; // [OPEN FILE]
    int chain_mg[5] = {20, 13, 21, 40, 225}; // [RANK - 2]
    int chain_eg[5] = {14, 6, 9, 24, -22}; // [RANK - 2]
    int passed_mg[6] = {-30, -37, -24, 8, 10, 26}; // [RANK - 1]
    int passed_eg[6] = {-39, -16, 21, 60, 116, 123}; // [RANK - 1]
    int candidate_mg[4] = {-24, -4, 13, 63}; // [RANK - 1]
    int candidate_eg[4] = {-5, -1, 14, 46}; // [RANK - 1]
    int king_tropism_eg[2] = {0, 5}; // [OWN, OTHER]
    int passer_tropism_eg[2] = {-7, 15}; // [OWN, OTHER]

    // Evaluation of pawn structure relative to other pieces
    int blocked_mg[2] = {-7, -5}; // [OWN, OTHER]
    int blocked_eg[2] = {-4, -34}; // [OWN, OTHER]
    int pos_r_open_file_mg = 52;
    int pos_r_open_file_eg = 12;
    int pos_r_own_half_open_file_mg = 29; // "own" refers to side with missing pawn
    int pos_r_own_half_open_file_eg = -3;
    int pos_r_other_half_open_file_mg = 23;
    int pos_r_other_half_open_file_eg = 1;

    // [KNIGHT, BISHOP]
    int outpost_mg[2] = {8, 12};
    int outpost_eg[2] = {12, 7};
    int outpost_hole_mg[2] = {41, 42};
    int outpost_hole_eg[2] = {12, -6};
    int outpost_half_mg[2] = {-2, 4};
    int outpost_half_eg[2] = {-7, 1};

    /// King safety
    int ks_pawn_shield[4] = {-15, -1, 15, 26}; // 0, 1, 2, and 3 pawns close to the king

    int kat_zero = 5;
    int kat_open_file = 28;
    int kat_own_half_open_file = 22;
    int kat_other_half_open_file = 10;
    int kat_attack_weight[5] = {3, 16, 8, 8, 12};
    int kat_defence_weight[5] = {13, 3, 4, 4, 1};

    int kat_table_scale = 26;
    int kat_table_translate = 76;
    int kat_table_max = 429;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 27;
    int pos_bishop_pair_eg = 67;

    int pos_r_trapped_mg = -76;
    int pos_r_behind_own_passer_eg = 0;
    int pos_r_behind_enemy_passer_eg = 61;
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
