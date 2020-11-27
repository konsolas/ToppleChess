//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

// lqa: 0.06315
struct eval_params_t {
    /// Material
    int mat_exch_knight = 21;
    int mat_exch_bishop = 28;
    int mat_exch_rook = 51;
    int mat_exch_queen = 158;

    /// Piece-square tables

    // PST for N, B, R, Q, K:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // horizontal and vertical symmetry assumed
    v4si_t n_pst[16] = {{402, 375, 411, 348},
                        {410, 437, 391, 382},
                        {425, 412, 411, 374},
                        {431, 409, 413, 385},
                        {387, 351, 408, 345},
                        {415, 373, 401, 377},
                        {416, 416, 399, 383},
                        {426, 397, 415, 381},
                        {394, 329, 410, 327},
                        {394, 339, 412, 359},
                        {400, 363, 397, 389},
                        {416, 378, 396, 388},
                        {305, 180, 286, 383},
                        {385, 268, 422, 302},
                        {399, 323, 391, 369},
                        {389, 346, 414, 351},};
    v4si_t q_pst[16] = {{1317, 1323, 1265, 1269},
                        {1318, 1333, 1280, 1286},
                        {1312, 1334, 1276, 1272},
                        {1307, 1341, 1304, 1278},
                        {1334, 1327, 1232, 1232},
                        {1326, 1334, 1270, 1241},
                        {1324, 1336, 1281, 1259},
                        {1316, 1320, 1283, 1279},
                        {1328, 1351, 1224, 1227},
                        {1315, 1324, 1242, 1265},
                        {1329, 1309, 1253, 1258},
                        {1332, 1333, 1243, 1269},
                        {1327, 1319, 1223, 1185},
                        {1317, 1319, 1248, 1215},
                        {1322, 1293, 1238, 1262},
                        {1330, 1316, 1245, 1226},};
    v4si_t b_pst[16] = {{409, 395, 394, 394},
                        {401, 417, 394, 401},
                        {414, 434, 389, 388},
                        {422, 428, 394, 372},
                        {421, 421, 390, 389},
                        {432, 441, 390, 405},
                        {425, 454, 397, 387},
                        {426, 436, 400, 391},
                        {443, 337, 357, 418},
                        {434, 440, 385, 381},
                        {441, 383, 380, 399},
                        {426, 422, 401, 385},
                        {416, 372, 345, 439},
                        {436, 390, 381, 399},
                        {417, 372, 402, 380},
                        {424, 364, 401, 403},};
    v4si_t r_pst[16] = {{569, 583, 687, 680},
                        {578, 589, 680, 678},
                        {572, 586, 685, 685},
                        {573, 596, 677, 680},
                        {580, 558, 691, 674},
                        {590, 595, 680, 677},
                        {585, 570, 684, 686},
                        {574, 602, 678, 675},
                        {567, 593, 707, 670},
                        {570, 587, 712, 681},
                        {577, 587, 699, 687},
                        {579, 607, 699, 675},
                        {584, 553, 699, 688},
                        {587, 570, 680, 701},
                        {587, 600, 687, 691},
                        {588, 607, 676, 689},};

    // PST for pawns: mirrored horizontally with first + eighth excluded
    v4si_t p_pst[24] = {{163, 292, 124, 207},
                        {126, 231, 146, 220},
                        {125, 229, 126, 245},
                        {91,  208, 139, 196},
                        {119, 217, 108, 134},
                        {144, 192, 111, 158},
                        {128, 197, 112, 151},
                        {128, 194, 94,  145},
                        {121, 89,  110, 112},
                        {112, 112, 111, 116},
                        {100, 116, 114, 111},
                        {103, 103, 104, 105},
                        {114, 95,  104, 102},
                        {106, 89,  109, 105},
                        {107, 92,  105, 105},
                        {117, 102, 97,  94},
                        {95,  101, 101, 96},
                        {87,  80,  103, 101},
                        {98,  95,  95,  98},
                        {89,  96,  93,  93},
                        {102, 80,  102, 99},
                        {93,  113, 100, 98},
                        {99,  115, 92,  94},
                        {93,  60,  91,  87},};


    // PST for king
    // Horizontal, Vertical and diagonal symmetry, like N and Q
    v4si_t k_pst[16] = {{-126, 58,   0,   -18},
                        {-166, 8,    27,  2},
                        {-114, -49,  10,  10},
                        {-178, -57,  12,  9},
                        {28,   61,   -19, -9},
                        {-25,  8,    -18, 19},
                        {-71,  -27,  -7,  10},
                        {-94,  -67,  -4,  14},
                        {33,   114,  -48, 11},
                        {-11,  -2,   -4,  28},
                        {-38,  -138, 9,   34},
                        {-74,  -146, 18,  26},
                        {67,   134,  -64, -30},
                        {37,   65,   -11, -18},
                        {24,   -13,  15,  -19},
                        {0,    -75,  -1,  8},};

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.

    // Indexed by [OPEN FILE]
    v4si_t isolated[2] = {{-12, 5,   -4, 0},
                          {-16, -19, -8, 1},};
    v4si_t backwards[2] = {{-4,  11, -6,  1},
                           {-18, -1, -21, -2},};
    v4si_t semi_backwards[2] = {{3,   5,  3,  7},
                                {-16, -2, -1, 3},};
    v4si_t paired[2] = {{11, -4, 5, 3},
                        {2,  6,  7, 3},};
    v4si_t detached[2] = {{-6, -8, -3,  -5},
                          {-4, 8,  -17, 5},};
    v4si_t doubled[2] = {{-12, 16, -15, -36},
                         {8,   -3, -8,  -56},};

    // Indexed by [RANK - 2]
    v4si_t chain[5] = {{16,  24,  21, 9},
                       {8,   9,   18, 1},
                       {20,  -5,  16, 16},
                       {15,  82,  80, -27},
                       {182, 237, 78, -86},};

    // Indexed by [RANK - 1]
    v4si_t passed[6] = {{-16, -20, -44, -41},
                        {-13, -42, -22, -13},
                        {-3,  -45, 16,  30},
                        {8,   4,   33,  90},
                        {-3,  46,  52,  159},
                        {-31, 139, 86,  156},};
    v4si_t candidate[4] = {{-14, -21, -16, 4},
                           {2,   -36, 7,   -3},
                           {11,  -3,  8,   31},
                           {64,  39,  29,  83},};
    v4si_t king_tropism[2] = {{-2, -20, 2, -1},
                              {0,  2,   5, 6},};
    v4si_t passer_tropism[2] = {{4, 4, -11, -6},
                                {2, 0, 13,  17},};

    // Evaluation of pawn structure relative to other pieces
    v4si_t blocked[2] = {{-10, 15,  -5,  -11},
                         {-4,  -14, -15, -52},};
    v4si_t pos_r_open_file = {22, 57, 32, -8};
    v4si_t pos_r_own_half_open_file = {22, 18, -10, 6};
    v4si_t pos_r_other_half_open_file = {16, 8, 0, 12};

    // [KNIGHT, BISHOP]
    v4si_t outpost[2] = {{4,  -4, 10, -15},
                         {-1, 10, 9,  10},};
    v4si_t outpost_hole[2] = {{35, 32, 40, 1},
                              {29, 49, 5,  -19},};
    v4si_t outpost_half[2] = {{0,  -14, -3, -13},
                              {-1, -1,  11, -18},};

    /// King safety
    // 0, 1, 2, and 3 pawns close to the king
    v4si_t ks_pawn_shield[4] = {{14, -62, 13,  18},
                                {13, -33, 1,   17},
                                {13, 26,  -1,  -14},
                                {25, 67,  -12, -33},};

    int kat_zero = 8;
    int kat_open_file = 23;
    int kat_own_half_open_file = 17;
    int kat_other_half_open_file = 15;
    int kat_attack_weight[5] = {10, 16, 11, 9, 10,};
    int kat_defence_weight[5] = {7, 4, 4, 1, 2,};

    int kat_table_scale = 47;
    int kat_table_translate = 80;
    v4si_t kat_table_max = {443, 423, -74, -80};

    /// Threats
    v4si_t undefended[5] = {{-5,  -10, 2,  6},
                            {-13, -34, 8,  -16},
                            {-9,  -29, -3, -8},
                            {5,   -40, -2, -8},
                            {-9,  -25, 6,  0},};
    v4si_t threat_matrix[4][5] = {
            {{17, 29,  19, -51}, {52, 23,  0,   -1},  {48,  20,  40,  28},  {62,  -11, -24, -35}, {39, 46, -59, -63},},
            {{-9, -11, 5,  1},   {6,  104, 167, 149}, {18,  42,  19,  29},  {60,  44,  25,  -58}, {27, 56, -41, -58},},
            {{0,  1,   15, 15},  {21, 52,  25,  28},  {-11, -11, -37, -37}, {19,  18,  8,   -17}, {24, 32, 118, 104},},
            {{-5, -8,  20, 15},  {5,  33,  9,   28},  {17,  55,  9,   22},  {129, -52, -4,  -28}, {70, 23, 14,  -3},},
    };

    /// Other positional
    v4si_t pos_bishop_pair{26, 23, 72, 72};
    v4si_t mat_opp_bishop[3] = {{8,  -36, -2, 135},
                                {11, -43, -6, 159},
                                {22, -32, 77, 48},};

    v4si_t pos_r_trapped = {-37, -122, 10, -52};
    v4si_t pos_r_behind_own_passer = {7, 3, -28, 34};
    v4si_t pos_r_behind_enemy_passer = {4, 50, 50, 63};

    v4si_t pos_mob[4] = {{7, 3, 11, 1},
                         {6, 7, 7,  6},
                         {3, 4, 5,  4},
                         {3, 4, 8,  3},};

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
