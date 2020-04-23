//
// Created by Vincent on 30/09/2017.
//

#include "eval.h"
#include "endgame.h"
#include <algorithm>
#include <cstring>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utility tables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

U64 BB_KING_SQUARE[64] = {};
U64 BB_KING_CIRCLE[64] = {};
U64 BB_PAWN_SHIELD[64] = {};

void evaluator_t::eval_init() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        BB_KING_SQUARE[sq] = single_bit(sq) | find_moves<KING>(WHITE, sq, 0);
    }

    for (uint8_t sq = 0; sq < 64; sq++) {
        // King circle
        BB_KING_CIRCLE[sq] = BB_KING_SQUARE[sq];
        if(rank_index(sq) == 0) BB_KING_CIRCLE[sq] |= bb_shifts::shift<D_N>(BB_KING_SQUARE[sq]);
        if(rank_index(sq) == 7) BB_KING_CIRCLE[sq] |= bb_shifts::shift<D_S>(BB_KING_SQUARE[sq]);
        if(file_index(sq) == 0) BB_KING_CIRCLE[sq] |= bb_shifts::shift<D_E>(BB_KING_SQUARE[sq]);
        if(file_index(sq) == 7) BB_KING_CIRCLE[sq] |= bb_shifts::shift<D_W>(BB_KING_SQUARE[sq]);

        if(rank_index(sq) <= 1) {
            BB_PAWN_SHIELD[sq] = bb_shifts::shift<D_N>(BB_KING_SQUARE[sq]);
        } else if(rank_index(sq) >= 6) {
            BB_PAWN_SHIELD[sq] = bb_shifts::shift<D_S>(BB_KING_SQUARE[sq]);
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
            pst[WHITE][BISHOP][square_mapping[i][j]][MG] = params.b_pst_mg[i];
            pst[WHITE][BISHOP][square_mapping[i][j]][EG] = params.b_pst_eg[i];
            pst[WHITE][ROOK][square_mapping[i][j]][MG] = params.r_pst_mg[i];
            pst[WHITE][ROOK][square_mapping[i][j]][EG] = params.r_pst_eg[i];
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
    eval_data_t data = {};

    // Initialise king danger evaluation
    data.king_pos[WHITE] = bit_scan(board.bb_pieces[WHITE][KING]);
    data.king_pos[BLACK] = bit_scan(board.bb_pieces[BLACK][KING]);
    data.king_circle[WHITE] = BB_KING_CIRCLE[data.king_pos[WHITE]];
    data.king_circle[BLACK] = BB_KING_CIRCLE[data.king_pos[BLACK]];
    data.king_danger[WHITE] = params.kat_zero;
    data.king_danger[BLACK] = params.kat_zero;
    data.update_attacks(WHITE, KING, find_moves<KING>(WHITE, data.king_pos[WHITE], board.bb_all));
    data.update_attacks(BLACK, KING, find_moves<KING>(BLACK, data.king_pos[BLACK], board.bb_all));

    // Middlegame and endgame scores
    int mg = 0;
    int eg = 0;

    // Main evaluation functions
    double phase = game_phase(board);
    eval_pawns(board, mg, eg, data);
    eval_pieces(board, mg, eg, data);
    eval_threats(board, mg, eg, data);
    eval_positional(board, mg, eg, data);

    // Interpolate between middlegame and endgame scores
    auto total = int(phase * mg + (1 - phase) * eg);

    return board.record[board.now].next_move ? -total : total;
}

double evaluator_t::game_phase(const board_t &board) const {
    material_data_t material = board.record[board.now].material;

    const int mat_w = params.mat_exch_knight * (material.info.w_knights)
                      + params.mat_exch_bishop * (material.info.w_bishops)
                      + params.mat_exch_rook * (material.info.w_rooks)
                      + params.mat_exch_queen * (material.info.w_queens);
    const int mat_b = params.mat_exch_knight * (material.info.b_knights)
                      + params.mat_exch_bishop * (material.info.b_bishops)
                      + params.mat_exch_rook * (material.info.b_rooks)
                      + params.mat_exch_queen * (material.info.b_queens);
    const int mat_max = 2 * (params.mat_exch_knight * 2
                             + params.mat_exch_bishop * 2
                             + params.mat_exch_rook * 2
                             + params.mat_exch_queen * 1);
    const int mat_total = mat_w + mat_b;

    // Calculate tapering (game phase)
    // Close to 1 at the start, close to 0 at the end
    return std::min((double(mat_total) / double(mat_max)), 1.0);
}

void evaluator_t::eval_pieces(const board_t &board, int &mg, int &eg, eval_data_t &data) {
    U64 pieces;
    for (int type = KNIGHT; type < KING; type++) {
        pieces = board.bb_pieces[WHITE][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg += params.pst[WHITE][type][sq][MG];
            eg += params.pst[WHITE][type][sq][EG];

            U64 attacks = find_moves(Piece(type), WHITE, sq, board.bb_all);
            data.king_danger[WHITE] -= pop_count(attacks & data.king_circle[WHITE]) * params.kat_defence_weight[type];
            data.king_danger[BLACK] += pop_count(attacks & data.king_circle[BLACK]) * params.kat_attack_weight[type];
            data.update_attacks(WHITE, Piece(type), attacks);
            int mobility = pop_count(attacks & ~data.attacks[BLACK][PAWN] & ~board.bb_side[WHITE]);
            mg += mobility * params.pos_mob_mg[type - 1];
            eg += mobility * params.pos_mob_eg[type - 1];
        }

        pieces = board.bb_pieces[BLACK][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg -= params.pst[BLACK][type][sq][MG];
            eg -= params.pst[BLACK][type][sq][EG];

            U64 attacks = find_moves(Piece(type), WHITE, sq, board.bb_all);
            data.king_danger[BLACK] -= pop_count(attacks & data.king_circle[BLACK]) * params.kat_defence_weight[type];
            data.king_danger[WHITE] += pop_count(attacks & data.king_circle[WHITE]) * params.kat_attack_weight[type];
            data.update_attacks(BLACK, Piece(type), attacks);
            int mobility = pop_count(attacks & ~data.attacks[WHITE][PAWN] & ~board.bb_side[BLACK]);
            mg -= mobility * params.pos_mob_mg[type - 1];
            eg -= mobility * params.pos_mob_eg[type - 1];
        }
    }
}

void evaluator_t::eval_pawns(const board_t &board, int &mg, int &eg, eval_data_t &data) {
    U64 pawn_hash = board.record[board.now].kp_hash;
    size_t index = (pawn_hash & pawn_hash_entries);
    pawns::structure_t *entry = pawn_hash_table.data() + index;
    if(entry->get_hash() != pawn_hash) {
        *entry = pawns::structure_t(params, pawn_hash,
                                    board.bb_pieces[WHITE][PAWN], board.bb_pieces[BLACK][PAWN],
                                    board.bb_pieces[WHITE][KING], board.bb_pieces[BLACK][KING]);
    }

    mg += entry->get_eval_mg();
    eg += entry->get_eval_eg();

    U64 w_pawns = board.bb_pieces[WHITE][PAWN];
    U64 b_pawns = board.bb_pieces[BLACK][PAWN];

    U64 open_files = pawns::open_files(w_pawns, b_pawns);
    U64 half_open_files[2] = {
            pawns::half_or_open_files(w_pawns) ^ open_files,
            pawns::half_or_open_files(b_pawns) ^ open_files
    };

    // Blocked pawns
    U64 stop_squares[2] = {pawns::stop_squares<WHITE>(w_pawns), pawns::stop_squares<BLACK>(b_pawns)};
    int blocked_count[2][2] = {
            {pop_count(stop_squares[WHITE] & board.bb_side[WHITE]),
             pop_count(stop_squares[WHITE] & board.bb_side[BLACK])},
            {pop_count(stop_squares[BLACK] & board.bb_side[BLACK]),
             pop_count(stop_squares[BLACK] & board.bb_side[WHITE])}
    };

    // Rook on open files
    int open_file_count[2] = {
            pop_count(open_files & board.bb_pieces[WHITE][ROOK]),
            pop_count(open_files & board.bb_pieces[BLACK][ROOK])
    };
    int own_half_open_file_count[2] = {
            pop_count(half_open_files[WHITE] & board.bb_pieces[WHITE][ROOK]),
            pop_count(half_open_files[BLACK] & board.bb_pieces[BLACK][ROOK])
    };
    int other_half_open_file_count[2] = {
            pop_count(half_open_files[BLACK] & board.bb_pieces[WHITE][ROOK]),
            pop_count(half_open_files[WHITE] & board.bb_pieces[BLACK][ROOK])
    };

    // Outposts
    U64 defended[2] = {pawns::attacks<WHITE>(w_pawns), pawns::attacks<BLACK>(b_pawns)};
    U64 outpost_squares[2] = {pawns::outpost[WHITE] & defended[WHITE] & ~defended[BLACK],
                              pawns::outpost[BLACK] & defended[BLACK] & ~defended[WHITE]};
    U64 holes_bb[2] = {pawns::holes<WHITE>(w_pawns), pawns::holes<BLACK>(b_pawns)};

    int outpost_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & board.bb_pieces[WHITE][KNIGHT]),
             pop_count(outpost_squares[WHITE] & board.bb_pieces[WHITE][BISHOP])},
            {pop_count(outpost_squares[BLACK] & board.bb_pieces[BLACK][KNIGHT]),
             pop_count(outpost_squares[BLACK] & board.bb_pieces[BLACK][BISHOP])},
    };
    int outpost_hole_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & holes_bb[BLACK] & board.bb_pieces[WHITE][KNIGHT]),
             pop_count(outpost_squares[WHITE] & holes_bb[BLACK] & board.bb_pieces[WHITE][BISHOP])},
            {pop_count(outpost_squares[BLACK] & holes_bb[WHITE] & board.bb_pieces[BLACK][KNIGHT]),
             pop_count(outpost_squares[BLACK] & holes_bb[WHITE] & board.bb_pieces[BLACK][BISHOP])},
    };
    int outpost_half_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & half_open_files[WHITE] & board.bb_pieces[WHITE][KNIGHT]),
             pop_count(outpost_squares[WHITE] & half_open_files[WHITE] & board.bb_pieces[WHITE][BISHOP])},
            {pop_count(outpost_squares[BLACK] & half_open_files[BLACK] & board.bb_pieces[BLACK][KNIGHT]),
             pop_count(outpost_squares[BLACK] & half_open_files[BLACK] & board.bb_pieces[BLACK][BISHOP])},
    };

    // Rooks
    U64 passed[2] = {
            pawns::passed<WHITE>(board.bb_pieces[WHITE][PAWN], board.bb_pieces[BLACK][PAWN]),
            pawns::passed<BLACK>(board.bb_pieces[BLACK][PAWN], board.bb_pieces[WHITE][PAWN])
    };
    U64 passed_rear[2] = {
            bb_shifts::shift<D_S>(bb_shifts::fill_occluded<D_S>(passed[WHITE], ~board.bb_all)),
            bb_shifts::shift<D_N>(bb_shifts::fill_occluded<D_N>(passed[BLACK], ~board.bb_all)),
    };
    int rook_behind[2][2] = {
            {pop_count(board.bb_pieces[WHITE][ROOK] & passed_rear[WHITE]),
                    pop_count(board.bb_pieces[WHITE][ROOK] & passed_rear[BLACK])},
            {pop_count(board.bb_pieces[BLACK][ROOK] & passed_rear[BLACK]),
                    pop_count(board.bb_pieces[BLACK][ROOK] & passed_rear[WHITE])},
    };

    // King shield
    int pawn_shield_w = std::min(3, pop_count(BB_PAWN_SHIELD[data.king_pos[WHITE]] & board.bb_pieces[WHITE][PAWN]));
    int pawn_shield_b = std::min(3, pop_count(BB_PAWN_SHIELD[data.king_pos[BLACK]] & board.bb_pieces[BLACK][PAWN]));

    mg += (blocked_count[WHITE][0] - blocked_count[BLACK][0]) * params.blocked_mg[0];
    mg += (blocked_count[WHITE][1] - blocked_count[BLACK][1]) * params.blocked_mg[1];
    eg += (blocked_count[WHITE][0] - blocked_count[BLACK][0]) * params.blocked_eg[0];
    eg += (blocked_count[WHITE][1] - blocked_count[BLACK][1]) * params.blocked_eg[1];

    mg += (open_file_count[WHITE] - open_file_count[BLACK]) * params.pos_r_open_file_mg;
    mg += (own_half_open_file_count[WHITE] - own_half_open_file_count[BLACK]) * params.pos_r_own_half_open_file_mg;
    mg += (other_half_open_file_count[WHITE] - other_half_open_file_count[BLACK]) *
          params.pos_r_other_half_open_file_mg;
    eg += (open_file_count[WHITE] - open_file_count[BLACK]) * params.pos_r_open_file_eg;
    eg += (own_half_open_file_count[WHITE] - own_half_open_file_count[BLACK]) * params.pos_r_own_half_open_file_eg;
    eg += (other_half_open_file_count[WHITE] - other_half_open_file_count[BLACK]) *
          params.pos_r_other_half_open_file_eg;

    mg += (outpost_count[WHITE][0] - outpost_count[BLACK][0]) * params.outpost_mg[0];
    mg += (outpost_count[WHITE][1] - outpost_count[BLACK][1]) * params.outpost_mg[1];
    eg += (outpost_count[WHITE][0] - outpost_count[BLACK][0]) * params.outpost_eg[0];
    eg += (outpost_count[WHITE][1] - outpost_count[BLACK][1]) * params.outpost_eg[1];
    mg += (outpost_hole_count[WHITE][0] - outpost_hole_count[BLACK][0]) * params.outpost_hole_mg[0];
    mg += (outpost_hole_count[WHITE][1] - outpost_hole_count[BLACK][1]) * params.outpost_hole_mg[1];
    eg += (outpost_hole_count[WHITE][0] - outpost_hole_count[BLACK][0]) * params.outpost_hole_eg[0];
    eg += (outpost_hole_count[WHITE][1] - outpost_hole_count[BLACK][1]) * params.outpost_hole_eg[1];
    mg += (outpost_half_count[WHITE][0] - outpost_half_count[BLACK][0]) * params.outpost_half_mg[0];
    mg += (outpost_half_count[WHITE][1] - outpost_half_count[BLACK][1]) * params.outpost_half_mg[1];
    eg += (outpost_half_count[WHITE][0] - outpost_half_count[BLACK][0]) * params.outpost_half_eg[0];
    eg += (outpost_half_count[WHITE][1] - outpost_half_count[BLACK][1]) * params.outpost_half_eg[1];

    eg += (rook_behind[WHITE][0] - rook_behind[BLACK][0]) * params.pos_r_behind_own_passer_eg;
    eg += (rook_behind[WHITE][1] - rook_behind[BLACK][1]) * params.pos_r_behind_enemy_passer_eg;

    // Update attacks
    data.update_attacks(WHITE, PAWN, pawns::left_attacks<WHITE>(board.bb_pieces[WHITE][PAWN]));
    data.update_attacks(WHITE, PAWN, pawns::right_attacks<WHITE>(board.bb_pieces[WHITE][PAWN]));
    data.update_attacks(BLACK, PAWN, pawns::left_attacks<BLACK>(board.bb_pieces[BLACK][PAWN]));
    data.update_attacks(BLACK, PAWN, pawns::right_attacks<BLACK>(board.bb_pieces[BLACK][PAWN]));

    // King safety
    if(board.bb_pieces[BLACK][ROOK] || board.bb_pieces[BLACK][QUEEN]) {
        if(data.king_circle[WHITE] & open_files) {
            data.king_danger[WHITE] += params.kat_open_file;
        } else if(data.king_circle[WHITE] & half_open_files[WHITE]) {
            data.king_danger[WHITE] += params.kat_own_half_open_file;
        } else if(data.king_circle[WHITE] & half_open_files[BLACK]) {
            data.king_danger[WHITE] += params.kat_other_half_open_file;
        }
    }
    if(board.bb_pieces[WHITE][ROOK] || board.bb_pieces[WHITE][QUEEN]) {
        if(data.king_circle[BLACK] & open_files) {
            data.king_danger[BLACK] += params.kat_open_file;
        } else if(data.king_circle[BLACK] & half_open_files[BLACK]) {
            data.king_danger[BLACK] += params.kat_own_half_open_file;
        } else if(data.king_circle[BLACK] & half_open_files[WHITE]) {
            data.king_danger[BLACK] += params.kat_other_half_open_file;
        }
    }

    mg += params.ks_pawn_shield[pawn_shield_w];
    mg -= params.ks_pawn_shield[pawn_shield_b];
    data.king_danger[WHITE] -= params.kat_defence_weight[PAWN] * pawn_shield_w;
    data.king_danger[WHITE] += params.kat_attack_weight[PAWN] * pop_count(defended[BLACK] & data.king_circle[WHITE]);
    data.king_danger[BLACK] -= params.kat_defence_weight[PAWN] * pawn_shield_b;
    data.king_danger[BLACK] += params.kat_attack_weight[PAWN] * pop_count(defended[WHITE] & data.king_circle[BLACK]);
}

void evaluator_t::eval_threats(const board_t &board, int &mg, int &eg, eval_data_t &data) {
    for(int target = PAWN; target < KING; target++) {
        int undefended[2] = {pop_count(board.bb_pieces[WHITE][target] & ~data.team_attacks[WHITE]),
                             pop_count(board.bb_pieces[BLACK][target] & ~data.team_attacks[BLACK])};
        mg += (undefended[WHITE] - undefended[BLACK]) * params.undefended_mg[target];
        eg += (undefended[WHITE] - undefended[BLACK]) * params.undefended_eg[target];

        for(int attacker = PAWN; attacker < QUEEN; attacker++) {
            int attacks[2] = {pop_count(board.bb_pieces[BLACK][target] & data.attacks[WHITE][attacker]),
                           pop_count(board.bb_pieces[WHITE][target] & data.attacks[BLACK][attacker])};
            mg += (attacks[WHITE] - attacks[BLACK]) * params.threat_matrix_mg[attacker][target];
            eg += (attacks[WHITE] - attacks[BLACK]) * params.threat_matrix_eg[attacker][target];
        }
    }

    mg -= params.kat_table[std::clamp(data.king_danger[WHITE], 0, 127)];
    mg += params.kat_table[std::clamp(data.king_danger[BLACK], 0, 127)];
}

void evaluator_t::eval_positional(const board_t &board, int &mg, int &eg, eval_data_t &data) {
    if(board.record[board.now].material.info.w_bishops >= 2) {
        mg += params.pos_bishop_pair_mg;
        eg += params.pos_bishop_pair_eg;
    }

    if(board.record[board.now].material.info.b_bishops >= 2) {
        mg -= params.pos_bishop_pair_mg;
        eg -= params.pos_bishop_pair_eg;
    }

    // Check for opposite coloured bishops
    bool opposite_coloured_bishops = board.bb_pieces[WHITE][BISHOP] && board.bb_pieces[BLACK][BISHOP]
                                     && !multiple_bits(board.bb_pieces[WHITE][BISHOP]) && !multiple_bits(board.bb_pieces[BLACK][BISHOP])
                                     && !same_colour(bit_scan(board.bb_pieces[WHITE][BISHOP]), bit_scan(board.bb_pieces[BLACK][BISHOP]));
    const int pawn_balance = board.record[board.now].material.info.w_pawns - board.record[board.now].material.info.b_pawns;

    if(opposite_coloured_bishops) {
        if(pawn_balance > 0) {
            eg -= params.mat_opp_bishop[std::min(2, pawn_balance - 1)];
        } else if(pawn_balance < 0) {
            eg += params.mat_opp_bishop[std::min(2, -pawn_balance - 1)];
        }
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
