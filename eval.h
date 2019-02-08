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
    int mat_mg[5] = {89, 390, 462, 590, 1261,};
    int mat_eg[5] = {109, 377, 420, 692, 1317,};
    int mat_exch_scale = 10;
    int mat_exch_pawn = 0;
    int mat_exch_minor = 4;
    int mat_exch_rook = 6;
    int mat_exch_queen = 19;
    int mat_opp_bishop_only_eg[3] = {124, 132, 16,}; // [PAWN ADVANTAGE]
    int mat_opp_bishop_eg[3] = {52, 37, 47,}; // [PAWN ADVANTAGE]

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {47, 71, 82, 88, 44, 80, 90, 89, 14, 18, 65, 60, -94, 32, 1, 31,
    };
    int q_pst_mg[16] = {-22, -39, -21, -31, -13, 11, -6, -15, -37, -43, 14, -3, -23, -31, -25, 3,
    };
    int n_pst_eg[16] = {-14, 8, 32, 34, -31, -5, 8, 18, -50, -14, -14, 0, -79, -64, -22, -27,
    };
    int q_pst_eg[16] = {-11, 44, 17, 54, -34, -43, 1, 16, -40, -10, -50, -14, -48, -23, -25, -47,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {-91, -58, -125, -62, -57, -6, -21, -100, -35, 21, 17, -16, 8, -11, -12, -1, 16, -7, 5, 2, -6, 19, 13, 7, -7,
                        3, 7, -17, -49, -31, -41, -53,
    };
    int r_pst_mg[32] = {50, 17, -18, 49, 6, 5, 11, 33, -36, -1, 11, -7, -20, -18, -1, -2, -36, -34, -6, -15, -40, -15, -11, -33, -40,
                        -9, -20, -53, -11, -6, -7, 12,};
    int b_pst_eg[32] = {-16, -25, -18, -26, -12, -14, -12, -19, -6, -10, -7, -9, -5, 1, -2, 2, -11, -4, -7, 5, -4, -13, -8, -9, -12,
                        -27, -26, -15, -26, -37, -44, -21,};
    int r_pst_eg[32] = {32, 37, 46, 31, 27, 31, 30, 25, 24, 20, 14, 15, 19, 20, 19, 14, 23, 21, 19, 16, 17, 11, 12, 13, 18, 12, 15, 29, 20, 22, 25, 9,};
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {186, 244, 124, 129, 57, 71, 51, 26, 35, 29, 17, 32, 17, 3, 10, 22, 1, -6, 12, -1, -4, 21, 10, -6,};
    int p_pst_eg[24] = {156, 130, 143, 147, 63, 54, 59, 49, 31, 29, 32, 20, 22, 19, 16, 9, 14, 15, 7, 4, 19, 13, 15, 2,};

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = {-16, -4, 6, 7, 6, 3, 23, -11, 47, 57, 24, -51, -19, 19, 79, 93, 43, 92, 62, 22, 38, 57, 123, 107,};
    int k_pst_exposed_mg = 44;
    int k_pst_eg[16] = {
            25, 62, 80, 82, 41, 64, 79, 81, 10, 38, 65, 80, -41, -4, 25, 22,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-11, -36}, {-15, -35}, {-11, -22}, {-11, -20}, {-6, 11}, {0, -21},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{3, -4}, {2, -9}, {-4, -16}, {-15, -16}, {-26, 1}, {0, -13},}; // [RANK][OPEN FILE]
    int backwards_mg[2] = {-4, -25,}; // [OPEN FILE]
    int backwards_eg[2] = {-9, -14,}; // [OPEN FILE]
    int doubled_mg[2] = {-4, 18,}; // [OPEN FILE]
    int doubled_eg[2] = {-23, -31,}; // [OPEN FILE]
    int blocked_mg[2] = {-15, -17,}; // [OPEN_FILE]
    int blocked_eg[2] = {-5, 2,}; // [OPEN_FILE]
    int chain_mg[5] = {11, 20, 24, 37, 172,}; // [RANK]
    int chain_eg[5] = {17, 6, 3, 28, -42,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {2, 0, -13, 7, 10, -40,}; // [RANK]
    int passed_eg[6] = {7, 14, 46, 75, 127, 154,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-36, -5, 18, 21,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 8;
    int kat_attack_weight[5] = {-1, 10, 6, 6, 8,};
    int kat_defence_weight[5] = {5, 2, 3, 1, 1,};

    int kat_table_scale = 18;
    int kat_table_translate = 32;
    int kat_table_max = 406;
    int kat_table_offset = 10;

    /// Other positional
    int pos_bishop_pair_mg = 47;
    int pos_bishop_pair_eg = 65;

    int pos_r_trapped_mg = -94;
    int pos_r_open_file_mg = 33;
    int pos_r_open_file_eg = 13;
    int pos_r_behind_own_passer_mg = 2;
    int pos_r_behind_own_passer_eg = 3;
    int pos_r_behind_enemy_passer_mg = 1;
    int pos_r_behind_enemy_passer_eg = 3;
    int pos_r_xray_pawn_mg = 12;
    int pos_r_xray_pawn_eg = 10;

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

    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1
private:
    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t *eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);

    void eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);
};

#endif //TOPPLE_EVAL_H
