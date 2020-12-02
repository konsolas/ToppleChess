//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

// lq: 0.0633654
struct eval_params_t {
    // Tapering parameters
    int mat_exch_knight = 24;
    int mat_exch_bishop = 32;
    int mat_exch_rook = 55;
    int mat_exch_queen = 155;
    int pt_blocked_file[4] = {65, 71, 117, 118,};
    int pt_open_file[4] = {24, -37, -32, -40,};
    int pt_island = 20;
    int pt_max = 499;

    // Piece-square tables
    // 4x4 tables include the following squares, mirrored horizontally and vertically
    // A4, B4, C4, D4,
    // A3, B3, C3, D3,
    // A2, B2, C2, D2,
    // A1, B1, C1, D1,
    // Pawn tables are mirrored vertically only
    v4si_t n_pst[16] = {{430, 391, 423, 377},
                        {424, 416, 419, 376},
                        {438, 422, 439, 382},
                        {467, 418, 460, 388},
                        {394, 380, 437, 370},
                        {410, 405, 418, 376},
                        {418, 414, 403, 385},
                        {432, 418, 434, 387},
                        {408, 378, 435, 357},
                        {403, 377, 438, 374},
                        {409, 385, 419, 383},
                        {419, 410, 421, 380},
                        {378, 230, 360, 324},
                        {391, 362, 454, 360},
                        {391, 378, 432, 360},
                        {394, 378, 433, 379},};
    v4si_t q_pst[16] = {{1323, 1321, 1275, 1276},
                        {1333, 1315, 1318, 1290},
                        {1316, 1314, 1292, 1288},
                        {1327, 1308, 1320, 1306},
                        {1323, 1335, 1307, 1232},
                        {1319, 1329, 1307, 1265},
                        {1320, 1325, 1281, 1288},
                        {1328, 1311, 1254, 1295},
                        {1326, 1335, 1226, 1243},
                        {1310, 1316, 1260, 1270},
                        {1320, 1327, 1277, 1256},
                        {1327, 1331, 1263, 1268},
                        {1316, 1326, 1255, 1209},
                        {1316, 1316, 1236, 1250},
                        {1323, 1310, 1250, 1253},
                        {1328, 1327, 1244, 1243},};
    v4si_t b_pst[16] = {{394, 415, 409, 393},
                        {414, 401, 395, 400},
                        {416, 419, 399, 389},
                        {435, 423, 393, 384},
                        {407, 433, 412, 384},
                        {422, 437, 382, 397},
                        {404, 441, 399, 394},
                        {433, 427, 395, 399},
                        {432, 423, 340, 384},
                        {401, 452, 382, 390},
                        {440, 427, 379, 388},
                        {423, 429, 399, 395},
                        {379, 428, 306, 405},
                        {449, 418, 387, 391},
                        {425, 406, 391, 395},
                        {432, 401, 403, 403},};
    v4si_t r_pst[16] = {{586, 575, 709, 686},
                        {610, 571, 690, 686},
                        {592, 572, 689, 691},
                        {618, 575, 687, 687},
                        {593, 571, 711, 681},
                        {593, 592, 706, 679},
                        {596, 575, 706, 686},
                        {594, 581, 701, 681},
                        {584, 574, 707, 690},
                        {578, 581, 720, 697},
                        {581, 584, 712, 696},
                        {582, 591, 712, 695},
                        {598, 577, 689, 704},
                        {599, 583, 696, 690},
                        {586, 595, 688, 692},
                        {596, 596, 684, 688},};
    v4si_t p_pst[24] = {{164, 231, 161, 197},
                        {199, 158, 194, 215},
                        {144, 170, 146, 224},
                        {91,  138, 149, 202},
                        {94,  167, 111, 130},
                        {129, 174, 110, 152},
                        {119, 161, 96,  149},
                        {118, 153, 102, 122},
                        {108, 112, 107, 110},
                        {109, 114, 100, 120},
                        {91,  114, 109, 116},
                        {89,  111, 105, 107},
                        {97,  117, 115, 101},
                        {105, 102, 112, 106},
                        {110, 103, 105, 105},
                        {103, 120, 108, 93},
                        {92,  95,  115, 99},
                        {90,  82,  104, 105},
                        {106, 96,  96,  96},
                        {99,  88,  103, 92},
                        {103, 98,  97,  101},
                        {96,  98,  104, 103},
                        {107, 104, 93,  95},
                        {104, 83,  91,  89},};
    v4si_t k_pst[16] = {{-82, -43,  15,  -11},
                        {-47, -89,  35,  16},
                        {-47, -75,  57,  10},
                        {-23, -133, 56,  16},
                        {8,   35,   -6,  -11},
                        {6,   -31,  -11, 14},
                        {-19, -61,  6,   12},
                        {-2,  -100, 4,   17},
                        {37,  45,   -42, -12},
                        {-4,  -25,  2,   14},
                        {-36, -91,  13,  27},
                        {-66, -115, 20,  29},
                        {54,  83,   -72, -48},
                        {44,  38,   -12, -13},
                        {-11, 19,   13,  0},
                        {-19, -13,  6,   5},};

    // Pawn structure
    v4si_t isolated[2] = {{-8,  -7,  6, -4},
                          {-16, -18, 2, 0},};
    v4si_t backwards[2] = {{-6,  1,   -2,  -5},
                           {-27, -12, -22, -11},};
    v4si_t semi_backwards[2] = {{0,   5,   -2, 3},
                                {-10, -11, 0,  -1},};
    v4si_t paired[2] = {{10, 10, 4,  3},
                        {9,  6,  19, 4},};
    v4si_t detached[2] = {{-5, -6, -1, -5},
                          {3,  -4, -8, -6},};
    v4si_t doubled[2] = {{5, -10, -20, -18},
                         {6, 17,  -15, -38},};
    v4si_t chain[5] = {{17, 21,  22,  17},
                       {14, 9,   11,  14},
                       {15, 20,  16,  20},
                       {12, 76,  72,  15},
                       {45, 285, 144, -40},};
    v4si_t passed[6] = {{-13, -16, -44, -40},
                        {-22, -23, -4,  -15},
                        {-16, -19, 41,  23},
                        {26,  -3,  73,  67},
                        {29,  -6,  100, 125},
                        {-14, 18,  99,  132},};
    v4si_t candidate[4] = {{-18, -18, -8,  -8},
                           {-18, -2,  13,  -1},
                           {-18, 14,  27,  15},
                           {46,  54,  116, 39},};

    // Interaction of pieces and pawn structure
    v4si_t king_tropism[2] = {{-6, -6, 5, 0},
                              {-1, 0,  6, 5},};
    v4si_t passer_tropism[2] = {{6, 2, -19, -8},
                                {2, 0, 16,  15},};
    v4si_t blocked[2] = {{-9, -6, -10, -4},
                         {-3, -5, -17, -37},};
    v4si_t pos_r_open_file = {28, 41, 37, 12};
    v4si_t pos_r_own_half_open_file = {19, 20, -10, -1};
    v4si_t pos_r_other_half_open_file = {19, 15, 15, 6};
    v4si_t outpost[2] = {{-11, 4,  12, 1},
                         {8,   -6, -1, 12},};
    v4si_t outpost_hole[2] = {{38, 32, 9,  15},
                              {22, 45, -8, -4},};
    v4si_t outpost_half[2] = {{-1, -8, 6,  -19},
                              {-6, 1,  34, -8},};
    v4si_t ks_pawn_shield[4] = {{56, -31, 29, 12},
                                {38, -8,  11, 11},
                                {27, 16,  2,  0},
                                {15, 39,  5,  -13},};

    // King safety
    int kat_zero = 9;
    int kat_open_file = 23;
    int kat_own_half_open_file = 16;
    int kat_other_half_open_file = 13;
    int kat_attack_weight[5] = {9, 16, 11, 10, 11,};
    int kat_defence_weight[5] = {8, 3, 4, 2, 1,};
    int kat_table_scale = 46;
    int kat_table_translate = 78;
    v4si_t kat_table_max = {445, 445, -20, -101};

    // Dynamic threats
    v4si_t undefended[5] = {{-1,  -10, 0,  4},
                            {-17, -16, 14, -4},
                            {-14, -14, 6,  -7},
                            {4,   -4,  -9, -7},
                            {-18, -13, 19, 2},};
    v4si_t overprotected[5] = {{2,  4, -10, 4},
                               {15, 7, -4,  14},
                               {2,  6, 19,  16},
                               {7,  7, 19,  7},
                               {6,  3, 28,  20},};
    v4si_t threat_matrix[4][5] = {
            {{46, 3,   35, -10}, {62,  52,  5,   -9},  {81,  36,  13,  40},  {50,  52, -24, -48}, {50, 44, -102, -56},},
            {{-7, -12, 8,  1},   {140, -31, 169, 169}, {28,  23,  6,   30},  {48,  65, 49,  -21}, {22, 41, -88,  -36},},
            {{7,  -3,  31, 12},  {31,  27,  61,  19},  {-11, -11, -37, -37}, {30,  15, 25,  -7},  {43, 20, 91,   112},},
            {{0,  -4,  21, 18},  {8,   15,  31,  16},  {27,  28,  16,  18},  {256, 5,  -5,  -9},  {75, 62, 34,   -9},},
    };

    // Other positional factors
    v4si_t pos_bishop_pair{24, 33, 75, 70};
    v4si_t mat_opp_bishop[3] = {{-18, -17, 32, 67},
                                {-7,  -17, 67, 70},
                                {70,  4,   86, 64},};
    v4si_t pos_r_trapped = {-9, -73, -9, -17};
    v4si_t pos_r_behind_own_passer = {-7, 10, -22, -5};
    v4si_t pos_r_behind_enemy_passer = {-52, 40, 66, 57};
    v4si_t pos_mob[4] = {{11, 7, 24, 4},
                         {10, 7, 13, 6},
                         {3,  3, 13, 4},
                         {5,  4, 16, 6},};

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

        U64 team_attacks[2] = {};
        U64 attacks[2][6] = {};
        U64 double_attacks[2] = {};

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
