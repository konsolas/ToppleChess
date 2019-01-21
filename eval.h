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
    int mat_mg[5] = {80, 362, 415, 528, 1174,};
    int mat_eg[5] = { 100, 367, 410, 685, 1311,};
    int mat_exch_scale = 3;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 3;
    int mat_exch_rook = 5;
    int mat_exch_queen = 12;

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {36, 58, 69, 74, 35, 67, 77, 72, 4, 12, 55, 47, -92, 24, -5, 16,
    };
    int q_pst_mg[16] = {-18, -31, -17, -26, -15, 10, -4, -10, -33, -34, 13, 0, -21, -30, -23, 4,
    };
    int n_pst_eg[16] = {-16, 5, 27, 32, -31, -9, 2, 17, -48, -16, -17, -1, -73, -68, -23, -23,
    };
    int q_pst_eg[16] = {-15, 31, 15, 49, -17, -31, 7, 16, -34, -15, -33, -9, -38, -15, -16, -33,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-84, -35, -84, -43, -52, -2, -18, -103, -29, 31, 22, -19, 4, -14, -17, -4, 9, -13, 0, -4, -12, 15, 5, 2, -11,
                        -2, 2, -23, -53, -32, -44, -57,
    };
    int r_pst_mg[32] = {75, 28, 8, 46, 47, 50, 56, 68, -6, 26, 42, 20, 3, 7, 10, 14, -30, -23, 4, -9, -36, -13, -2, -26, -41, -5, -14
            , -48, -13, -12, -4, 7,};
    int b_pst_eg[32] = {-18, -35, -27, -36, -13, -18, -16, -15, -9, -19, -14, -9, -6, 2, -2, 0, -8, -1, -5, 6, -3, -15, -7, -8, -11,
                        -28, -25, -14, -24, -36, -42, -17,};
    int r_pst_eg[32] = { 34, 41, 49, 41, 28, 29, 31, 28, 20, 14, 8, 11, 15, 13, 18, 13, 23, 20, 18, 16, 17, 10, 7, 14, 18, 8, 12, 30,
                         16, 17, 20, 12,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {223, 280, 185, 191, 36, 53, 34, 18, 30, 29, 15, 27, 14, 3, 10, 20, -1, -5, 11, -5, -6, 21, 9, -8,};
    int p_pst_eg[24] = {134, 109, 116, 119, 56, 48, 52, 42, 24, 22, 26, 16, 18, 16, 13, 5, 12, 13, 4, 4, 16, 8, 11, 1,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {15, 7, 15, 4, 15, 8, 30, 18, 56, 53, 22, -44, -22, 19, 71, 99, 60, 89, 62, 23, 36, 53, 119, 121,};
    int k_pst_exposed_mg = 81;
    int k_pst_eg[16] = {
            20, 51, 67, 70, 38, 59, 71, 73, 13, 38, 60, 76, -33, 3, 28, 24,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-7, -35}, {-8, -35}, {-9, -26}, {-6, -23}, {4, -19}, {0, -62},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{0, -5}, {-1, -11}, {-4, -17}, {-13, -16}, {-29, 11}, {0, 10},}; // [RANK][OPEN FILE]
    int doubled_mg[2] = {-3, 26,}; // [OPEN FILE]
    int doubled_eg[2] = {-20, -28,}; // [OPEN FILE]
    int blocked_mg[2] = {-14, -16,}; // [OPEN_FILE]
    int blocked_eg[2] = {-3, -1,}; // [OPEN_FILE]
    int chain_mg[5] = {11, 18, 22, 32, 80,}; // [RANK]
    int chain_eg[5] = {13, 0, -3, 23, -12,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-1, 0, -18, -2, 21, -101,}; // [RANK]
    int passed_eg[6] = {5, 11, 42, 70, 111, 140,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-38, -5, 18, 24,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 19;
    int kat_attack_weight[5] = {-3, 13, 8, 8, 10,};
    int kat_defence_weight[5] = {4, 3, 4, 0, -1,};

    int kat_table_scale = 13;
    int kat_table_translate = 32;
    int kat_table_max = 267;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 57;
    int pos_bishop_pair_eg = 48;

    int pos_r_trapped_mg = -89;
    int pos_r_open_file_mg = 18;
    int pos_r_open_file_eg = -23;
    int pos_r_behind_own_passer_mg = 4;
    int pos_r_behind_own_passer_eg = 8;
    int pos_r_behind_enemy_passer_mg = 5;
    int pos_r_behind_enemy_passer_eg = 1;
    int pos_r_xray_pawn_mg = 6;
    int pos_r_xray_pawn_eg = 6;

    // TODO: Square colour weaknesses, mobility, etc.
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
    int kat_table[96] = {};

    eval_params_t params;
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_size);

    ~evaluator_t();

    int evaluate(const board_t &board);

    /// Initialise generic evaluation tables
    static void eval_init();

private:
    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1

    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t *eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);
};

#endif //TOPPLE_EVAL_H
