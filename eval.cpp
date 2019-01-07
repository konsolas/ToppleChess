//
// Created by Vincent on 30/09/2017.
//

#include "eval.h"
#include "board.h"
#include "endgame.h"
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

U64 BB_KING_CIRCLE[64] = {};

U64 BB_CENTRE = 0;

int kat_table[512] = {};

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
        }
    }

    // Set up king attack table
    for(int i = 0; i < 512; i++) {
        kat_table[i] = (int) (256.0 / (1 + exp(8.0 - (i / 32.0))));
    }

    std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

evaluator_t::evaluator_t(eval_params_t params, size_t pawn_hash_size) : params(params) {
    // Set up pawn hash table
    pawn_hash_size /= (sizeof(pawn_entry_t) * bucket_size);
    this->pawn_hash_entries = tt::lower_power_of_2(pawn_hash_size) - 1;
    pawn_hash_table = new pawn_entry_t[pawn_hash_entries * bucket_size + bucket_size];
    memset(pawn_hash_table, 0, sizeof(pawn_entry_t) * (pawn_hash_entries * bucket_size + bucket_size));

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

    // King middlegame pst
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 8; ++j) {
            pst[WHITE][KING][(2 - i) * 8 + j][MG] = params.k_pst_mg[i * 8 + j];
        }
    }

    for (int i = 24; i < 64; i++) {
        pst[WHITE][KING][i][MG] = params.k_pst_exposed_mg;
    }

    // Mirror PST for black
    for (int piece = 0; piece < 6; piece++) {
        for (int square = 0; square < 64; square++) {
            pst[BLACK][piece][MIRROR_TABLE[square]][MG] = pst[WHITE][piece][square][MG];
            pst[BLACK][piece][MIRROR_TABLE[square]][EG] = pst[WHITE][piece][square][EG];
        }
    }
}

evaluator_t::~evaluator_t() {
    delete[] pawn_hash_table;
}

int evaluator_t::evaluate(const board_t &board) {
    // Check endgame
    eg_eval_t eg_eval = eval_eg(board);
    if (eg_eval.valid) {
        return board.record[board.now].next_move ? -eg_eval.eval : +eg_eval.eval;
    }

    // Middlegame and endgame scores
    int mg = 0;
    int eg = 0;

    // Main evaluation functions
    eval_material(board, mg, eg);
    eval_pst(board, mg, eg);

    pawn_entry_t *pawn_data = eval_pawns(board);
    mg += pawn_data->eval_mg;
    eg += pawn_data->eval_eg;

    int king_danger[2] = {};

    eval_moves(board, mg, eg, pawn_data, king_danger[WHITE], king_danger[BLACK]);
    eval_king_safety(board, mg, eg, pawn_data, king_danger[WHITE], king_danger[BLACK]);

    mg -= kat_table[king_danger[WHITE]]; // High danger is bad
    mg += kat_table[king_danger[BLACK]]; // High danger is bad

    // Interpolate between middlegame and endgame scores
    double game_process = double(pop_count(board.bb_all) - 2) / 30.0;
    auto total = int(game_process * eg + (1 - game_process) * mg);

    return board.record[board.now].next_move ? -total : total;
}

void evaluator_t::eval_material(const board_t &board, int &mg, int &eg) {
    const int pawn_balance = (pop_count(board.bb_pieces[WHITE][PAWN]) - pop_count(board.bb_pieces[BLACK][PAWN]));
    mg += params.mat_mg[PAWN] * pawn_balance;
    eg += params.mat_eg[PAWN] * pawn_balance;

    const int knight_balance = (pop_count(board.bb_pieces[WHITE][KNIGHT]) - pop_count(board.bb_pieces[BLACK][KNIGHT]));
    mg += params.mat_mg[KNIGHT] * knight_balance;
    eg += params.mat_eg[KNIGHT] * knight_balance;

    const int bishop_balance = (pop_count(board.bb_pieces[WHITE][BISHOP]) - pop_count(board.bb_pieces[BLACK][BISHOP]));
    mg += params.mat_mg[BISHOP] * bishop_balance;
    eg += params.mat_eg[BISHOP] * bishop_balance;

    const int rook_balance = (pop_count(board.bb_pieces[WHITE][ROOK]) - pop_count(board.bb_pieces[BLACK][ROOK]));
    mg += params.mat_mg[ROOK] * rook_balance;
    eg += params.mat_eg[ROOK] * rook_balance;

    const int queen_balance = (pop_count(board.bb_pieces[WHITE][QUEEN]) - pop_count(board.bb_pieces[BLACK][QUEEN]));
    mg += params.mat_mg[QUEEN] * queen_balance;
    eg += params.mat_eg[QUEEN] * queen_balance;
}

void evaluator_t::eval_pst(const board_t &board, int &mg, int &eg) {
    U64 pieces;
    for (int type = 1; type < 6; type++) {
        pieces = board.bb_pieces[WHITE][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg += pst[WHITE][type][sq][MG];
            eg += pst[WHITE][type][sq][EG];
        }

        pieces = board.bb_pieces[BLACK][type];
        while (pieces) {
            uint8_t sq = pop_bit(pieces);
            mg -= pst[BLACK][type][sq][MG];
            eg -= pst[BLACK][type][sq][EG];
        }
    }
}

evaluator_t::pawn_entry_t* evaluator_t::eval_pawns(const board_t &board) {
    // Return the entry if found
    size_t index = (board.record[board.now].pawn_hash & pawn_hash_entries) * bucket_size;
    pawn_entry_t *bucket = pawn_hash_table + index;
    pawn_entry_t *entry = bucket;
    for(size_t i = 0; i < bucket_size; i++) {
        if((bucket + i)->hash == board.record[board.now].pawn_hash) {
            (bucket + i)->hits++;
            return bucket + i;
        }

        if((bucket + i)->hits < entry->hits) {
            entry = bucket + i;
        }
    }

    // Set up entry
    entry->hash = board.record[board.now].pawn_hash;
    entry->hits = 1;
    entry->eval_mg = 0;
    entry->eval_eg = 0;

    // Find open files
    U64 all_pawns = board.bb_pieces[WHITE][PAWN];
    for(uint8_t file = 0; file < 8; file++) {
        if((all_pawns & file_mask(file)) == 0) {
            entry->open_files |= file_mask(file);
        }
    }

    // Evaluate
    U64 pawns;
    pawns = board.bb_pieces[WHITE][PAWN];
    while (pawns) {
        uint8_t sq = pop_bit(pawns);
        entry->eval_mg += pst[WHITE][PAWN][sq][MG];
        entry->eval_eg += pst[WHITE][PAWN][sq][EG];

        U64 caps = pawn_caps(WHITE, sq);
        entry->defended[WHITE] |= caps;
        entry->attackable[WHITE] |= BB_HOLE[WHITE][sq];

        if(caps & board.bb_pieces[WHITE][PAWN]) {
            entry->eval_mg += params.chain_mg[rank_index(sq) - 1];
            entry->eval_eg += params.chain_eg[rank_index(sq) - 1];
        }

        bool open_file = (BB_IN_FRONT[WHITE][sq] & board.bb_pieces[BLACK][PAWN]) == 0;

        if(bb_normal_moves::pawn_moves_x1[WHITE][sq] & board.bb_side[WHITE]) {
            entry->eval_mg += params.blocked_mg[open_file];
            entry->eval_eg += params.blocked_eg[open_file];
        }

        U64 not_passer = BB_PASSED[WHITE][sq] & board.bb_pieces[BLACK][PAWN];
        if(!not_passer) {
            entry->eval_mg += params.passed_mg[rank_index(sq) - 1];
            entry->eval_eg += params.passed_eg[rank_index(sq) - 1];

            entry->passers[WHITE] |= single_bit(sq);
        }

        U64 not_isolated = BB_ISOLATED[WHITE][sq] & board.bb_pieces[WHITE][PAWN]; // Friendly pawns
        if (!not_isolated) {
            entry->eval_mg += params.isolated_mg[rank_index(sq) - 1][open_file];
            entry->eval_eg += params.isolated_eg[rank_index(sq) - 1][open_file];
        }

        if(BB_IN_FRONT[WHITE][sq] & board.bb_pieces[WHITE][PAWN]) {
            entry->eval_mg += params.doubled_mg[open_file];
            entry->eval_eg += params.doubled_eg[open_file];
        }
    }

    pawns = board.bb_pieces[BLACK][PAWN];
    while (pawns) {
        uint8_t sq = pop_bit(pawns);
        entry->eval_mg -= pst[BLACK][PAWN][sq][MG];
        entry->eval_eg -= pst[BLACK][PAWN][sq][EG];

        U64 caps = pawn_caps(BLACK, sq);
        entry->defended[BLACK] |= caps;
        entry->attackable[BLACK] |= BB_HOLE[BLACK][sq];

        if(caps & board.bb_pieces[BLACK][PAWN]) {
            entry->eval_mg -= params.chain_mg[(7 - rank_index(sq)) - 1];
            entry->eval_eg -= params.chain_eg[(7 - rank_index(sq)) - 1];
        }

        bool open_file = (BB_IN_FRONT[BLACK][sq] & board.bb_pieces[WHITE][PAWN]) == 0;

        if(bb_normal_moves::pawn_moves_x1[BLACK][sq] & board.bb_side[BLACK]) {
            entry->eval_mg -= params.blocked_mg[open_file];
            entry->eval_eg -= params.blocked_eg[open_file];
        }

        U64 not_passer = BB_PASSED[BLACK][sq] & board.bb_pieces[WHITE][PAWN];
        if(!not_passer) {
            entry->eval_mg -= params.passed_mg[(7 - rank_index(sq)) - 1];
            entry->eval_eg -= params.passed_eg[(7 - rank_index(sq)) - 1];

            entry->passers[BLACK] |= single_bit(sq);
        }

        U64 not_isolated = BB_ISOLATED[BLACK][sq] & board.bb_pieces[BLACK][PAWN]; // Friendly pawns
        if (!not_isolated) {
            entry->eval_mg -= params.isolated_mg[(7 - rank_index(sq)) - 1][open_file];
            entry->eval_eg -= params.isolated_eg[(7 - rank_index(sq)) - 1][open_file];
        }

        if(BB_IN_FRONT[BLACK][sq] & board.bb_pieces[BLACK][PAWN]) {
            entry->eval_mg -= params.doubled_mg[open_file];
            entry->eval_eg -= params.doubled_eg[open_file];
        }
    }

    return entry;
}

void evaluator_t::eval_moves(const board_t &board, int &mg, int &eg,
        const pawn_entry_t *entry, int &king_danger_w, int &king_danger_b) {
    U64 pieces;
    const U64 not_white = ~board.bb_side[WHITE];
    const U64 not_black = ~board.bb_side[BLACK];
    const U64 undefended_white = ~entry->defended[WHITE];
    const U64 undefended_black = ~entry->defended[BLACK];

    U64 span[2] = {};

    U64 king_circle[2] = {BB_KING_CIRCLE[bit_scan(board.bb_pieces[WHITE][KING])],
            BB_KING_CIRCLE[bit_scan(board.bb_pieces[BLACK][KING])]};
    
    /// KNIGHTS
    pieces = board.bb_pieces[WHITE][KNIGHT];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<KNIGHT>(WHITE, sq, board.bb_all) & not_white & undefended_black;
        span[WHITE] |= moves;
        int move_count = pop_count(moves);
        mg += params.n_mob_mg[move_count];
        eg += params.n_mob_eg[move_count];

        if(moves & king_circle[WHITE]) king_danger_w += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[BLACK]) king_danger_b += params.kat_attack_value[KNIGHT];
    }
    
    pieces = board.bb_pieces[BLACK][KNIGHT];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<KNIGHT>(BLACK, sq, board.bb_all) & not_black & undefended_white;
        span[BLACK] |= moves;
        int move_count = pop_count(moves);
        mg -= params.n_mob_mg[move_count];
        eg -= params.n_mob_eg[move_count];

        if(moves & king_circle[BLACK]) king_danger_b += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[WHITE]) king_danger_w += params.kat_attack_value[KNIGHT];
    }

    /// BISHOPS
    pieces = board.bb_pieces[WHITE][BISHOP];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<BISHOP>(WHITE, sq, board.bb_all) & not_white & undefended_black;
        span[WHITE] |= moves;
        int move_count = pop_count(moves);
        mg += params.b_mob_mg[move_count];
        eg += params.b_mob_eg[move_count];

        if(moves & king_circle[WHITE]) king_danger_w += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[BLACK]) king_danger_b += params.kat_attack_value[KNIGHT];
    }

    pieces = board.bb_pieces[BLACK][BISHOP];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<BISHOP>(BLACK, sq, board.bb_all) & not_black & undefended_white;
        span[BLACK] |= moves;
        int move_count = pop_count(moves);
        mg -= params.b_mob_mg[move_count];
        eg -= params.b_mob_eg[move_count];

        if(moves & king_circle[BLACK]) king_danger_b += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[WHITE]) king_danger_w += params.kat_attack_value[KNIGHT];
    }

    /// ROOKS
    pieces = board.bb_pieces[WHITE][ROOK];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<ROOK>(WHITE, sq, board.bb_all) & not_white & undefended_black;
        span[WHITE] |= moves;
        int move_count = pop_count(moves);
        mg += params.r_mob_mg[move_count];
        eg += params.r_mob_eg[move_count];

        if(moves & king_circle[WHITE]) king_danger_w += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[BLACK]) king_danger_b += params.kat_attack_value[KNIGHT];
    }

    pieces = board.bb_pieces[BLACK][ROOK];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<ROOK>(BLACK, sq, board.bb_all) & not_black & undefended_white;
        span[BLACK] |= moves;
        int move_count = pop_count(moves);
        mg -= params.r_mob_mg[move_count];
        eg -= params.r_mob_eg[move_count];

        if(moves & king_circle[BLACK]) king_danger_b += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[WHITE]) king_danger_w += params.kat_attack_value[KNIGHT];
    }

    /// QUEENS
    pieces = board.bb_pieces[WHITE][QUEEN];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<QUEEN>(WHITE, sq, board.bb_all) & not_white & undefended_black;
        span[WHITE] |= moves;
        int move_count = pop_count(moves);
        mg += params.q_mob_mg[move_count];
        eg += params.q_mob_eg[move_count];

        if(moves & king_circle[WHITE]) king_danger_w += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[BLACK]) king_danger_b += params.kat_attack_value[KNIGHT];
    }

    pieces = board.bb_pieces[BLACK][QUEEN];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);

        U64 moves = find_moves<QUEEN>(BLACK, sq, board.bb_all) & not_black & undefended_white;
        span[BLACK] |= moves;
        int move_count = pop_count(moves);
        mg -= params.q_mob_mg[move_count];
        eg -= params.q_mob_eg[move_count];

        if(moves & king_circle[BLACK]) king_danger_b += params.kat_defend_value[KNIGHT];
        if(moves & king_circle[WHITE]) king_danger_w += params.kat_attack_value[KNIGHT];
    }

    /// SPAN
    int span_w = pop_count(span[WHITE]);
    mg += params.mob_span_mg[std::min(span_w, 31)];
    eg += params.mob_span_eg[std::min(span_w, 31)];

    int span_b = pop_count(span[BLACK]);
    mg -= params.mob_span_mg[std::min(span_b, 31)];
    eg -= params.mob_span_eg[std::min(span_b, 31)];
}

void evaluator_t::eval_king_safety(const board_t &board, int &mg, int &eg, const evaluator_t::pawn_entry_t *entry,
                                   int &king_danger_w, int &king_danger_b) {
    U64 king_circle[2] = {BB_KING_CIRCLE[bit_scan(board.bb_pieces[WHITE][KING])],
                          BB_KING_CIRCLE[bit_scan(board.bb_pieces[BLACK][KING])]};

    if(board.bb_pieces[BLACK][ROOK] != 0 && (king_circle[WHITE] & entry->open_files) != 0) {
        king_danger_w += params.kat_open_file;
    }
    if(board.bb_pieces[WHITE][ROOK] != 0 && (king_circle[BLACK] & entry->open_files) != 0) {
        king_danger_b += params.kat_open_file;
    }

    mg += params.ks_pawn_shield[std::min(3, pop_count(king_circle[WHITE] & board.bb_pieces[WHITE][PAWN]))];
    mg -= params.ks_pawn_shield[std::min(3, pop_count(king_circle[BLACK] & board.bb_pieces[BLACK][PAWN]))];
}

