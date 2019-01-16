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
    int mat_mg[5] = {79, 359, 406, 540, 1173,};
    int mat_eg[5] = {95, 363, 411, 682, 1310,};
    int mat_exchange = 3;

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16] = {
            35, 58, 68, 74, 
            34, 67, 75, 70, 
            6, 13, 53, 47, 
            -89, 24, -4, 15,
    };
    int q_pst_mg[16] = {
            -19, -33, -19, -29, 
            -15, 9, -6, -10, 
            -36, -30, 15, 1, 
            -20, -29, -23, 4,
    };
    int n_pst_eg[16] = {
            -16, 6, 27, 31, 
            -35, -10, 2, 17, 
            -51, -18, -20, -3, 
            -71, -76, -23, -25,
    };
    int q_pst_eg[16] = {
            -16, 30, 14, 47, 
            -20, -30, 6, 17, 
            -34, -21, -31, -9, 
            -37, -18, -18, -29,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -85, -33, -71, -32, 
            -46, -4, -17, -104, 
            -28, 29, 21, -16, 
            5, -15, -17, 0, 
            9, -14, -1, -3, 
            -12, 14, 2, 2, 
            -11, -3, 1, -24, 
            -53, -33, -45, -56,
    };
    int r_pst_mg[32] = {
            32, 21, 10, 29, 
            37, 43, 58, 48, 
            3, 32, 48, 27, 
            11, 14, 23, 25, 
            -26, -21, 5, -4, 
            -36, -10, -4, -27, 
            -36, -1, -13, -47, 
            -8, -3, -3, 3,
    };
    int b_pst_eg[32] = {
            -15, -34, -29, -41, 
            -16, -19, -17, -15, 
            -12, -22, -18, -12, 
            -9, 2, -3, -3, 
            -14, -1, -7, 4, 
            -5, -17, -7, -11, 
            -11, -28, -24, -12, 
            -23, -35, -41, -18,
    };
    int r_pst_eg[32] = {
            36, 32, 38, 34, 
            25, 24, 20, 23, 
            25, 19, 12, 15, 
            16, 18, 18, 13, 
            26, 23, 19, 17, 
            20, 11, 10, 17, 
            20, 5, 12, 35, 
            14, 15, 23, 9,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24] = {
            219, 255, 191, 195, 
            36, 43, 34, 11, 
            30, 29, 14, 25, 
            14, 5, 10, 19, 
            -2, -5, 10, -6, 
            -6, 21, 9, -9,
    };
    int p_pst_eg[24] = {
            137, 113, 118, 120, 
            60, 54, 55, 49, 
            21, 20, 26, 16, 
            18, 14, 13, 5, 
            13, 14, 4, 6,
            18, 7, 13, 2,
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24] = { 
            30, 21, 38, 49, 49, 37, 36, 30, 
            53, 53, 34, -34, -10, 28, 66, 84, 
            58, 95, 62, -18, 47, 12, 113, 113,
    };
    int k_pst_exposed_mg = 120;
    int k_pst_eg[16] = {
            8, 39, 57, 60, 33, 58, 66, 65, 16, 39, 59, 80, -35, 0, 38, 26,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{-8, -35}, {-6, -35}, {-10, -25}, {-4, -25}, {5, -26}, {0, -65},}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{1, -5}, {-3, -11}, {-3, -19}, {-13, -17}, {-33, 11}, {0, 12},}; // [RANK][OPEN FILE]
    int doubled_mg[2] = {-4, 28,}; // [OPEN FILE]
    int doubled_eg[2] = {-19, -28,}; // [OPEN FILE]
    int blocked_mg[2] = {-12, -16,}; // [OPEN_FILE]
    int blocked_eg[2] = {-4, -1}; // [OPEN_FILE]
    int chain_mg[5] = {11, 18, 23, 32, 17,}; // [RANK]
    int chain_eg[5] = {13, -1, -4, 22, 9,}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-1, -2, -18, -3, 21, -102,}; // [RANK]
    int passed_eg[6] = {7, 13, 49, 77, 116, 145,}; // [RANK]

    /// King safety
    int ks_pawn_shield[4] = {-32, -5, 17, 22,}; // 0, 1, 2, and 3 pawns close to the king

    int kat_open_file = 19;
    int kat_attack_weight[5] = {-3, 12, 8, 7, 10,};
    int kat_defence_weight[5] = {4, 3, 4, 0, -2,};

    /// Other positional
    int pos_bishop_pair_mg = 61;
    int pos_bishop_pair_eg = 46;

    int pos_r_trapped_mg = -59;
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
    double eval_material(const board_t &board, int &mg, int &eg); // returns tapering factor 0-1

    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t *eval_pawns(const board_t &board);

    void eval_king_safety(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry);

    void eval_positional(const board_t &board, int &mg, int &eg);
};

#endif //TOPPLE_EVAL_H
