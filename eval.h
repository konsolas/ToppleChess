//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include "types.h"
#include "board.h"

enum GamePhase {
    MG, EG
};

struct eval_params_t {
    /// Material
    int mat_mg[5] = {80, 225, 287, 524, 993};
    int mat_eg[5] = {82, 407, 439, 589, 1228};

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            18, 35, 49, 50,
            -9, 19, 31, 33,
            -18, 8, 5, 23,
            2, -63, 11, 12,
    };
    int q_pst_mg[16] = {
            1, 34, 24, 51,
            -9, -2, 24, 25,
            -22, -3, -11, 11,
            -16, -1, -2, -17,
    };
    int n_pst_eg[16] = {
            18, 37, 48, 54,
            19, 46, 51, 55,
            -7, 7, 36, 28,
            -121, 11, -15, -4,
    };
    int q_pst_eg[16] = {
            -20, -35, -23, -35,
            -8, 0, -14, -14,
            -33, -37, 7, -8,
            -24, -31, -29, -2,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -5, -15, -27, -11,
            0, -15, -8, -9,
            -7, -17, -19, -8,
            -3, 8, 0, 3,
            -8, -8, -2, 13,
            -11, -12, -10, -7,
            -10, -40, -28, -21,
            -29, -46, -61, -11,
    };
    int r_pst_mg[32] = {
            15, 11, 12, 4,
            -1, 4, -4, -4,
            3, -2, -3, -9,
            7, 4, -1, -6,
            13, 15, 16, 8,
            12, 10, -2, 7,
            18, 14, 7, 22,
            13, 20, 17, 1,
    };
    int b_pst_eg[32] = {
            -59, -38, -35, -44,
            -40, 11, -7, -60,
            -9, 33, 40, 3,
            15, 3, 7, 12,
            19, 13, 14, 6,
            14, 21, 23, 19,
            9, 22, 22, 3,
            -19, -9, -13, -33,
    };
    int r_pst_eg[32] = {
            44, 42, 40, 65,
            60, 55, 74, 74,
            31, 49, 49, 53,
            10, 15, 36, 36,
            -19, -17, -4, -6,
            -32, -16, -3, -23,
            -38, -22, -16, -40,
            -16, -12, -8, -3,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            232, 208, 231, 223,
            84, 73, 79, 70,
            19, 19, 29, 14,
            5, 15, 7, -3,
            2, 13, -2, -5,
            7, 6, 9, -8,
    };
    int p_pst_eg[24] = {
            48, 81, -9, 6,
            11, 26, 12, 0,
            21, 19, 4, 19,
            18, -4, 9, 20,
            6, -5, 12, 6,
            1, 16, 12, 2,
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {
            69, 74, 93, 104, 104, 96, 82, 79,
            51, 73, 104, 121, 128, 102, 83, 70,
            6, 29, 92, 61, 63, 81, 49, 38,
    };
    int k_pst_exposed_mg = 101;
    int k_pst_eg[16] = {
            -32, 32, 71, 83, 24, 61, 52, 44, 53, 50, 22, -17, 65, 89, 19, 32,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{13, 7}, {15, 2}, {9, -8}, {-7, -17}, {-61, -6}, {0, -27},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{-11, -41}, {-21, -42}, {-17, -27}, {-6, -11}, {23, 9}, {0, 11},}; // [RANK][OPEN FILE]
    int doubled_mg[2] = {-33, -32}; // [OPEN FILE]
    int doubled_eg[2] = {4, 20,}; // [OPEN FILE]
    int blocked_mg[2] = {0, -1}; // [OPEN_FILE]
    int blocked_eg[2] = {-11, -13}; // [OPEN_FILE]
    int chain_mg[5] = {18, 3, -9, 0, -87,}; // [RANK]
    int chain_eg[5] = {6, 12, 24, 42, 163,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {3, 4, 35, 60, 78, 22,}; // [RANK]
    int passed_eg[6] = {0, 6, -5, 6, 14, 49,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-7, 5, 11, 16,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 19;
    int kat_attack_weight[5] = {-3, 9, 5, 3, 8,};
    int kat_defence_weight[5] = {0, 1, 1, 0, -2,};

    /// Mobility
    //int mob_mg[5] = {};
    //int mob_eg[5]

    /// Other positional
    int pos_bishop_pair_mg = 73;
    int pos_bishop_pair_eg = 18;
    // TODO: Square colour weaknesses, rook behind passed pawn, exchange when up material, mobility, etc.
};

class evaluator_t {
    size_t pawn_hash_entries;
    static constexpr size_t bucket_size = 4;
    struct pawn_entry_t {
        U64 hash;
        uint32_t hits;

        uint32_t eval_mg;
        uint32_t eval_eg;

        U64 passers[2];
        U64 defended[2];
        U64 open_files;
        U64 attackable[2];
    } *pawn_hash_table = nullptr;

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]

    eval_params_t params;
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_size);

    ~evaluator_t();

    int evaluate(const board_t &board);

    /// Initialise generic evaluation tables
    static void eval_init();

private:
    void eval_material(const board_t &board, int &mg, int &eg);

    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t *eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);

    void eval_positional(const board_t &board, int &mg, int &eg);
};

#endif //TOPPLE_EVAL_H
