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
    int mat_mg[5] = {87, 249, 304, 534, 1018};
    int mat_eg[5] = {70, 349, 413, 510, 1076};

    /// Piece-square tables

    // PST for N, Q only:
    // A4, B4, C4, D4
    // A3, B3, C3, D3
    // A2, B2, C2, D2
    // A1, B1, C1, D1
    // Horizontal, Vertical and diagonal symmetry
    int n_pst_mg[16]{
            31, 47, 54, 65,
            10, 21, 44, 39,
            -29, -4, 3, 23,
            -21, -45, -1, 7,
    };
    int q_pst_mg[16]{
            5, 21, 23, 37,
            -12, 1, 27, 25,
            -10, 2, 1, 11,
            -12, -12, -7, -29,
    };
    int n_pst_eg[16]{
            -1, 20, 36, 37,
            2, 28, 35, 49,
            1, 7, 41, 21,
            -125, 0, -8, -8,
    };
    int q_pst_eg[16]{
            -25, -17, -17, -18,
            -4, -2, -8, -11,
            -29, -30, -5, -8,
            -16, -33, -27, -5,
    };

    // PST for pawn, rook:
    // A8, B8, C8, D8, A7, B7, C7, D7, etc.
    // Only vertical symmetry
    int b_pst_mg[32] = {
            -19, -16, -35, -20,
            -26, -41, -17, -28,
            2, -13, -20, -4,
            -13, -6, 2, 1,
            -2, 2, -7, 1,
            -9, -14, -13, -6,
            3, -13, -15, -26,
            -12, -21, -85, -52,
    };
    int r_pst_mg[32]{
            -26, 9, 16, 16,
            21, -5, 6, 20,
            14, -4, 3, 4,
            7, -5, 6, 8,
            8, 3, 11, 12,
            11, 1, -1, 6,
            4, 14, 15, 6,
            10, 11, 19, 6,
    };
    int b_pst_eg[32]{
            2, -49, -14, -50,
            -48, 3, -7, 7,
            -54, 23, 37, 10,
            -2, 4, 0, -11,
            4, -5, 8, 2,
            15, 11, 13, 12,
            5, -4, 13, 16,
            0, -22, -6, -18,
    };
    int r_pst_eg[32]{
            -23, 47, 38, 42,
            36, 68, 54, 22,
            23, 47, 34, 41,
            36, 43, 26, 11,
            6, 3, -8, -14,
            -8, 0, 2, -14,
            -18, -25, -13, -16,
            -29, -1, -1, -4,
    };
    // Pawns have first and eighth rank excluded
    int p_pst_mg[24]{
            -8, 176, 200, 191,
            193, 82, 87, 78,
            95, 3, 17, 13,
            1, -6, 4, 8,
            -3, 3, 6, 14,
            -5, -3, 25, 17,
    };
    int p_pst_eg[24]{
            4, 4, -11, 3,
            15, -7, 8, 16,
            -5, 13, 0, 7,
            22, 11, 4, -10,
            13, 2, 9, -11,
            10, 12, 14, 15,
    };

    // PST for king:
    // A3, B3, C3, D3, E3, F3, G3, H3, A2, B2, C2, etc.
    // mg: First 3 ranks only. Default to king_pst_exposed_mg for higher ranks.
    // eg: Horizontal, vertical and diagonal symmetry, like N, B, Q
    int k_pst_mg[24]{
            8, 70, 88, 104,
            108, 103, 90, 77,
            53, 74, 100, 119,
            127, 110, 101, 79,
            25, 38, 82, 38,
            40, 71, 71, 46,
    };
    int k_pst_exposed_mg = 89;
    int k_pst_eg[16]{
            -19, 66, 103, 108,
            39, 65, 55, 36,
            49, 47, 21, -19,
            53, 78, 22, 36,
    };

    /// Pawn structure

    // Pawn structure: doubled, isolated, backwards, chain, protected, etc.
    int isolated_mg[6][2] = {{14,  5},
                             {11,  -1},
                             {15,  -2},
                             {3,   -2},
                             {-72, 10},
                             {0,   1}}; // [RANK][OPEN FILE]
    int isolated_eg[6][2] = {{-9,  -38},
                             {-15, -39},
                             {-18, -22},
                             {-6,  -10},
                             {28,  0},
                             {0,   -4}}; // [RANK][OPEN FILE]
    int doubled_mg[2] = {-21, -28}; // [OPEN FILE]
    int doubled_eg[2] = {3, 17}; // [OPEN FILE]
    int blocked_mg[2] = {-5, 2}; // [OPEN_FILE]
    int blocked_eg[2] = {-8, -8}; // [OPEN_FILE]
    int chain_mg[5] = {19, 9, 13, 15, -27}; // [RANK]
    int chain_eg[5] = {5, 10, 17, 20, 52}; // [RANK]
    // Passed pawns
    int passed_mg[6] = {-6, 5, 24, 50, 48, 21}; // [RANK]
    int passed_eg[6] = {-10, -4, -4, 2, -2, 15}; // [RANK]

    /// Mobility

    // "Span" mobility: total number of piece moves (not pawn) of any type, up to a maximum of 32
    int mob_span_mg[32] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
    };
    int mob_span_eg[32] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
    };

    // Individual piece mobility, indexed by number of possible moves, excluding squares defended by enemy pawns
    int n_mob_mg[9] = {-37, -33, -4, -1, 3, 7, 11, 15, 18};
    int b_mob_mg[15] = {-28, -24, -15, 8, 13, 18, 25, 27, 31, 32,
                        35, 39, 40, 46, 48};
    int r_mob_mg[15] = {-24, -15, 8, 13, 18, 25, 27, 31, 32,
                        35, 39, 40, 46, 48, 50};
    int q_mob_mg[29] = {-20, -20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    int n_mob_eg[9] = {-37, -33, -4, -1, 3, 7, 11, 15, 18};
    int b_mob_eg[15] = {-28, -24, -15, 8, 13, 18, 25, 27, 31, 32,
                        35, 39, 40, 46, 48};
    int r_mob_eg[15] = {-24, -15, 8, 13, 18, 25, 27, 31, 32,
                        35, 39, 40, 46, 48, 50};
    int q_mob_eg[29] = {-20, -20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /// King safety
    int ks_pawn_shield[4] = {-10, -1, 10, 20}; // 0, 1, 2, and 3 pawns close to the king

    int kat_attack_value[5] = {1, 2, 2, 3, 5};
    int kat_defend_value[5] = {-1, -2, -3, -4, -6};
    int kat_open_file = 10; // only if opponent has rooks

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

    void eval_moves(const board_t &board, int &mg, int &eg,
                    const pawn_entry_t *entry, int &king_danger_w, int &king_danger_b);

    void eval_king_safety(const board_t &board, int &mg, int &eg,
                          const pawn_entry_t *entry, int &king_danger_w, int &king_danger_b);
};

#endif //TOPPLE_EVAL_H
