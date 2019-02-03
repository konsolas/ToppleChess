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
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

evaluator_t::evaluator_t(eval_params_t params, size_t pawn_hash_size) : params(params) {
    // Set up pawn hash table
    pawn_hash_size /= (sizeof(pawn_entry_t) * bucket_size);
    this->pawn_hash_entries = tt::lower_power_of_2(pawn_hash_size) - 1;
    pawn_hash_table = new pawn_entry_t[pawn_hash_entries * bucket_size + bucket_size]();

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

    // Set up king safety evaluation table.
    for(int i = 0; i < 64; i++) {
        // Translated + scaled sigmoid function
        kat_table[i] = (int) (double(params.kat_table_max) /
                (1 + exp((params.kat_table_translate - i) / double(params.kat_table_scale)))) - params.kat_table_offset;
        if(i > 0 && kat_table[i] == kat_table[i-1]) {
            std::cout << "duplicate in kat_table, i=" << i << std::endl;
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

    pawn_entry_t *pawn_data = eval_pawns(board);
    mg += pawn_data->eval_mg;
    eg += pawn_data->eval_eg;

    // Main evaluation functions
    double phase = eval_material(board, mg, eg);
    eval_pst(board, mg, eg);
    eval_king_safety(board, mg, eg, pawn_data);
    eval_positional(board, mg, eg, pawn_data);

    // Interpolate between middlegame and endgame scores
    auto total = int(phase * mg + (1 - phase) * eg);

    return board.record[board.now].next_move ? -total : total;
}

double evaluator_t::eval_material(const board_t &board, int &mg, int &eg) {
    // Check for opposite coloured bishops
    bool opposite_coloured_bishops = board.bb_pieces[WHITE][BISHOP] && board.bb_pieces[BLACK][BISHOP]
        && !multiple_bits(board.bb_pieces[WHITE][BISHOP]) && !multiple_bits(board.bb_pieces[BLACK][BISHOP])
        && !same_colour(bit_scan(board.bb_pieces[WHITE][BISHOP]), bit_scan(board.bb_pieces[BLACK][BISHOP]));

    // Collect material data
    const int pawn_w = pop_count(board.bb_pieces[WHITE][PAWN]); 
    const int pawn_b = pop_count(board.bb_pieces[BLACK][PAWN]);
    const int pawn_balance = pawn_w - pawn_b;
    mg += params.mat_mg[PAWN] * pawn_balance;
    eg += params.mat_eg[PAWN] * pawn_balance;

    if(opposite_coloured_bishops) {
        if(pawn_balance > 0) {
            eg -= params.mat_opp_bishop[std::min(2, pawn_balance - 1)];
        } else if(pawn_balance < 0) {
            eg += params.mat_opp_bishop[std::min(2, -pawn_balance - 1)];
        }
    }

    const int knight_w = pop_count(board.bb_pieces[WHITE][KNIGHT]);
    const int knight_b = pop_count(board.bb_pieces[BLACK][KNIGHT]);
    const int knight_balance = knight_w - knight_b;
    mg += params.mat_mg[KNIGHT] * knight_balance;
    eg += params.mat_eg[KNIGHT] * knight_balance;

    const int bishop_w = pop_count(board.bb_pieces[WHITE][BISHOP]);
    const int bishop_b = pop_count(board.bb_pieces[BLACK][BISHOP]);
    const int bishop_balance = bishop_w - bishop_b;
    mg += params.mat_mg[BISHOP] * bishop_balance;
    eg += params.mat_eg[BISHOP] * bishop_balance;
    
    const int rook_w = pop_count(board.bb_pieces[WHITE][ROOK]);
    const int rook_b = pop_count(board.bb_pieces[BLACK][ROOK]);
    const int rook_balance = rook_w - rook_b;    
    mg += params.mat_mg[ROOK] * rook_balance;
    eg += params.mat_eg[ROOK] * rook_balance;

    const int queen_w = pop_count(board.bb_pieces[WHITE][QUEEN]);
    const int queen_b = pop_count(board.bb_pieces[BLACK][QUEEN]);
    const int queen_balance = queen_w - queen_b;
    mg += params.mat_mg[QUEEN] * queen_balance;
    eg += params.mat_eg[QUEEN] * queen_balance;

    const int mat_w = params.mat_exch_pawn * (pawn_w)
                      + params.mat_exch_minor * (knight_w + bishop_w)
                      + params.mat_exch_rook * (rook_w)
                      + params.mat_exch_queen * (queen_w);
    const int mat_b = params.mat_exch_pawn * (pawn_b)
                      + params.mat_exch_minor * (knight_b + bishop_b)
                      + params.mat_exch_rook * (rook_b)
                      + params.mat_exch_queen * (queen_b);
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
    U64 all_pawns = board.bb_pieces[WHITE][PAWN] | board.bb_pieces[BLACK][PAWN];
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
            entry->eval_mg += params.chain_mg[rel_rank(WHITE, rank_index(sq)) - 1];
            entry->eval_eg += params.chain_eg[rel_rank(WHITE, rank_index(sq)) - 1];
        }

        bool open_file = (BB_IN_FRONT[WHITE][sq] & board.bb_pieces[BLACK][PAWN]) == 0;

        if(bb_normal_moves::pawn_moves_x1[WHITE][sq] & board.bb_side[WHITE]) {
            entry->eval_mg += params.blocked_mg[open_file];
            entry->eval_eg += params.blocked_eg[open_file];
        }

        U64 not_passer = BB_PASSED[WHITE][sq] & board.bb_pieces[BLACK][PAWN];
        if(!not_passer) {
            entry->eval_mg += params.passed_mg[rel_rank(WHITE, rank_index(sq)) - 1];
            entry->eval_eg += params.passed_eg[rel_rank(WHITE, rank_index(sq)) - 1];

            entry->passers[WHITE] |= single_bit(sq);
        }

        U64 backwards = BB_BACKWARDS[WHITE][sq] & board.bb_pieces[WHITE][PAWN];
        if(!backwards && (board.bb_pieces[BLACK][PAWN] & pawn_caps(WHITE, uint8_t(sq + rel_offset(WHITE, D_N))))) {
            entry->eval_mg += params.backwards_mg[open_file];
            entry->eval_eg += params.backwards_eg[open_file];
        }

        U64 not_isolated = BB_ISOLATED[WHITE][sq] & board.bb_pieces[WHITE][PAWN]; // Friendly pawns
        if (!not_isolated) {
            entry->eval_mg += params.isolated_mg[rel_rank(WHITE, rank_index(sq)) - 1][open_file];
            entry->eval_eg += params.isolated_eg[rel_rank(WHITE, rank_index(sq)) - 1][open_file];
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
            entry->eval_mg -= params.chain_mg[rel_rank(BLACK, rank_index(sq)) - 1];
            entry->eval_eg -= params.chain_eg[rel_rank(BLACK, rank_index(sq)) - 1];
        }

        bool open_file = (BB_IN_FRONT[BLACK][sq] & board.bb_pieces[WHITE][PAWN]) == 0;

        if(bb_normal_moves::pawn_moves_x1[BLACK][sq] & board.bb_side[BLACK]) {
            entry->eval_mg -= params.blocked_mg[open_file];
            entry->eval_eg -= params.blocked_eg[open_file];
        }

        U64 not_passer = BB_PASSED[BLACK][sq] & board.bb_pieces[WHITE][PAWN];
        if(!not_passer) {
            entry->eval_mg -= params.passed_mg[rel_rank(BLACK, rank_index(sq)) - 1];
            entry->eval_eg -= params.passed_eg[rel_rank(BLACK, rank_index(sq)) - 1];

            entry->passers[BLACK] |= single_bit(sq);
        }

        U64 backwards = BB_BACKWARDS[BLACK][sq] & board.bb_pieces[BLACK][PAWN];
        if(!backwards && (board.bb_pieces[WHITE][PAWN] & pawn_caps(BLACK, uint8_t(sq + rel_offset(BLACK, D_N))))) {
            entry->eval_mg -= params.backwards_mg[open_file];
            entry->eval_eg -= params.backwards_eg[open_file];
        }

        U64 not_isolated = BB_ISOLATED[BLACK][sq] & board.bb_pieces[BLACK][PAWN]; // Friendly pawns
        if (!not_isolated) {
            entry->eval_mg -= params.isolated_mg[rel_rank(BLACK, rank_index(sq)) - 1][open_file];
            entry->eval_eg -= params.isolated_eg[rel_rank(BLACK, rank_index(sq)) - 1][open_file];
        }

        if(BB_IN_FRONT[BLACK][sq] & board.bb_pieces[BLACK][PAWN]) {
            entry->eval_mg -= params.doubled_mg[open_file];
            entry->eval_eg -= params.doubled_eg[open_file];
        }
    }

    return entry;
}

void evaluator_t::eval_king_safety(const board_t &board, int &mg, int &eg, const evaluator_t::pawn_entry_t *entry) {
    int king_pos[2] = {bit_scan(board.bb_pieces[WHITE][KING]), bit_scan(board.bb_pieces[BLACK][KING])};
    U64 king_circle[2] = {BB_KING_CIRCLE[king_pos[WHITE]], BB_KING_CIRCLE[king_pos[BLACK]]};

    int pawn_shield_w = std::min(3, pop_count(king_circle[WHITE] & board.bb_pieces[WHITE][PAWN]));
    int pawn_shield_b = std::min(3, pop_count(king_circle[BLACK] & board.bb_pieces[BLACK][PAWN]));

    mg += params.ks_pawn_shield[pawn_shield_w];
    mg -= params.ks_pawn_shield[pawn_shield_b];
    
    // Check for attacks on the king
    int king_danger[2] = {};

    if(board.bb_pieces[BLACK][ROOK] != 0 && (king_circle[WHITE] & entry->open_files) != 0) {
        king_danger[WHITE] += params.kat_open_file;
    }
    if(board.bb_pieces[WHITE][ROOK] != 0 && (king_circle[BLACK] & entry->open_files) != 0) {
        king_danger[BLACK] += params.kat_open_file;
    }

    king_danger[WHITE] -= params.kat_defence_weight[PAWN] * pawn_shield_w;
    king_danger[WHITE] += params.kat_attack_weight[PAWN] * pop_count(entry->defended[BLACK] & king_circle[WHITE]);

    king_danger[BLACK] -= params.kat_defence_weight[PAWN] * pawn_shield_b;
    king_danger[BLACK] += params.kat_attack_weight[PAWN] * pop_count(entry->defended[WHITE] & king_circle[BLACK]);
    
    U64 knights = board.bb_pieces[WHITE][KNIGHT];
    while (knights) {
        uint8_t sq = pop_bit(knights);
        U64 moves = find_moves<KNIGHT>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[KNIGHT] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[KNIGHT] * pop_count(moves & king_circle[BLACK]);
    }

    U64 bishops = board.bb_pieces[WHITE][BISHOP];
    while (bishops) {
        uint8_t sq = pop_bit(bishops);
        U64 moves = find_moves<BISHOP>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[BISHOP] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[BISHOP] * pop_count(moves & king_circle[BLACK]);
    }

    U64 rooks = board.bb_pieces[WHITE][ROOK];
    while (rooks) {
        uint8_t sq = pop_bit(rooks);
        U64 moves = find_moves<ROOK>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[ROOK] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[ROOK] * pop_count(moves & king_circle[BLACK]);
    }

    U64 queens = board.bb_pieces[WHITE][QUEEN];
    while (queens) {
        uint8_t sq = pop_bit(queens);
        U64 moves = find_moves<QUEEN>(WHITE, sq, board.bb_all);
        king_danger[WHITE] -= params.kat_defence_weight[QUEEN] * pop_count(moves & king_circle[WHITE]);
        king_danger[BLACK] += params.kat_attack_weight[QUEEN] * pop_count(moves & king_circle[BLACK]);
    }

    knights = board.bb_pieces[BLACK][KNIGHT];
    while (knights) {
        uint8_t sq = pop_bit(knights);
        U64 moves = find_moves<KNIGHT>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[KNIGHT] * pop_count(moves & king_circle[WHITE]);
        king_danger[WHITE] += params.kat_attack_weight[KNIGHT] * pop_count(moves & king_circle[WHITE]);
    }

    bishops = board.bb_pieces[BLACK][BISHOP];
    while (bishops) {
        uint8_t sq = pop_bit(bishops);
        U64 moves = find_moves<BISHOP>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[BISHOP] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[BISHOP] * pop_count(moves & king_circle[WHITE]);
    }

    rooks = board.bb_pieces[BLACK][ROOK];
    while (rooks) {
        uint8_t sq = pop_bit(rooks);
        U64 moves = find_moves<ROOK>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[ROOK] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[ROOK] * pop_count(moves & king_circle[WHITE]);
    }

    queens = board.bb_pieces[BLACK][QUEEN];
    while (queens) {
        uint8_t sq = pop_bit(queens);
        U64 moves = find_moves<QUEEN>(BLACK, sq, board.bb_all);
        king_danger[BLACK] -= params.kat_defence_weight[QUEEN] * pop_count(moves & king_circle[BLACK]);
        king_danger[WHITE] += params.kat_attack_weight[QUEEN] * pop_count(moves & king_circle[WHITE]);
    }

    mg -= kat_table[std::min(std::max(king_danger[WHITE], 0), 63)];
    mg += kat_table[std::min(std::max(king_danger[BLACK], 0), 63)];
}

void evaluator_t::eval_positional(const board_t &board, int &mg, int &eg, const pawn_entry_t *entry) {
    if(pop_count(board.bb_pieces[WHITE][BISHOP]) >= 2) {
        mg += params.pos_bishop_pair_mg;
        eg += params.pos_bishop_pair_eg;
    }

    if(pop_count(board.bb_pieces[BLACK][BISHOP]) >= 2) {
        mg -= params.pos_bishop_pair_mg;
        eg -= params.pos_bishop_pair_eg;
    }

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

    U64 pieces = board.bb_pieces[WHITE][ROOK];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);
        if(single_bit(sq) & entry->open_files) {
            mg += params.pos_r_open_file_mg;
            eg += params.pos_r_open_file_eg;
        }

        if(BB_IN_FRONT[WHITE][sq] & entry->passers[WHITE]) {
            mg += params.pos_r_behind_own_passer_mg;
            eg += params.pos_r_behind_own_passer_eg;
        }

        if(BB_IN_FRONT[WHITE][sq] & entry->passers[BLACK]) {
            mg += params.pos_r_behind_enemy_passer_mg;
            eg += params.pos_r_behind_enemy_passer_eg;
        }

        int xray_pawn = pop_count(find_moves<ROOK>(WHITE, sq, board.bb_pieces[WHITE][PAWN]) & board.bb_pieces[BLACK][PAWN]);
        if(xray_pawn) {
            mg += params.pos_r_xray_pawn_mg * xray_pawn;
            eg += params.pos_r_xray_pawn_eg * xray_pawn;
        }
    }

    pieces = board.bb_pieces[BLACK][ROOK];
    while (pieces) {
        uint8_t sq = pop_bit(pieces);
        if(single_bit(sq) & entry->open_files) {
            mg -= params.pos_r_open_file_mg;
            eg -= params.pos_r_open_file_eg;
        }

        if(BB_IN_FRONT[BLACK][sq] & entry->passers[BLACK]) {
            mg -= params.pos_r_behind_own_passer_mg;
            eg -= params.pos_r_behind_own_passer_eg;
        }

        if(BB_IN_FRONT[BLACK][sq] & entry->passers[WHITE]) {
            mg -= params.pos_r_behind_enemy_passer_mg;
            eg -= params.pos_r_behind_enemy_passer_eg;
        }

        int xray_pawn = pop_count(find_moves<ROOK>(BLACK, sq, board.bb_pieces[BLACK][PAWN]) & board.bb_pieces[WHITE][PAWN]);
        if(xray_pawn) {
            mg -= params.pos_r_xray_pawn_mg * xray_pawn;
            eg -= params.pos_r_xray_pawn_eg * xray_pawn;
        }
    }
}

