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
    int mat_mg[5] = {103, 279, 338, 495, 1040};
    int mat_eg[5] = {72, 314, 340, 504, 985};

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16]{-11, 25, 49, 45,
                     -33, 10, 64, 38,
                     -34, -7, 6, -7,
                     -101, -27, -29, -31
    };
    int q_pst_mg[16]{
        -19, 3, 3, 18,
        -3, 2, 26, 10,
        -31, -8, 4, 7,
        -1, -37, -22, 4
    };
    int n_pst_eg[16]{
            -24, 1, 21, 19,
            -33, -3, 11, 22,
            14, -14, 8, 7,
            -98, -27, -23, -15
    };
    int q_pst_eg[16]{
            -31, -4, -13, -6,
            -16, -16, 2, -13,
            -28, -17, -14, -10,
            -22, -38, -27, -5
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -19, -39, -40, -41,
            -43, -36, -11, -9,
            -61, 9, 0, -9,
            -4, -2, -5, 0,
            0, -7, 12, 0,
            0, 8, 7, 0,
            20, -5, -35, -26,
            49, -40, -13, -114,
    };
    int r_pst_mg[32]{
            -54, 13, 17, 16,
            30, 21, 31, 35,
            28, 14, 20, 17,
            27, 9, 8, 7,
            13, -2, 2, -1,
            6, -2, -1, -3,
            -5, -10, -6, -12,
            -1, -27, -13, 3,
    };
    int b_pst_eg[32]{
            -8, -17, -14, -45,
            -32, 0, 5, 5,
            -55, 22, 31, 2,
            10, -1, -9, 5,
            6, -1, 15, 2,
            6, 10, 13, 12,
            -7, -7, -19, -1,
            -2, -22, -7, -20,
    };
    int r_pst_eg[32]{
        -13, 33, 35, 28,
        22, 31, 32, 28,
        0, 26, 24, 7,
        27, 32, 14, 6,
        14, 1, -8, -6,
        9, -1, -4, -15,
        -8, -21, -15, -14,
        -8, -11, -7, -3,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24]{
        -7, 161, 183, 187,
        194, 93, 106, 90,
        132, 12, 10, 16,
        20, -11, -19, -24,
        13, -9, -3, -15,
        35, 15, 43, 32,
    };
    int p_pst_eg[24]{
            -110, 36, 36, 39,
            22, 3, 25, 29,
            -13, 13, 2, 8,
            14, 3, -5, -4,
            12, -8, 2, -6,
            13, 1, -1, 3,
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24]{
            53, 103, 79, 60, 60, 79, 103, 89,
            114, 128, 104, 85, 85, 104, 128, 114,
            119, 140, 139, 60, 90, 60, 140, 119,
    };
    int k_pst_exposed_mg = 50;
    int k_pst_eg[16]{
            54, 75, 86, 89,
            44, 66, 76, 80,
            29, 51, 62, 65,
            10, 32, 43, 46,
    };

    /// Pawn structure
    // TODO: Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    // TODO: Passed pawns

    /// Mobility
    // TODO: Restrict by pawn attacks?

    /// King safety
    // TODO: King safety

    /// Other positional
    // TODO: Bishop pair, square colour weaknesses, rook behind passed pawn, etc.
};

class evaluator_t {
    size_t pawn_hash_entries;
    static constexpr size_t bucket_size = 4;
    struct pawn_entry_t {
        U64 hash;
        uint32_t hits;
        uint32_t eval_mg;
        uint32_t eval_eg;
    } *pawn_hash_table = nullptr;

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    int mat[5][2] = {};
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_size);

    ~evaluator_t();

    int evaluate(const board_t &board);

    /// Initialise generic evaluation tables
    static void eval_init();
private:
    void eval_material(const board_t &board, int &mg, int &eg);

    void eval_pst(const board_t &board, int &mg, int &eg);

    pawn_entry_t* eval_pawns(const board_t &board);
};

#endif //TOPPLE_EVAL_H
