//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

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
    v4si_t n_pst[16] = {
            {404, 404, 375, 375},
            {421, 421, 378, 378},
            {429, 429, 383, 383},
            {432, 432, 388, 388},
            {387, 387, 372, 372},
            {413, 413, 381, 381},
            {422, 422, 386, 386},
            {427, 427, 390, 390},
            {387, 387, 362, 362},
            {385, 385, 380, 380},
            {396, 396, 383, 383},
            {415, 415, 381, 381},
            {275, 275, 312, 312},
            {372, 372, 364, 364},
            {393, 393, 365, 365},
            {387, 387, 379, 379},
    };
    v4si_t q_pst[16] = {
            {1318, 1318, 1255, 1255},
            {1321, 1321, 1271, 1271},
            {1316, 1316, 1266, 1266},
            {1310, 1310, 1298, 1298},
            {1333, 1333, 1219, 1219},
            {1329, 1329, 1253, 1253},
            {1324, 1324, 1275, 1275},
            {1315, 1315, 1274, 1274},
            {1326, 1326, 1228, 1228},
            {1315, 1315, 1246, 1246},
            {1327, 1327, 1243, 1243},
            {1331, 1331, 1249, 1249},
            {1327, 1327, 1201, 1201},
            {1318, 1318, 1223, 1223},
            {1314, 1314, 1239, 1239},
            {1328, 1328, 1227, 1227},
    };
    v4si_t b_pst[16] = {
            {412, 412, 382, 382},
            {406, 406, 388, 388},
            {420, 420, 383, 383},
            {426, 426, 377, 377},
            {427, 427, 378, 378},
            {438, 438, 385, 385},
            {435, 435, 387, 387},
            {430, 430, 389, 389},
            {425, 425, 364, 364},
            {438, 438, 378, 378},
            {435, 435, 375, 375},
            {429, 429, 386, 386},
            {408, 408, 376, 376},
            {432, 432, 376, 376},
            {412, 412, 383, 383},
            {413, 413, 390, 390},
    };
    v4si_t r_pst[16] = {
            {572, 572, 679, 679},
            {578, 578, 673, 673},
            {570, 570, 678, 678},
            {576, 576, 673, 673},
            {579, 579, 672, 672},
            {591, 591, 671, 671},
            {577, 577, 677, 677},
            {580, 580, 671, 671},
            {573, 573, 684, 684},
            {577, 577, 689, 689},
            {576, 576, 688, 688},
            {583, 583, 686, 686},
            {580, 580, 688, 688},
            {585, 585, 680, 680},
            {590, 590, 682, 682},
            {591, 591, 676, 676},
    };

    // PST for pawns: mirrored horizontally with first + eighth excluded
    v4si_t p_pst[24] = {
            {205, 205, 184, 184},
            {158, 158, 204, 204},
            {149, 149, 206, 206},
            {121, 121, 183, 183},
            {132, 132, 122, 122},
            {145, 145, 138, 138},
            {133, 133, 133, 133},
            {135, 135, 111, 111},
            {110, 110, 109, 109},
            {108, 108, 113, 113},
            {99, 99, 114, 114},
            {99, 99, 104, 104},
            {106, 106, 103, 103},
            {100, 100, 105, 105},
            {100, 100, 104, 104},
            {111, 111, 95, 95},
            {94, 94, 99, 99},
            {83, 83, 100, 100},
            {96, 96, 94, 94},
            {88, 88, 92, 92},
            {95, 95, 98, 98},
            {94, 94, 98, 98},
            {101, 101, 92, 92},
            {86, 86, 86, 86},
    };


    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    v4si_t k_pst[16] = {
            {-55, -55, 8, 8},
            {-80, -80, 30, 30},
            {-51, -51, 24, 24},
            {-73, -73, 24, 24},
            {26, 26, 6, 6},
            {-10, -10, 20, 20},
            {-23, -23, 17, 17},
            {-46, -46, 18, 18},
            {29, 29, -13, -13},
            {-15, -15, 4, 4},
            {-58, -58, 17, 17},
            {-80, -80, 21, 21},
            {54, 54, -42, -42},
            {37, 37, -19, -19},
            {26, 26, -2, -2},
            {0, 0, 3, 3},
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.

    // Indexed by [OPEN FILE]
    v4si_t isolated[2] = {{-4, -4, -2, -2}, {-17, -17, 0, 0}};
    v4si_t backwards[2] = {{1, 1, -4, -4}, {-17, -17, -14, -14}};
    v4si_t semi_backwards[2] = {{5, 5, 3, 3}, {-17, -17, -2, -2}};
    v4si_t paired[2] = {{10, 10, 3, 3}, {0, 0, 4, 4}};
    v4si_t detached[2] = {{-5, -5, -6, -6}, {-3, -3, -7, -7}};
    v4si_t doubled[2] = {{-16, -16, -21, -21}, {5, 5, -32, -32}};

    // Indexed by [RANK - 2]
    v4si_t chain[5] = {
            {16, 16, 17, 17},
            {8, 8, 12, 12},
            {15, 15, 16, 16},
            {30, 30, 33, 33},
            {209, 209, -3, -3},
    };

    // Indexed by [RANK - 1]
    v4si_t passed[6] = {
            {-23, -23, -40, -40},
            {-31, -31, -17, -17},
            {-20, -20, 21, 21},
            {10, 10, 59, 59},
            {14, 14, 116, 116},
            {12, 12, 124, 124},
    };
    v4si_t candidate[4] = {
            {-14, -14, -11, -11},
            {-6, -6, 1, 1},
            {10, 10, 15, 15},
            {56, 56, 48, 48},
    };
    v4si_t king_tropism[2] = {{0, 0, -1, -1}, {0, 0, 5, 5}};
    v4si_t passer_tropism[2] = {{0, 0, -8, -8}, {0, 0, 15, 15}};

    // Evaluation of pawn structure relative to other pieces
    v4si_t blocked[2] = {{-7, -7, -5, -5}, {-5, -5, -33, -33}};
    v4si_t pos_r_open_file = {41, 41, 13, 13};
    v4si_t pos_r_own_half_open_file = {24, 24, -5, -5}; // "own" refers to side with missing pawn
    v4si_t pos_r_other_half_open_file = {16, 16, 4, 4};

    // [KNIGHT, BISHOP]
    v4si_t outpost[2] = {{5, 5, 7, 7}, {-2, -2, 9, 9}};
    v4si_t outpost_hole[2] = {{33, 33, 17, 17}, {37, 37, -5, -5}};
    v4si_t outpost_half[2] = {{-3, -3, -6, -6}, {4, 4, 1, 1}};

    /// King safety
    // 0, 1, 2, and 3 pawns close to the king
    v4si_t ks_pawn_shield[4] = {{-18, -18, 0, 0}, {-3, -3, 0, 0}, {14, 14, 0, 0}, {34, 34, 0, 0}};

    int kat_zero = 8;
    int kat_open_file = 24;
    int kat_own_half_open_file = 17;
    int kat_other_half_open_file = 14;
    int kat_attack_weight[5] = {8, 15, 11, 9, 11};
    int kat_defence_weight[5] = {8, 4, 4, 2, 1};

    int kat_table_scale = 45;
    int kat_table_translate = 77;
    v4si_t kat_table_max = {430, 430, 0, 0};

    /// Threats
    v4si_t undefended[5] = {
            {-9, -9, 6, 6},
            {-17, -17, -1, -1},
            {-13, -13, -5, -5},
            {-3, -3, -7, -7},
            {-12, -12, -6, -6},
    };
    v4si_t threat_matrix[4][5] = {
            {{20, 20, -2, -2}, {50, 50, -10, -10}, {46, 46, 32, 32}, {49, 49, -43, -43}, {40, 40, -57, -57}},
            {{-9, -9, 6, 6}, {24, 24, 159, 159}, {24, 24, 24, 24}, {61, 61, -11, -11}, {35, 35, -45, -45}},
            {{1, 1, 13, 13}, {28, 28, 26, 26}, {-11, -11, -37, -37}, {19, 19, -1, -1}, {26, 26, 107, 107}},
            {{-3, -3, 17, 17}, {12, 12, 17, 17}, {27, 27, 14, 14}, {42, 42, 3, 3}, {68, 68, -1, -1}}
    };

    /// Other positional
    v4si_t pos_bishop_pair = {25, 25, 67, 67};
    v4si_t mat_opp_bishop[3] = {{0, 0, 53, 53}, {0, 0, 64, 64}, {0, 0, 69, 69}}; // [PAWN ADVANTAGE]

    v4si_t pos_r_trapped = {-68, -68, 0, 0};
    v4si_t pos_r_behind_own_passer = {0, 0, -4, -4};
    v4si_t pos_r_behind_enemy_passer = {0, 0, 59, 59};

    v4si_t pos_mob[4] = {{7, 7, 5, 5}, {7, 7, 6, 6}, {3, 3, 4, 4}, {3, 3, 6, 6}}; // [PIECE, ¬KING, ¬PAWN]

    // TODO: Evaluate better
};

struct processed_params_t : public eval_params_t {
    explicit processed_params_t(const eval_params_t &params);

    v4si_t pst[2][6][64] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    v4si_t kat_table[128] = {};
};

class alignas(64) evaluator_t {
    pawns::structure_t *pawn_hash_table;
    size_t pawn_hash_entries;

    const processed_params_t &params;
public:
    evaluator_t(const processed_params_t &params, size_t pawn_hash_size);
    ~evaluator_t();
    evaluator_t(const evaluator_t &) = delete;

    int evaluate(const board_t &board);
    void prefetch(U64 pawn_hash);

    /// Initialise generic evaluation tables
    static void eval_init();

    [[nodiscard]] float game_phase(const board_t &board) const; // returns tapering factor 0-1
private:
    struct eval_data_t {
        int king_pos[2];
        U64 king_circle[2];
        int king_danger[2];

        U64 team_attacks[2];
        U64 attacks[2][6];
        U64 double_attacks[2];

        void update_attacks(Team team, Piece piece, U64 bb) {
            double_attacks[team] |= team_attacks[team] & bb;
            attacks[team][piece] |= bb;
            team_attacks[team] |= bb;
        }
    };

    v4si_t eval_pawns(const board_t &board, eval_data_t &data, float &taper);
    v4si_t eval_pieces(const board_t &board, eval_data_t &data);
    v4si_t eval_threats(const board_t &board, eval_data_t &data);
    v4si_t eval_positional(const board_t &board, eval_data_t &data);
};

#endif //TOPPLE_EVAL_H
