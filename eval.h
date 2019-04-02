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
    int mat_mg[5] = {95, 365, 436, 583, 1325};
    int mat_eg[5] = {101, 369, 389, 645, 1235};
    int mat_exch_scale = -7;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 2;
    int mat_exch_rook = 4;
    int mat_exch_queen = 9;
    int mat_opp_bishop[3] = {76, 62, 46}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            62, 65, 71, 79, 
            56, 75, 70, 68, 
            33, 28, 60, 69, 
            -56, 52, 21, 49
    };
    int q_pst_mg[16] = {
            -24, -49, -40, -54, 
            -16, -2, -19, -26, 
            -29, -56, -2, -7, 
            -21, -15, -13, 6
    };
    int n_pst_eg[16] = {
            -21, -2, 14, 16, 
            -37, -17, -7, 5, 
            -49, -18, -20, -12, 
            -66, -58, -26, -29
    };
    int q_pst_eg[16] = {
            10, 44, 15, 41, 
            -7, -31, -3, 9, 
            -25, 2, -30, -16, 
            -24, -18, -18, -28
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -80, -75, -140, -63, 
            -66, -27, -47, -94, 
            -49, -4, -8, -30, 
            -12, -13, -15, -6, 
            7, -29, -5, -6, 
            -8, 19, 4, 2, 
            -2, 9, 16, -12, 
            -26, -12, -14, -26
    };
    int r_pst_mg[32] = {
            13, -4, -50, 9, 
            -2, 0, 10, 13, 
            -39, -2, 10, -14, 
            -29, -22, -11, -14, 
            -45, -38, -19, -26, 
            -40, -20, -19, -33,
            -37, -15, -31, -49, 
            -8, -3, -14, 7
    };
    int b_pst_eg[32] = {
            -16, -21, -10, -26, 
            -12, -17, -14, -21, 
            -11, -21, -19, -17, 
            -17, -14, -19, -15, 
            -27, -13, -17, -13, 
            -13, -27, -20, -18, 
            -21, -35, -35, -22, 
            -25, -31, -33, -21
    };
    int r_pst_eg[32] = {
            32, 31, 46, 29, 
            21, 24, 22, 18, 
            22, 16, 8, 15, 
            20, 17, 18, 13, 
            23, 22, 20, 15, 
            16, 10, 9, 10, 
            16, 8, 14, 25,
            14, 14, 17, 2
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            127, 196, 78, 77, 
            43, 59, 38, 22, 
            15, 10, -4, 14, 
            13, -2, 6, 16,
            9, -3, 11, 5, 
            9, 22, 15, 9
    };
    int p_pst_eg[24] = {
            111, 86, 102, 103, 
            54, 46, 49, 38, 
            30, 29, 32, 19, 
            18, 16, 12, 6, 
            6, 10, 2, -3, 
            10, 7, 8, -8
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {
            -6, -5, -5, -1, 3, -3, 18, -1, 
            52, 48, 16, -35, -8, 10, 65, 88, 
            36, 84, 57, 17, 42, 52, 114, 95
    };
    int k_pst_exposed_mg = 48;
    int k_pst_eg[16] = {
            22, 54, 68, 70, 
            37, 59, 71, 72, 
            6, 36, 61, 71, 
            -37, -7, 24, 24,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-13, -32}, {-19, -37}, {-8, -27}, {1, -14}, {5, 6}, {-509, -33}}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{6, -2}, {6, -4}, {-2, -13}, {-15, -18}, {-23, 1}, {607, -1}}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-2, -23}; // [OPEN FILE]
    int backwards_eg[2] = {-9, -13}; // [OPEN FILE]
    int doubled_mg[2] = {-8, 13}; // [OPEN FILE]
    int doubled_eg[2] = {-28, -32}; // [OPEN FILE]
    int blocked_mg = -18; // [OPEN_FILE]
    int blocked_eg = 3; // [OPEN_FILE]
    int chain_mg[5] = {9, 13, 21, 49, 245}; // [RANK]
    int chain_eg[5] = {17, 6, 0, 15, -69}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-8, -3, -15, 7, -3, -9}; // [RANK]
    int passed_eg[6] = {6, 9, 40, 66, 121, 159}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-22, 1, 18, 24}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 13;
    int kat_own_half_open_file = 10;
    int kat_other_half_open_file = 12;
    int kat_attack_weight[5] = {-2, 10, 4, 3, 7};
    int kat_defence_weight[5] = {6, 3, 3, 1, 0};

    int kat_table_scale = 18;
    int kat_table_translate = 38;
    int kat_table_max = 416;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 37;
    int pos_bishop_pair_eg = 65;

    int pos_r_trapped_mg = -68;
    int pos_r_behind_own_passer_eg = 15;
    int pos_r_behind_enemy_passer_eg = 3;
    int pos_r_xray_pawn_eg = 15;

    int pos_r_open_file_mg = 76;
    int pos_r_open_file_eg = 3;
    int pos_r_own_half_open_file_mg = 17;
    int pos_r_own_half_open_file_eg = 9;
    int pos_r_other_half_open_file_mg = 34;
    int pos_r_other_half_open_file_eg = 8;

    int pos_mob_mg[4] = {10, 11, 6, 6}; // [PIECE, ¬KING, ¬PAWN]
    int pos_mob_eg[4] = {1, 3, 3, 5};

    // TODO: Square colour weaknesses, mobility, etc.
};

class evaluator_t {
    struct pawn_entry_t {
        U64 hash;

        int32_t eval_mg;
        int32_t eval_eg;

        U64 passers[2];
        U64 defended[2];
        U64 half_open_files[2]; // [side with the pawn]
        U64 open_files;
        U64 advance_squares[2];
        U64 attackable[2];
    };

    size_t pawn_hash_entries;
    pawn_entry_t *pawn_hash_table;

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    int kat_table[96] = {};

    eval_params_t params;
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_size);
    evaluator_t(const evaluator_t&) = delete;
    evaluator_t& operator=(const evaluator_t &) = delete;
    ~evaluator_t();

    int evaluate(const board_t &board);
    void prefetch(U64 pawn_hash);

    /// Initialise generic evaluation tables
    static void eval_init();

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    pawn_entry_t eval_pawns(const board_t &board);

    void eval_pst(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);

    void eval_movement(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t &entry);
};

#endif //TOPPLE_EVAL_H
