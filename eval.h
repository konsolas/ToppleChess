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
    int mat_mg[5] = {100, 300, 300, 500, 900};
    int mat_eg[5] = {100, 300, 300, 500, 900};

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16]{
            -11, 6, 16, 21,
            -33, -1, 0, 4,
            -35, -17, -10, -8,
            -57, -27, -34, -29
    };
    int q_pst_mg[16]{
            -1, 3, 3, 3,
            -1, 3, 3, 3,
            -1, 4, 4, 4,
            -1, 4, 4, 4
    };
    int n_pst_eg[16]{
            -16, -10, 2, 16,
            -21, -15, -2, 11,
            -27, -21, -8, 5,
            -39, -33, -20, -6
    };
    int q_pst_eg[16]{
            -12, -2, 2, 7,
            -16, -7, -2, 2,
            -21, -12, -7, -2,
            -32, -21, -16, -12
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -19, -8, -11, -15,
            -12, 2, 0, -4,
            -11, 0, 0, -2,
            -8, 6, 3, 0,
            0, 7, 4, 0,
            -8, 8, 4, 0,
            10, 3, 0, -3,
            -21, -10, -13, -17
    };
    int r_pst_mg[32]{
            -8, -6, -4, -3,
            16, 18, 19, 19,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -1, 1, 3,
            -8, -2, 0, 3,
            -8, -6, -4, 5,
    };
    int b_pst_eg[32]{
            -26, -16, -17, -10,
            -17, -8, -8, -1,
            -13, -4, -4, 2,
            -14, -4, -5, 1,
            -14, -4, -5, 1,
            -13, -4, -4, 2,
            -17, -8, -8, -1,
            -26, -16, -17, -10,
    };
    int r_pst_eg[32]{
            -8, -6, -4, -3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -2, 0, 3,
            -8, -6, -4, -3,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24]{
            -8, 0, 0, 0,
            -8, 0, 0, 0,
            -8, 0, 6, 10,
            -8, 0, 14, 18,
            -8, 0, 7, 7,
            0, 0, -5, -9,
    };
    int p_pst_eg[24]{
            20, 25, 30, 35,
            9, 15, 20, 26,
            5, 10, 15, 19,
            1, 5, 9, 15,
            -1, -1, -1, -1,
            -6, -6, -6, -6,
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24]{
            89, 103, 79, 60, 60, 79, 103, 89,
            114, 128, 104, 85, 85, 104, 128, 114,
            119, 140, 139, 60, 90, 60, 140, 119
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
    struct pawn_entry_t {
        int partial_eval;
        U64 holes[2];
        U64 defended_by[2];
    } *pawn_hash_table = nullptr;

    int pst[2][6][64][2] = {}; // [TEAM][PIECE][SQUARE][MG/EG]
    int mat[5][2] = {};
public:
    evaluator_t(eval_params_t params, size_t pawn_hash_entries);

    ~evaluator_t();

    int evaluate(const board_t &board) const;

    /// Initialise generic evaluation tables
    static void eval_init();

private:
    void eval_material(const board_t &board, int &mg, int &eg) const;

    void eval_pst(const board_t &board, int &mg, int &eg) const;

    void eval_pawns(const board_t &board, int &mg, int &eg) const;
};

#endif //TOPPLE_EVAL_H
