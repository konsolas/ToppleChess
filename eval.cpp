//
// Created by Vincent on 30/09/2017.
//

#include "eval.h"
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
            pst[WHITE][KNIGHT][square_mapping[i][j]] = params.n_pst[i];
            pst[WHITE][BISHOP][square_mapping[i][j]] = params.b_pst[i];
            pst[WHITE][ROOK][square_mapping[i][j]] = params.r_pst[i];
            pst[WHITE][QUEEN][square_mapping[i][j]] = params.q_pst[i];
            pst[WHITE][KING][square_mapping[i][j]] = params.k_pst[i];
        }
    }

    // Vertically mirrored tables
    for (uint8_t sq = 0; sq < 64; sq++) {
        int param_index = 32 - (4 - (file_index(sq) % 4) + rank_index(sq) * 4);

        if (sq >= 8 && sq < 56) {
            pst[WHITE][PAWN][sq] = params.p_pst[param_index - 4];
        }
    }

    // Mirror PST for black
    for (int piece = 0; piece < 6; piece++) {
        for (int square = 0; square < 64; square++) {
            pst[BLACK][piece][MIRROR_TABLE[square]] = pst[WHITE][piece][square];
        }
    }

    // Set up king safety evaluation table.
    for(int i = 0; i < 128; i++) {
        // Translated + scaled sigmoid function
        for (int j = 0; j < 4; j++) {
            kat_table[i][j] = (int) ((double) params.kat_table_max[j] /
                            (1 + exp((params.kat_table_translate - i) * double(params.kat_table_scale) / 1024.0)));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

evaluator_t::evaluator_t(const processed_params_t &params, size_t pawn_hash_size) : params(params) {
    // Set up pawn hash table
    pawn_hash_size /= sizeof(pawns::structure_t);
    this->pawn_hash_entries = tt::lower_power_of_2(pawn_hash_size) - 1;
    pawn_hash_table = new pawns::structure_t[pawn_hash_entries + 1]();
}

evaluator_t::~evaluator_t() {
    delete[] pawn_hash_table;
}

void evaluator_t::prefetch(U64 pawn_hash) {
    size_t index = (pawn_hash & pawn_hash_entries);
    pawns::structure_t *bucket = pawn_hash_table + index;

#if defined(__GNUC__)
    __builtin_prefetch(bucket);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
    _mm_prefetch((char*) bucket, _MM_HINT_T0);
#endif
}

int evaluator_t::evaluate(const board_t &board) {
    eval_data_t data = {};

    // Initialise king danger evaluation
    data.king_pos[WHITE] = bit_scan(board.pieces(WHITE, KING));
    data.king_pos[BLACK] = bit_scan(board.pieces(BLACK, KING));
    data.king_circle[WHITE] = BB_KING_CIRCLE[data.king_pos[WHITE]];
    data.king_circle[BLACK] = BB_KING_CIRCLE[data.king_pos[BLACK]];
    data.king_danger[WHITE] = params.kat_zero;
    data.king_danger[BLACK] = params.kat_zero;
    data.update_attacks(WHITE, KING, find_moves<KING>(WHITE, data.king_pos[WHITE], board.all()));
    data.update_attacks(BLACK, KING, find_moves<KING>(BLACK, data.king_pos[BLACK], board.all()));

    // Score accumulator
    v4si_t score = {0, 0, 0, 0};

    // Main evaluation functions
    float me_phase = game_phase(board);
    float co_phase;
    score += eval_pawns(board, data, co_phase);
    score += eval_pieces(board, data);
    score += eval_threats(board, data);
    score += eval_positional(board, data);

    // Interpolate between scores
    float mg = co_phase * score[0] + (1 - co_phase) * score[1];
    float eg = co_phase * score[2] + (1 - co_phase) * score[3];
    auto total = int(me_phase * mg + (1 - me_phase) * eg);

    return board.now().next_move ? -total : total;
}

float evaluator_t::game_phase(const board_t &board) const {
    material_data_t material = board.now().material;

    const int mat_w = params.mat_exch_knight * (material.count(WHITE, KNIGHT))
                      + params.mat_exch_bishop * (material.count(WHITE, BISHOP))
                      + params.mat_exch_rook * (material.count(WHITE, ROOK))
                      + params.mat_exch_queen * (material.count(WHITE, QUEEN));
    const int mat_b = params.mat_exch_knight * (material.count(BLACK, KNIGHT))
                      + params.mat_exch_bishop * (material.count(BLACK, BISHOP))
                      + params.mat_exch_rook * (material.count(BLACK, ROOK))
                      + params.mat_exch_queen * (material.count(BLACK, QUEEN));
    const int mat_max = 2 * (params.mat_exch_knight * 2
                             + params.mat_exch_bishop * 2
                             + params.mat_exch_rook * 2
                             + params.mat_exch_queen * 1);
    const int mat_total = mat_w + mat_b;

    // Calculate tapering (game phase)
    // Close to 1 at the start, close to 0 at the end
    return std::min((float(mat_total) / float(mat_max)), 1.0F);
}

v4si_t evaluator_t::eval_pieces(const board_t &board, eval_data_t &data) {
    v4si_t score = {0, 0, 0, 0};
    U64 pieces;
    for (Piece type = KNIGHT; type < KING; type++) {
        U64 cached_double_attack_mask[2] = {~data.double_attacks[WHITE] | data.double_attacks[BLACK],
                                            ~data.double_attacks[BLACK] | data.double_attacks[WHITE]};

        pieces = board.pieces(WHITE, type);
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            score += params.pst[WHITE][type][sq];

            U64 attacks = find_moves(Piece(type), WHITE, sq, board.all());
            data.king_danger[WHITE] -= pop_count(attacks & data.king_circle[WHITE]) * params.kat_defence_weight[type];
            data.king_danger[BLACK] += pop_count(attacks & data.king_circle[BLACK]) * params.kat_attack_weight[type];
            data.update_attacks(WHITE, Piece(type), attacks);
            int mobility = pop_count(attacks & ~data.attacks[BLACK][PAWN] & ~board.side(WHITE) & cached_double_attack_mask[BLACK]);
            score += mobility * params.pos_mob[type - 1];
        }

        pieces = board.pieces(BLACK, type);

        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            score -= params.pst[BLACK][type][sq];

            U64 attacks = find_moves(Piece(type), WHITE, sq, board.all());
            data.king_danger[BLACK] -= pop_count(attacks & data.king_circle[BLACK]) * params.kat_defence_weight[type];
            data.king_danger[WHITE] += pop_count(attacks & data.king_circle[WHITE]) * params.kat_attack_weight[type];
            data.update_attacks(BLACK, Piece(type), attacks);
            int mobility = pop_count(attacks & ~data.attacks[WHITE][PAWN] & ~board.side(BLACK) & cached_double_attack_mask[WHITE]);
            score -= mobility * params.pos_mob[type - 1];
        }
    }

    return score;
}

v4si_t evaluator_t::eval_pawns(const board_t &board, eval_data_t &data, float &taper) {
    U64 pawn_hash = board.now().kp_hash;
    size_t index = (pawn_hash & pawn_hash_entries);
    pawns::structure_t *entry = pawn_hash_table + index;
    if(entry->get_hash() != pawn_hash) {
        *entry = pawns::structure_t(params, pawn_hash,
                                    board.pieces(WHITE, PAWN), board.pieces(BLACK, PAWN),
                                    board.pieces(WHITE, KING), board.pieces(BLACK, KING));
    }

    v4si_t score = {entry->get_eval_mg(), entry->get_eval_mg(), entry->get_eval_eg(), entry->get_eval_eg()};
    taper = entry->get_taper();

    U64 w_pawns = board.pieces(WHITE, PAWN);
    U64 b_pawns = board.pieces(BLACK, PAWN);

    U64 open_files = pawns::open_files(w_pawns, b_pawns);
    U64 half_open_files[2] = {
            pawns::half_or_open_files(w_pawns) ^ open_files,
            pawns::half_or_open_files(b_pawns) ^ open_files
    };

    // Blocked pawns
    U64 stop_squares[2] = {pawns::stop_squares<WHITE>(w_pawns), pawns::stop_squares<BLACK>(b_pawns)};
    int blocked_count[2][2] = {
            {pop_count(stop_squares[WHITE] & board.side(WHITE)),
             pop_count(stop_squares[WHITE] & board.side(BLACK))},
            {pop_count(stop_squares[BLACK] & board.side(BLACK)),
             pop_count(stop_squares[BLACK] & board.side(WHITE))}
    };

    // Rook on open files
    int open_file_count[2] = {
            pop_count(open_files & board.pieces(WHITE, ROOK)),
            pop_count(open_files & board.pieces(BLACK, ROOK))
    };
    int own_half_open_file_count[2] = {
            pop_count(half_open_files[WHITE] & board.pieces(WHITE, ROOK)),
            pop_count(half_open_files[BLACK] & board.pieces(BLACK, ROOK))
    };
    int other_half_open_file_count[2] = {
            pop_count(half_open_files[BLACK] & board.pieces(WHITE, ROOK)),
            pop_count(half_open_files[WHITE] & board.pieces(BLACK, ROOK))
    };

    // Outposts
    U64 defended[2] = {pawns::attacks<WHITE>(w_pawns), pawns::attacks<BLACK>(b_pawns)};
    U64 outpost_squares[2] = {pawns::outpost[WHITE] & defended[WHITE] & ~defended[BLACK],
                              pawns::outpost[BLACK] & defended[BLACK] & ~defended[WHITE]};
    U64 holes_bb[2] = {pawns::holes<WHITE>(w_pawns), pawns::holes<BLACK>(b_pawns)};

    int outpost_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & board.pieces(WHITE, KNIGHT)),
             pop_count(outpost_squares[WHITE] & board.pieces(WHITE, BISHOP))},
            {pop_count(outpost_squares[BLACK] & board.pieces(BLACK, KNIGHT)),
             pop_count(outpost_squares[BLACK] & board.pieces(BLACK, BISHOP))},
    };
    int outpost_hole_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & holes_bb[BLACK] & board.pieces(WHITE, KNIGHT)),
             pop_count(outpost_squares[WHITE] & holes_bb[BLACK] & board.pieces(WHITE, BISHOP))},
            {pop_count(outpost_squares[BLACK] & holes_bb[WHITE] & board.pieces(BLACK, KNIGHT)),
             pop_count(outpost_squares[BLACK] & holes_bb[WHITE] & board.pieces(BLACK, BISHOP))},
    };
    int outpost_half_count[2][2] = {
            {pop_count(outpost_squares[WHITE] & half_open_files[WHITE] & board.pieces(WHITE, KNIGHT)),
             pop_count(outpost_squares[WHITE] & half_open_files[WHITE] & board.pieces(WHITE, BISHOP))},
            {pop_count(outpost_squares[BLACK] & half_open_files[BLACK] & board.pieces(BLACK, KNIGHT)),
             pop_count(outpost_squares[BLACK] & half_open_files[BLACK] & board.pieces(BLACK, BISHOP))},
    };

    // Rooks
    U64 passed[2] = {
            pawns::passed<WHITE>(board.pieces(WHITE, PAWN), board.pieces(BLACK, PAWN)),
            pawns::passed<BLACK>(board.pieces(BLACK, PAWN), board.pieces(WHITE, PAWN))
    };
    U64 passed_rear[2] = {
            bb_shifts::shift<D_S>(bb_shifts::fill_occluded<D_S>(passed[WHITE], ~board.all())),
            bb_shifts::shift<D_N>(bb_shifts::fill_occluded<D_N>(passed[BLACK], ~board.all())),
    };
    int rook_behind[2][2] = {
            {pop_count(board.pieces(WHITE, ROOK) & passed_rear[WHITE]),
                    pop_count(board.pieces(WHITE, ROOK) & passed_rear[BLACK])},
            {pop_count(board.pieces(BLACK, ROOK) & passed_rear[BLACK]),
                    pop_count(board.pieces(BLACK, ROOK) & passed_rear[WHITE])},
    };

    // King shield
    int pawn_shield_w = std::min(3, pop_count(BB_PAWN_SHIELD[data.king_pos[WHITE]] & board.pieces(WHITE, PAWN)));
    int pawn_shield_b = std::min(3, pop_count(BB_PAWN_SHIELD[data.king_pos[BLACK]] & board.pieces(BLACK, PAWN)));

    score += (blocked_count[WHITE][0] - blocked_count[BLACK][0]) * params.blocked[0];
    score += (blocked_count[WHITE][1] - blocked_count[BLACK][1]) * params.blocked[1];

    score += (open_file_count[WHITE] - open_file_count[BLACK]) * params.pos_r_open_file;
    score += (own_half_open_file_count[WHITE] - own_half_open_file_count[BLACK]) * params.pos_r_own_half_open_file;
    score += (other_half_open_file_count[WHITE] - other_half_open_file_count[BLACK]) * params.pos_r_other_half_open_file;

    score += (outpost_count[WHITE][0] - outpost_count[BLACK][0]) * params.outpost[0];
    score += (outpost_count[WHITE][1] - outpost_count[BLACK][1]) * params.outpost[1];
    score += (outpost_hole_count[WHITE][0] - outpost_hole_count[BLACK][0]) * params.outpost_hole[0];
    score += (outpost_hole_count[WHITE][1] - outpost_hole_count[BLACK][1]) * params.outpost_hole[1];
    score += (outpost_half_count[WHITE][0] - outpost_half_count[BLACK][0]) * params.outpost_half[0];
    score += (outpost_half_count[WHITE][1] - outpost_half_count[BLACK][1]) * params.outpost_half[1];

    score += (rook_behind[WHITE][0] - rook_behind[BLACK][0]) * params.pos_r_behind_own_passer;
    score += (rook_behind[WHITE][1] - rook_behind[BLACK][1]) * params.pos_r_behind_enemy_passer;

    // Update attacks
    data.update_attacks(WHITE, PAWN, pawns::left_attacks<WHITE>(board.pieces(WHITE, PAWN)));
    data.update_attacks(WHITE, PAWN, pawns::right_attacks<WHITE>(board.pieces(WHITE, PAWN)));
    data.update_attacks(BLACK, PAWN, pawns::left_attacks<BLACK>(board.pieces(BLACK, PAWN)));
    data.update_attacks(BLACK, PAWN, pawns::right_attacks<BLACK>(board.pieces(BLACK, PAWN)));

    // King safety
    if(board.pieces(BLACK, ROOK) || board.pieces(BLACK, QUEEN)) {
        if(data.king_circle[WHITE] & open_files) {
            data.king_danger[WHITE] += params.kat_open_file;
        } else if(data.king_circle[WHITE] & half_open_files[WHITE]) {
            data.king_danger[WHITE] += params.kat_own_half_open_file;
        } else if(data.king_circle[WHITE] & half_open_files[BLACK]) {
            data.king_danger[WHITE] += params.kat_other_half_open_file;
        }
    }
    if(board.pieces(WHITE, ROOK) || board.pieces(WHITE, QUEEN)) {
        if(data.king_circle[BLACK] & open_files) {
            data.king_danger[BLACK] += params.kat_open_file;
        } else if(data.king_circle[BLACK] & half_open_files[BLACK]) {
            data.king_danger[BLACK] += params.kat_own_half_open_file;
        } else if(data.king_circle[BLACK] & half_open_files[WHITE]) {
            data.king_danger[BLACK] += params.kat_other_half_open_file;
        }
    }

    score += params.ks_pawn_shield[pawn_shield_w];
    score -= params.ks_pawn_shield[pawn_shield_b];
    data.king_danger[WHITE] -= params.kat_defence_weight[PAWN] * pawn_shield_w;
    data.king_danger[WHITE] += params.kat_attack_weight[PAWN] * pop_count(defended[BLACK] & data.king_circle[WHITE]);
    data.king_danger[BLACK] -= params.kat_defence_weight[PAWN] * pawn_shield_b;
    data.king_danger[BLACK] += params.kat_attack_weight[PAWN] * pop_count(defended[WHITE] & data.king_circle[BLACK]);

    return score;
}

v4si_t evaluator_t::eval_threats(const board_t &board, eval_data_t &data) {
    v4si_t score = {0, 0, 0, 0};
    for(Piece target = PAWN; target < KING; target++) {
        int undefended[2] = {pop_count(board.pieces(WHITE, target) & ~data.team_attacks[WHITE]),
                             pop_count(board.pieces(BLACK, target) & ~data.team_attacks[BLACK])};
        int overprotected[2] = {pop_count(board.pieces(WHITE, target) & data.double_attacks[WHITE] & ~data.double_attacks[BLACK]),
                                pop_count(board.pieces(BLACK, target) & data.double_attacks[BLACK] & ~data.double_attacks[WHITE])};
        score += (undefended[WHITE] - undefended[BLACK]) * params.undefended[target];
        score += (overprotected[WHITE] - overprotected[BLACK]) * params.overprotected[target];

        for(int attacker = PAWN; attacker < QUEEN; attacker++) {
            int attacks[2] = {pop_count(board.pieces(BLACK, target) & data.attacks[WHITE][attacker]),
                           pop_count(board.pieces(WHITE, target) & data.attacks[BLACK][attacker])};
            score += (attacks[WHITE] - attacks[BLACK]) * params.threat_matrix[attacker][target];
        }
    }

    score -= params.kat_table[std::clamp(data.king_danger[WHITE], 0, 127)];
    score += params.kat_table[std::clamp(data.king_danger[BLACK], 0, 127)];

    return score;
}

v4si_t evaluator_t::eval_positional(const board_t &board, eval_data_t &data) {
    v4si_t score = {0, 0, 0, 0};

    if(board.now().material.count(WHITE, BISHOP) >= 2) {
        score += params.pos_bishop_pair;
    }

    if(board.now().material.count(BLACK, BISHOP) >= 2) {
        score -= params.pos_bishop_pair;
    }

    // Check for opposite coloured bishops
    bool opposite_coloured_bishops = board.pieces(WHITE, BISHOP) && board.pieces(BLACK, BISHOP)
                                     && !multiple_bits(board.pieces(WHITE, BISHOP)) && !multiple_bits(board.pieces(BLACK, BISHOP))
                                     && !same_colour(bit_scan(board.pieces(WHITE, BISHOP)), bit_scan(board.pieces(BLACK, BISHOP)));
    const int pawn_balance = board.now().material.count(WHITE, PAWN) - board.now().material.count(BLACK, PAWN);

    if(opposite_coloured_bishops) {
        if(pawn_balance > 0) {
            score -= params.mat_opp_bishop[std::min(2, pawn_balance - 1)];
        } else if(pawn_balance < 0) {
            score += params.mat_opp_bishop[std::min(2, -pawn_balance - 1)];
        }
    }

    // Rook trapped without castling
    if(((board.pieces(WHITE, ROOK) & single_bit(rel_sq(WHITE, A1)))
        && (board.pieces(WHITE, KING) & (bits_between(rel_sq(WHITE, A1), rel_sq(WHITE, E1)))))
       || ((board.pieces(WHITE, ROOK) & single_bit(rel_sq(WHITE, H1)))
       && (board.pieces(WHITE, KING) & (bits_between(rel_sq(WHITE, E1), rel_sq(WHITE, H1)))))) {
        score += params.pos_r_trapped;
    }

    if((((board.pieces(BLACK, ROOK) & single_bit(rel_sq(BLACK, A1)))
         && (board.pieces(BLACK, KING) & (bits_between(rel_sq(BLACK, A1), rel_sq(BLACK, E1)))))
        || ((board.pieces(BLACK, ROOK) & single_bit(rel_sq(BLACK, H1)))
            && (board.pieces(BLACK, KING) & (bits_between(rel_sq(BLACK, E1), rel_sq(BLACK, H1)))))))  {
        score -= params.pos_r_trapped;
    }

    return score;
}
