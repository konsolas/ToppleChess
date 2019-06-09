//
// Created by Vincent on 30/09/2017.
//

#include "eval.h"
#include "board.h"
#include "endgame.h"
#include "hash.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utility tables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

U64 BB_PASSED[2][64] = {};
U64 BB_ISOLATED[2][64] = {};
U64 BB_IN_FRONT[2][64] = {};
U64 BB_HOLE[2][64] = {};
U64 BB_BACKWARDS[2][64] = {};

U64 BB_KING_CIRCLE[64] = {};

U64 BB_CENTRE = 0;

void evaluator_t::eval_init() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        BB_HOLE[WHITE][sq] = 0xffffffffffffffff;

        for (uint8_t rank = rank_index(sq) + uint8_t(1); rank < 8; rank++) {
            // Passed
            if (file_index(sq) > 0) BB_PASSED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) - 1), rank));
            if (file_index(sq) < 7) BB_PASSED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) + 1), rank));
            BB_PASSED[WHITE][sq] |= single_bit(square_index(file_index(sq), rank));

            // Candidate
            BB_IN_FRONT[WHITE][sq] |= single_bit(square_index(file_index(sq), rank));
        }

        for(uint8_t rank = 0; rank <= rank_index(sq); rank++) {
            // Backward
            if (file_index(sq) > 0) BB_BACKWARDS[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) - 1), rank));
            if (file_index(sq) < 7) BB_BACKWARDS[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) + 1), rank));
            if(rank < rank_index(sq)) {
                BB_BACKWARDS[WHITE][sq] |= single_bit(square_index(file_index(sq), rank));
            }
        }

        // Hole
        BB_HOLE[WHITE][sq] ^= BB_PASSED[WHITE][sq];
        BB_HOLE[WHITE][sq] ^= BB_IN_FRONT[WHITE][sq];

        // Isolated
        for (uint8_t rank = 0; rank < 8; rank++) {
            if (file_index(sq) > 0)
                BB_ISOLATED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) - 1), rank));
            if (file_index(sq) < 7)
                BB_ISOLATED[WHITE][sq] |= single_bit(square_index(uint8_t(file_index(sq) + 1), rank));
        }

        // Centre
        if (file_index(sq) <= 4 && file_index(sq) >= 3 && rank_index(sq) <= 4 && rank_index(sq) >= 3) {
            BB_CENTRE |= single_bit(sq);
        }

        // King circle
        BB_KING_CIRCLE[sq] |= find_moves<KING>(WHITE, sq, 0);
        BB_KING_CIRCLE[sq] |= find_moves<KNIGHT>(WHITE, sq, 0);
    }

    // Mirror
    for (uint8_t i = 0; i < 64; i++) {
        // Tables
        // ... none yet

        for (uint8_t square = 0; square < 64; square++) {
            if (BB_PASSED[WHITE][i] & single_bit(square))
                BB_PASSED[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_ISOLATED[WHITE][i] & single_bit(square))
                BB_ISOLATED[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_IN_FRONT[WHITE][i] & single_bit(square))
                BB_IN_FRONT[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_HOLE[WHITE][i] & single_bit(square))
                BB_HOLE[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
            if (BB_BACKWARDS[WHITE][i] & single_bit(square))
                BB_BACKWARDS[BLACK][MIRROR_TABLE[i]] |= single_bit(MIRROR_TABLE[square]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Preprocessing evaluation parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

processed_params_t::processed_params_t(const eval_params_t &params)
        : eval_params_t(params) {
    // Four-way mirrored tables
    constexpr uint8_t square_mapping[16][4] = {{A4, A5, H4, H5},
                                               {B4, B5, G4, G5},
                                               {C4, C5, F4, F5},
                                               {D4, D5, E4, E5},
                                               {A3, A6, H3, H6},
                                               {B3, B6, G3, G6},
                                               {C3, C6, F3, F6},
                                               {D3, D6, E3, E6},
                                               {A2, A7, H2, H7},
                                               {B2, B7, G2, G7},
                                               {C2, C7, F2, F7},
                                               {D2, D7, E2, E7},
                                               {A1, A8, H1, H8},
                                               {B1, B8, G1, G8},
                                               {C1, C8, F1, F8},
                                               {D1, D8, E1, E8}};
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 4; j++) {
            pst[WHITE][KNIGHT][square_mapping[i][j]][MG] = params.n_pst_mg[i];
            pst[WHITE][KNIGHT][square_mapping[i][j]][EG] = params.n_pst_eg[i];
            pst[WHITE][QUEEN][square_mapping[i][j]][MG] = params.q_pst_mg[i];
            pst[WHITE][QUEEN][square_mapping[i][j]][EG] = params.q_pst_eg[i];
            pst[WHITE][KING][square_mapping[i][j]][MG] = params.k_pst_mg[i];
            pst[WHITE][KING][square_mapping[i][j]][EG] = params.k_pst_eg[i];
        }
    }

    // Vertically mirrored tables
    for (uint8_t sq = 0; sq < 64; sq++) {
        int param_index = 32 - (4 - (file_index(sq) % 4) + rank_index(sq) * 4);

        if (sq >= 8 && sq < 56) {
            pst[WHITE][PAWN][sq][MG] = params.p_pst_mg[param_index - 4];
            pst[WHITE][PAWN][sq][EG] = params.p_pst_eg[param_index - 4];
        }
        pst[WHITE][BISHOP][sq][MG] = params.b_pst_mg[param_index];
        pst[WHITE][BISHOP][sq][EG] = params.b_pst_eg[param_index];
        pst[WHITE][ROOK][sq][MG] = params.r_pst_mg[param_index];
        pst[WHITE][ROOK][sq][EG] = params.r_pst_eg[param_index];
    }

    // Mirror PST for black
    for (int piece = 0; piece < 6; piece++) {
        for (int square = 0; square < 64; square++) {
            pst[BLACK][piece][MIRROR_TABLE[square]][MG] = pst[WHITE][piece][square][MG];
            pst[BLACK][piece][MIRROR_TABLE[square]][EG] = pst[WHITE][piece][square][EG];
        }
    }

    // Set up king safety evaluation table.
    for(int i = 0; i < 128; i++) {
        // Translated + scaled sigmoid function
        kat_table[i] = (int) (double(params.kat_table_max) /
                              (1 + exp((params.kat_table_translate - i) / double(params.kat_table_scale)))) - params.kat_table_offset;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

evaluator_t::evaluator_t(const processed_params_t &params, size_t pawn_hash_size) : params(params) {
    // Set up pawn hash table
    pawn_hash_size /= sizeof(pawns::structure_t);
    this->pawn_hash_entries = tt::lower_power_of_2(pawn_hash_size) - 1;
    pawn_hash_table = std::vector<pawns::structure_t>(pawn_hash_entries + 1);
}

void evaluator_t::prefetch(U64 pawn_hash) {
    size_t index = (pawn_hash & pawn_hash_entries);
    pawns::structure_t *bucket = pawn_hash_table.data() + index;

#if defined(__GNUC__)
    __builtin_prefetch(bucket);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
    _mm_prefetch((char*) bucket, _MM_HINT_T0);
#endif
}

int evaluator_t::evaluate(const board_t &board) {
    // Check endgame
    /*
    eg_eval_t eg_eval = eval_eg(board);
    if (eg_eval.valid) {
        return board.record[board.now].next_move ? -eg_eval.eval : +eg_eval.eval;
    }
    */

    // Middlegame and endgame scores
    int mg = 0;
    int eg = 0;

    // Main evaluation functions
    double phase = eval_material(board, mg, eg);
    eval_pawns(board, mg, eg);
    eval_pst(board, mg, eg);
    eval_movement(board, mg, eg);
    eval_positional(board, mg, eg);

    // Interpolate between middlegame and endgame scores
    auto total = int(phase * mg + (1 - phase) * eg);

    return board.record[board.now].next_move ? -total : total;
}

double evaluator_t::eval_material(const board_t &board, int &mg, int &eg) {
    material_data_t material = board.record[board.now].material;

    // Check for opposite coloured bishops
    bool opposite_coloured_bishops = board.bb_pieces[WHITE][BISHOP] && board.bb_pieces[BLACK][BISHOP]
        && !multiple_bits(board.bb_pieces[WHITE][BISHOP]) && !multiple_bits(board.bb_pieces[BLACK][BISHOP])
        && !same_colour(bit_scan(board.bb_pieces[WHITE][BISHOP]), bit_scan(board.bb_pieces[BLACK][BISHOP]));

    // Collect material data
    const int pawn_balance = material.info.w_pawns - material.info.b_pawns;
    mg += params.mat_mg[PAWN] * pawn_balance;
    eg += params.mat_eg[PAWN] * pawn_balance;

    if(opposite_coloured_bishops) {
        if(pawn_balance > 0) {
            eg -= params.mat_opp_bishop[std::min(2, pawn_balance - 1)];
        } else if(pawn_balance < 0) {
            eg += params.mat_opp_bishop[std::min(2, -pawn_balance - 1)];
        }
    }

    const int knight_balance = material.info.w_knights - material.info.b_knights;
    mg += params.mat_mg[KNIGHT] * knight_balance;
    eg += params.mat_eg[KNIGHT] * knight_balance;

    const int bishop_balance = material.info.w_bishops - material.info.b_bishops;
    mg += params.mat_mg[BISHOP] * bishop_balance;
    eg += params.mat_eg[BISHOP] * bishop_balance;

    const int rook_balance = material.info.w_rooks - material.info.b_rooks;
    mg += params.mat_mg[ROOK] * rook_balance;
    eg += params.mat_eg[ROOK] * rook_balance;

    const int queen_balance = material.info.w_queens - material.info.b_queens;
    mg += params.mat_mg[QUEEN] * queen_balance;
    eg += params.mat_eg[QUEEN] * queen_balance;

    const int mat_w = params.mat_exch_pawn * (material.info.w_pawns)
                      + params.mat_exch_minor * (material.info.w_knights + material.info.w_bishops)
                      + params.mat_exch_rook * (material.info.w_rooks)
                      + params.mat_exch_queen * (material.info.w_queens);
    const int mat_b = params.mat_exch_pawn * (material.info.b_pawns)
                      + params.mat_exch_minor * (material.info.b_knights + material.info.b_bishops)
                      + params.mat_exch_rook * (material.info.b_rooks)
                      + params.mat_exch_queen * (material.info.b_queens);
    const int mat_max = 2 * (params.mat_exch_pawn * 8
                             + params.mat_exch_minor * (2 + 2)
                             + params.mat_exch_rook * 2
                             + params.mat_exch_queen * 1);
    const int mat_total = mat_w + mat_b;

    // Aim to exchange while up material
    eg += params.mat_exch_scale * (mat_w - mat_b);

    // Calculate tapering (game phase)
    // Close to 1 at the start, close to 0 at the end
    return std::min((double(mat_total) / double(mat_max)), 1.0);
}

void evaluator_t::eval_pst(const board_t &board, int &mg, int &eg) {
    U64 pieces;
    for (int type = KNIGHT; type < KING; type++) {
        pieces = board.bb_pieces[WHITE][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg += params.pst[WHITE][type][sq][MG];
            eg += params.pst[WHITE][type][sq][EG];
        }

        pieces = board.bb_pieces[BLACK][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg -= params.pst[BLACK][type][sq][MG];
            eg -= params.pst[BLACK][type][sq][EG];
        }
    }

    pieces = board.bb_pieces[WHITE][KING];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);
        mg += params.pst[WHITE][KING][sq][MG];
        eg += params.pst[WHITE][KING][sq][EG];
    }

    pieces = board.bb_pieces[BLACK][KING];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);
        mg -= params.pst[BLACK][KING][sq][MG];
        eg -= params.pst[BLACK][KING][sq][EG];
    }
}

void evaluator_t::eval_pawns(const board_t &board, int &mg, int &eg) {
    U64 pawn_hash = board.record[board.now].pawn_hash;
    size_t index = (pawn_hash & pawn_hash_entries);
    pawns::structure_t *entry = pawn_hash_table.data() + index;
    if(entry->get_hash() != pawn_hash) {
        *entry = pawns::structure_t(params, pawn_hash,
                                    board.bb_pieces[WHITE][PAWN], board.bb_pieces[BLACK][PAWN]);
    }

    mg += entry->get_eval_mg();
    eg += entry->get_eval_eg();

    entry->eval_dynamic(params, board, mg, eg);
}

void evaluator_t::eval_movement(const board_t &board, int &mg, int &eg) {
    int king_pos[2] = {bit_scan(board.bb_pieces[WHITE][KING]), bit_scan(board.bb_pieces[BLACK][KING])};
    U64 king_circle[2] = {BB_KING_CIRCLE[king_pos[WHITE]], BB_KING_CIRCLE[king_pos[BLACK]]};

    int pawn_shield_w = std::min(3, pop_count(king_circle[WHITE] & board.bb_pieces[WHITE][PAWN]));
    int pawn_shield_b = std::min(3, pop_count(king_circle[BLACK] & board.bb_pieces[BLACK][PAWN]));

    U64 open_files = pawns::open_files(board.bb_pieces[WHITE][PAWN], board.bb_pieces[BLACK][PAWN]);
    U64 half_open_files[2] = {
        pawns::half_or_open_files(board.bb_pieces[WHITE][PAWN]) ^ open_files,
        pawns::half_or_open_files(board.bb_pieces[BLACK][PAWN]) ^ open_files
    };
    U64 defended[2] = {
            pawns::attacks<WHITE>(board.bb_pieces[WHITE][PAWN]),
            pawns::attacks<BLACK>(board.bb_pieces[BLACK][PAWN])
    };
    U64 undefended_pawns[2] = {
            board.bb_pieces[WHITE][PAWN] & ~defended[WHITE],
            board.bb_pieces[BLACK][PAWN] & ~defended[BLACK]
    };
    U64 passed[2] = {
            pawns::passed<WHITE>(board.bb_pieces[WHITE][PAWN], board.bb_pieces[BLACK][PAWN]),
            pawns::passed<BLACK>(board.bb_pieces[BLACK][PAWN], board.bb_pieces[WHITE][PAWN])
    };
    U64 passed_rear[2] = {
            pawns::rear_span<WHITE>(passed[WHITE]),
            pawns::rear_span<BLACK>(passed[BLACK])
    };

    mg += params.ks_pawn_shield[pawn_shield_w];
    mg -= params.ks_pawn_shield[pawn_shield_b];

    // Check for attacks on the king
    int king_danger[2] = {};

    if(board.bb_pieces[BLACK][ROOK] != 0) {
        if(king_circle[WHITE] & open_files) {
            king_danger[WHITE] += params.kat_open_file;
        } else if(king_circle[WHITE] & half_open_files[WHITE]) {
            king_danger[WHITE] += params.kat_own_half_open_file;
        } else if(king_circle[WHITE] & half_open_files[BLACK]) {
            king_danger[WHITE] += params.kat_other_half_open_file;
        }
    }
    if(board.bb_pieces[WHITE][ROOK] != 0) {
        if(king_circle[BLACK] & open_files) {
            king_danger[BLACK] += params.kat_open_file;
        } else if(king_circle[BLACK] & half_open_files[BLACK]) {
            king_danger[BLACK] += params.kat_own_half_open_file;
        } else if(king_circle[BLACK] & half_open_files[WHITE]) {
            king_danger[BLACK] += params.kat_other_half_open_file;
        }
    }

    king_danger[WHITE] -= params.kat_defence_weight[PAWN] * pawn_shield_w;
    king_danger[WHITE] += params.kat_attack_weight[PAWN] * pop_count(defended[BLACK] & king_circle[WHITE]);

    king_danger[BLACK] -= params.kat_defence_weight[PAWN] * pawn_shield_b;
    king_danger[BLACK] += params.kat_attack_weight[PAWN] * pop_count(defended[WHITE] & king_circle[BLACK]);

    U64 knights = board.bb_pieces[WHITE][KNIGHT];
    while (knights) {
        uint8_t sq = pop_bit(knights);
        U64 moves = find_moves<KNIGHT>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[KNIGHT] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[KNIGHT] * pop_count(moves & king_circle[BLACK]);
        int move_count = pop_count(moves & ~defended[BLACK] & ~board.bb_side[WHITE]);
        mg += move_count * params.pos_mob_mg[KNIGHT - 1];
        eg += move_count * params.pos_mob_eg[KNIGHT - 1];
    }

    U64 bishops = board.bb_pieces[WHITE][BISHOP];
    while (bishops) {
        uint8_t sq = pop_bit(bishops);
        U64 moves = find_moves<BISHOP>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[BISHOP] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[BISHOP] * pop_count(moves & king_circle[BLACK]);
        int move_count = pop_count(moves & ~defended[BLACK] & ~board.bb_side[WHITE]);
        mg += move_count * params.pos_mob_mg[BISHOP - 1];
        eg += move_count * params.pos_mob_eg[BISHOP - 1];
    }

    U64 rooks = board.bb_pieces[WHITE][ROOK];
    while (rooks) {
        uint8_t sq = pop_bit(rooks);
        U64 moves = find_moves<ROOK>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[ROOK] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[ROOK] * pop_count(moves & king_circle[BLACK]);
        int move_count = pop_count(moves & ~defended[BLACK] & ~board.bb_side[WHITE]);
        mg += move_count * params.pos_mob_mg[ROOK - 1];
        eg += move_count * params.pos_mob_eg[ROOK - 1];

        // extended evaluation of rooks
        if((single_bit(sq) & passed_rear[WHITE]) && (moves & passed[WHITE])) eg += params.pos_r_behind_own_passer_eg;
        if((single_bit(sq) & passed_rear[BLACK]) && (moves & passed[BLACK])) eg += params.pos_r_behind_enemy_passer_eg;
        eg += pop_count(find_moves<ROOK>(WHITE, sq, board.bb_pieces[WHITE][PAWN]) & undefended_pawns[BLACK])
                * params.pos_r_xray_pawn_eg;
    }

    U64 queens = board.bb_pieces[WHITE][QUEEN];
    while (queens) {
        uint8_t sq = pop_bit(queens);
        U64 moves = find_moves<QUEEN>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[QUEEN] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[QUEEN] * pop_count(moves & king_circle[BLACK]);
        int move_count = pop_count(moves & ~defended[BLACK] & ~board.bb_side[WHITE]);
        mg += move_count * params.pos_mob_mg[QUEEN - 1];
        eg += move_count * params.pos_mob_eg[QUEEN - 1];
    }

    knights = board.bb_pieces[BLACK][KNIGHT];
    while (knights) {
        uint8_t sq = pop_bit(knights);
        U64 moves = find_moves<KNIGHT>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[KNIGHT] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[KNIGHT] * pop_count(moves & king_circle[WHITE]);
        int move_count = pop_count(moves & ~defended[WHITE] & ~board.bb_side[BLACK]);
        mg -= move_count * params.pos_mob_mg[KNIGHT - 1];
        eg -= move_count * params.pos_mob_eg[KNIGHT - 1];
    }

    bishops = board.bb_pieces[BLACK][BISHOP];
    while (bishops) {
        uint8_t sq = pop_bit(bishops);
        U64 moves = find_moves<BISHOP>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[BISHOP] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[BISHOP] * pop_count(moves & king_circle[WHITE]);
        int move_count = pop_count(moves & ~defended[WHITE] & ~board.bb_side[BLACK]);
        mg -= move_count * params.pos_mob_mg[BISHOP - 1];
        eg -= move_count * params.pos_mob_eg[BISHOP - 1];
    }

    rooks = board.bb_pieces[BLACK][ROOK];
    while (rooks) {
        uint8_t sq = pop_bit(rooks);
        U64 moves = find_moves<ROOK>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[ROOK] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[ROOK] * pop_count(moves & king_circle[WHITE]);
        int move_count = pop_count(moves & ~defended[WHITE] & ~board.bb_side[BLACK]);
        mg -= move_count * params.pos_mob_mg[ROOK - 1];
        eg -= move_count * params.pos_mob_eg[ROOK - 1];

        // extended evaluation of rooks
        if((single_bit(sq) & passed_rear[BLACK]) && (moves & passed[BLACK])) eg -= params.pos_r_behind_own_passer_eg;
        if((single_bit(sq) & passed_rear[WHITE]) && (moves & passed[WHITE])) eg -= params.pos_r_behind_enemy_passer_eg;
        eg -= pop_count(find_moves<ROOK>(BLACK, sq, board.bb_pieces[BLACK][PAWN]) & undefended_pawns[WHITE])
              * params.pos_r_xray_pawn_eg;
    }

    queens = board.bb_pieces[BLACK][QUEEN];
    while (queens) {
        uint8_t sq = pop_bit(queens);
        U64 moves = find_moves<QUEEN>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[QUEEN] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[QUEEN] * pop_count(moves & king_circle[WHITE]);
        int move_count = pop_count(moves & ~defended[WHITE] & ~board.bb_side[BLACK]);
        mg -= move_count * params.pos_mob_mg[QUEEN - 1];
        eg -= move_count * params.pos_mob_eg[QUEEN - 1];
    }

    mg -= params.kat_table[std::clamp(king_danger[WHITE], 0, 127)];
    mg += params.kat_table[std::clamp(king_danger[BLACK], 0, 127)];
}

void evaluator_t::eval_positional(const board_t &board, int &mg, int &eg) {
    if(board.record[board.now].material.info.w_bishops >= 2) {
        mg += params.pos_bishop_pair_mg;
        eg += params.pos_bishop_pair_eg;
    }

    if(board.record[board.now].material.info.b_bishops >= 2) {
        mg -= params.pos_bishop_pair_mg;
        eg -= params.pos_bishop_pair_eg;
    }

    // Rook trapped without castling
    if(((board.bb_pieces[WHITE][ROOK] & single_bit(rel_sq(WHITE, A1)))
        && (board.bb_pieces[WHITE][KING] & (bits_between(rel_sq(WHITE, A1), rel_sq(WHITE, E1)))))
       || ((board.bb_pieces[WHITE][ROOK] & single_bit(rel_sq(WHITE, H1)))
       && (board.bb_pieces[WHITE][KING] & (bits_between(rel_sq(WHITE, E1), rel_sq(WHITE, H1)))))) {
        mg += params.pos_r_trapped_mg;
    }

    if((((board.bb_pieces[BLACK][ROOK] & single_bit(rel_sq(BLACK, A1)))
         && (board.bb_pieces[BLACK][KING] & (bits_between(rel_sq(BLACK, A1), rel_sq(BLACK, E1)))))
        || ((board.bb_pieces[BLACK][ROOK] & single_bit(rel_sq(BLACK, H1)))
            && (board.bb_pieces[BLACK][KING] & (bits_between(rel_sq(BLACK, E1), rel_sq(BLACK, H1)))))))  {
        mg -= params.pos_r_trapped_mg;
    }
}
