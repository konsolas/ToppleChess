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
    int mat_mg[5] = {78, 229, 283, 528, 1014};
    int mat_eg[5] = {78, 385, 438, 565, 1181};

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16]{
        31, 42, 51, 60,
        3, 21, 41, 41,
        -18, 5, 6, 25,
        -3, -50, 9, 18
    };
    int q_pst_mg[16]{
        2, 19, 21, 41,
        -22, -3, 23, 25,
        -8, 10, 3, 13,
        -10, -10, -2, -29
    };
    int n_pst_eg[16]{
        10, 28, 48, 45,
        14, 40, 47, 58,
        4, 14, 49, 31,
        -130, 8, -6, -4
    };
    int q_pst_eg[16]{
        -27, -22, -22, -24,
        -7, 0, -7, -12,
        -32, -34, -4, -8,
        -21, -31, -27, -4
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -30, -18, -35, -18,
            -4, -19, -18, -40,
            -9, -8, -21, -22,
            -3, -2, -1, -3,
            -8, -1, -8, 2,
            -7, -11, -22, -12,
            -15, -35, -22, -24,
            -30, -49, -75, -17
    };
    int r_pst_mg[32]{
        17, 13, 13, 1,
        3, 12, -4, -10,
        5, -2, -5, -12,
        6, 5, 2, -7,
        14, 14, 11, 5,
        12, 5, -5, 1,
        16, 14, 12, 8,
        19, 20, 16, 3
    };
    int b_pst_eg[32]{
        -43, -56, -17, -52,
        -50, 3, -8, 2,
        -4, 13, 40, 29,
        6, -7, 3, 3,
        18, 6, 13, -3,
        14, 19, 21, 15,
        5, 22, 20, 3,
        -17, -18, -4, -21
    };
    int r_pst_eg[32]{
        44, 45, 41, 64,
        38, 27, 66, 70,
        32, 44, 43, 54,
        6, 14, 32, 44,
        -16, -22, -9, -1,
        -28, -15, 5, 2,
        -35, -23, -13, -22,
        -18, -15, -3, -2
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24]{
        212, 213, 220, 194,
        94, 74, 80, 76,
        13, 15, 23, 10,
        -2, 7, 1, -4,
        -2, 9, 4, 0,
        3, 13, 21, -8
    };
    int p_pst_eg[24]{
        9, -3, -21, -9,
        -3, 23, 11, -3,
        18, 8, -3, 10,
        13, -9, 5, 10,
        8, -12, 9, 1,
        6, 16, 14, 11
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24]{
        54, 72, 92, 110, 110, 98, 82, 78,
        53, 74, 104, 125, 128, 109, 87, 73,
        30, 37, 96, 49, 52, 77, 65, 48,
    };
    int k_pst_exposed_mg = 92;
    int k_pst_eg[16]{
        -19, 53, 91, 100,
        27, 63, 50, 25,
        60, 50, 22, -19,
        65, 83, 19, 33,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{14, 9}, {15, 3}, {16, -1}, {1, -10}, {-70, 2}, {0, -2}}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{-9, -42}, {-19, -41}, {-18, -23}, {-7, -9}, {31, -4}, {0, -1}}; // [RANK][OPEN FILE]
    int doubled_mg[2] = {-26, -28}; // [OPEN FILE]
    int doubled_eg[2] = {7, 18}; // [OPEN FILE]
    int blocked_mg[2] = {-4, -2}; // [OPEN_FILE]
    int blocked_eg[2] = {-10, -7}; // [OPEN_FILE]
    int chain_mg[5] = {21, 11, 7, -3, -74}; // [RANK]
    int chain_eg[5] = {6, 12, 21, 36, 119}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {0, 5, 30, 56, 70, 21}; // [RANK]
    int passed_eg[6] = {-12, 0, -7, 2, -5, 16}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-20, -3, 7, 29}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 22;
    int kat_attack_weight[5] = {-3, 9, 5, 3, 8};
    int kat_defence_weight[5] = {1, 1, 0, 0, -1};

    /// Other positional
    int pos_bishop_pair_mg = 82;
    int pos_bishop_pair_eg = 8;
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
