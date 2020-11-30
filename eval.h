//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

// lq: 0.0635743
struct eval_params_t {
    // Tapering parameters
    int mat_exch_knight = 24;
    int mat_exch_bishop = 32;
    int mat_exch_rook = 54;
    int mat_exch_queen = 155;

    int pt_blocked_file[4] = {75, 95, 133, 145,};
    int pt_half_open_file[4] = {9, 3, 4, 23,};
    int pt_open_file[4] = {15, -24, -22, -15,};

    // Piece-square tables
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1,
    // mirrored vertically for pawns
    v4si_t n_pst[16] = {
            {429, 391, 408, 378},
            {424, 416, 420, 376},
            {442, 424, 430, 381},
            {471, 425, 444, 388},
            {388, 382, 420, 367},
            {413, 408, 413, 377},
            {420, 419, 403, 384},
            {438, 421, 424, 387},
            {400, 378, 425, 358},
            {412, 377, 426, 375},
            {408, 389, 414, 384},
            {421, 411, 417, 379},
            {380, 239, 377, 319},
            {393, 361, 427, 360},
            {401, 380, 423, 366},
            {399, 378, 420, 383},
    };
    v4si_t q_pst[16] = {
            {1319, 1318, 1250, 1269},
            {1334, 1315, 1311, 1285},
            {1316, 1315, 1287, 1282},
            {1331, 1312, 1307, 1299},
            {1318, 1333, 1274, 1228},
            {1319, 1329, 1277, 1260},
            {1324, 1325, 1262, 1288},
            {1334, 1312, 1253, 1291},
            {1317, 1330, 1227, 1241},
            {1310, 1314, 1253, 1265},
            {1319, 1328, 1266, 1249},
            {1331, 1331, 1254, 1263},
            {1317, 1322, 1258, 1203},
            {1312, 1315, 1237, 1238},
            {1325, 1311, 1251, 1241},
            {1334, 1327, 1228, 1242},
    };
    v4si_t b_pst[16] = {
            {388, 414, 409, 389},
            {416, 401, 390, 394},
            {417, 419, 395, 388},
            {435, 424, 398, 383},
            {407, 429, 398, 383},
            {422, 440, 382, 394},
            {404, 441, 388, 392},
            {433, 428, 389, 399},
            {427, 423, 334, 378},
            {402, 449, 381, 385},
            {437, 427, 376, 388},
            {423, 429, 393, 392},
            {371, 425, 311, 395},
            {438, 423, 386, 384},
            {427, 407, 385, 395},
            {434, 399, 396, 401},
    };
    v4si_t r_pst[16] = {
            {580, 568, 702, 685},
            {609, 571, 680, 684},
            {591, 572, 682, 689},
            {633, 573, 674, 681},
            {586, 575, 705, 675},
            {587, 591, 703, 675},
            {597, 574, 700, 683},
            {599, 581, 695, 677},
            {577, 573, 709, 685},
            {577, 578, 721, 691},
            {580, 579, 707, 694},
            {583, 589, 700, 692},
            {594, 577, 687, 697},
            {593, 584, 695, 684},
            {591, 595, 688, 688},
            {596, 596, 684, 682},
    };
    v4si_t p_pst[24] = {
            {218, 213, 166, 191},
            {193, 158, 191, 210},
            {158, 163, 159, 217},
            {94,  137, 147, 194},
            {110, 154, 116, 130},
            {130, 166, 118, 148},
            {128, 151, 103, 148},
            {121, 143, 116, 119},
            {109, 111, 107, 110},
            {108, 113, 94,  119},
            {88,  111, 109, 116},
            {88,  111, 105, 104},
            {96,  115, 115, 101},
            {105, 102, 112, 106},
            {109, 101, 104, 105},
            {103, 118, 106, 94},
            {92,  95,  110, 99},
            {90,  82,  100, 102},
            {103, 96,  95,  96},
            {96,  88,  103, 92},
            {103, 98,  96,  101},
            {96,  96,  105, 101},
            {106, 102, 93,  94},
            {103, 83,  90,  89},
    };
    v4si_t k_pst[16] = {
            {-40, -62,  5,   -5},
            {-19, -88,  35,  18},
            {-7,  -69,  57,  12},
            {-27, -115, 64,  16},
            {7,   34,   -2,  -8},
            {8,   -27,  -2,  16},
            {-20, -47,  10,  12},
            {9,   -91,  5,   19},
            {32,  45,   -38, -12},
            {1,   -20,  8,   11},
            {-27, -83,  13,  25},
            {-64, -105, 17,  28},
            {49,  81,   -66, -48},
            {44,  38,   -12, -13},
            {-6,  23,   2,   -1},
            {-20, -11,  -4,  5},
    };

    // Pawn structure
    v4si_t isolated[2] = {{-4,  -7,  8,  -4},
                          {-16, -18, 11, 0},};
    v4si_t backwards[2] = {{-3,  1,   -4,  -5},
                           {-27, -13, -21, -12},};
    v4si_t semi_backwards[2] = {{0,   5,   -2, 3},
                                {-10, -14, 0,  -1},};
    v4si_t paired[2] = {{10, 10, 4,  3},
                        {8,  6,  20, 4},};
    v4si_t detached[2] = {{-6, -7, -1, -5},
                          {1,  -4, -7, -7},};
    v4si_t doubled[2] = {{0, -12, -19, -18},
                         {4, 18,  -31, -35},};
    v4si_t chain[5] = {{17,  21,  21, 19},
                       {14,  9,   10, 14},
                       {16,  20,  16, 21},
                       {16,  72,  49, 21},
                       {101, 251, 58, -17},};
    v4si_t passed[6] = {{-16, -16, -43, -40},
                        {-22, -23, -3,  -15},
                        {-15, -19, 46,  23},
                        {25,  0,   81,  64},
                        {27,  5,   115, 120},
                        {-1,  17,  116, 130},};
    v4si_t candidate[4] = {{-18, -17, -9,  -11},
                           {-22, -2,  20,  0},
                           {-20, 12,  30,  15},
                           {73,  50,  113, 42},};

    // Interaction of pawn structure with pieces
    v4si_t king_tropism[2] = {{-5, -6, 5, 0},
                              {-1, 0,  6, 5},};
    v4si_t passer_tropism[2] = {{5, 2, -19, -8},
                                {2, 0, 15,  15},};
    v4si_t blocked[2] = {{-9, -6, -10, -4},
                         {-3, -5, -18, -37},};
    v4si_t pos_r_open_file = {28, 41, 42, 12};
    v4si_t pos_r_own_half_open_file = {19, 21, -1, -4};
    v4si_t pos_r_other_half_open_file = {19, 15, 11, 6};
    v4si_t outpost[2] = {{-10, 4,  19, 1},
                         {3,   -3, 2,  11},};
    v4si_t outpost_hole[2] = {{37, 32, 13,  17},
                              {22, 43, -18, 0},};
    v4si_t outpost_half[2] = {{9,  -8, -2, -13},
                              {-2, 3,  27, -7},};
    v4si_t ks_pawn_shield[4] = {{50, -27, 27, 8},
                                {39, -8,  13, 11},
                                {28, 16,  2,  2},
                                {16, 39,  5,  -12},};

    // King safety
    int kat_zero = 9;
    int kat_open_file = 23;
    int kat_own_half_open_file = 16;
    int kat_other_half_open_file = 13;
    int kat_attack_weight[5] = {9, 16, 11, 10, 11,};
    int kat_defence_weight[5] = {8, 4, 4, 2, 1,};
    int kat_table_scale = 46;
    int kat_table_translate = 78;
    v4si_t kat_table_max = {445, 438, -19, -101};

    // Threats
    v4si_t undefended[5] = {{-1,  -10, 4,  4},
                            {-19, -19, 17, -4},
                            {-13, -14, 5,  -7},
                            {-1,  -4,  -9, -7},
                            {-13, -12, 1,  -5},};
    v4si_t threat_matrix[4][5] = {
            {{39, 7,   42, -5}, {62,  52,  0,   -8},  {80,  41,  17,  36},  {44,  50, -22, -43}, {49, 43, -87, -56},},
            {{-8, -12, 9,  4},  {135, -16, 171, 173}, {27,  25,  11,  29},  {52,  61, 33,  -14}, {22, 40, -79, -51},},
            {{6,  -1,  31, 12}, {28,  28,  68,  18},  {-11, -11, -37, -37}, {24,  16, 19,  -4},  {47, 23, 67,  112},},
            {{0,  -5,  21, 17}, {7,   15,  25,  17},  {27,  27,  14,  14},  {167, 25, 29,  -4},  {87, 63, 27,  -8},},
    };

    // Positional
    v4si_t pos_bishop_pair{22, 29, 70, 71};
    v4si_t mat_opp_bishop[3] = {{-23, -18, 58, 62},
                                {-10, -15, 89, 67},
                                {52,  8,   97, 59},};
    v4si_t pos_r_trapped = {-17, -73, -15, -12};
    v4si_t pos_r_behind_own_passer = {-8, 11, -15, -8};
    v4si_t pos_r_behind_enemy_passer = {-26, 36, 52, 59};

    v4si_t pos_mob[4] = {{11, 7, 19, 5},
                         {10, 7, 12, 6},
                         {3,  3, 11, 4},
                         {4,  3, 13, 6},};

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
