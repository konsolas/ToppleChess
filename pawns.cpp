//
// Created by Vincent on 01/06/2019.
//

#include "pawns.h"
#include "eval.h"

pawns::structure_t::structure_t(const processed_params_t &params, U64 kp_hash, U64 w_pawns, U64 b_pawns, U64 w_king, U64 b_king)
        : hash(kp_hash), eval_mg(0), eval_eg(0) {
    // Find pawns
    U64 open[2] = {pawns::open_pawns<WHITE>(w_pawns, b_pawns), pawns::open_pawns<BLACK>(b_pawns, w_pawns)};
    U64 isolated[2] = {pawns::isolated(w_pawns), pawns::isolated(b_pawns)};
    U64 backwards[2] = {pawns::backward<WHITE>(w_pawns, b_pawns), pawns::backward<BLACK>(b_pawns, w_pawns)};
    U64 doubled[2] = {pawns::doubled<WHITE>(w_pawns), pawns::doubled<BLACK>(b_pawns)};
    U64 chain[2] = {pawns::attacks<WHITE>(w_pawns) & w_pawns, pawns::attacks<BLACK>(b_pawns) & b_pawns};
    U64 passed[2] = {pawns::passed<WHITE>(w_pawns, b_pawns), pawns::passed<BLACK>(b_pawns, w_pawns)};
    U64 candidate[2] = {pawns::candidates<WHITE>(w_pawns, b_pawns), pawns::candidates<BLACK>(b_pawns, w_pawns)};

    // Count isolated, backwards and doubled pawns
    int isolated_counts[2][2] = {
            {pop_count(isolated[WHITE] & ~open[WHITE]), pop_count(isolated[WHITE] & open[WHITE])},
            {pop_count(isolated[BLACK] & ~open[BLACK]), pop_count(isolated[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]
    int backwards_counts[2][2] = {
            {pop_count(backwards[WHITE] & ~open[WHITE]), pop_count(backwards[WHITE] & open[WHITE])},
            {pop_count(backwards[BLACK] & ~open[BLACK]), pop_count(backwards[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]
    int doubled_counts[2][2] = {
            {pop_count(doubled[WHITE] & ~open[WHITE]), pop_count(doubled[WHITE] & open[WHITE])},
            {pop_count(doubled[BLACK] & ~open[BLACK]), pop_count(doubled[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]

    // Add to the scores
    eval_mg += (isolated_counts[WHITE][0] - isolated_counts[BLACK][0]) * params.isolated_mg[0];
    eval_mg += (isolated_counts[WHITE][1] - isolated_counts[BLACK][1]) * params.isolated_mg[1];
    eval_eg += (isolated_counts[WHITE][0] - isolated_counts[BLACK][0]) * params.isolated_eg[0];
    eval_eg += (isolated_counts[WHITE][1] - isolated_counts[BLACK][1]) * params.isolated_eg[1];
    
    eval_mg += (backwards_counts[WHITE][0] - backwards_counts[BLACK][0]) * params.backwards_mg[0];
    eval_mg += (backwards_counts[WHITE][1] - backwards_counts[BLACK][1]) * params.backwards_mg[1];
    eval_eg += (backwards_counts[WHITE][0] - backwards_counts[BLACK][0]) * params.backwards_eg[0];
    eval_eg += (backwards_counts[WHITE][1] - backwards_counts[BLACK][1]) * params.backwards_eg[1];
    
    eval_mg += (doubled_counts[WHITE][0] - doubled_counts[BLACK][0]) * params.doubled_mg[0];
    eval_mg += (doubled_counts[WHITE][1] - doubled_counts[BLACK][1]) * params.doubled_mg[1];
    eval_eg += (doubled_counts[WHITE][0] - doubled_counts[BLACK][0]) * params.doubled_eg[0];
    eval_eg += (doubled_counts[WHITE][1] - doubled_counts[BLACK][1]) * params.doubled_eg[1];

    U64 bb;
    
    // King PST
    bb = w_king;
    while (bb) {
        uint8_t sq = pop_bit(bb);
        eval_mg += params.pst[WHITE][KING][sq][MG];
        eval_eg += params.pst[WHITE][KING][sq][EG];
    }

    bb = b_king;
    while (bb) {
        uint8_t sq = pop_bit(bb);
        eval_mg -= params.pst[BLACK][KING][sq][MG];
        eval_eg -= params.pst[BLACK][KING][sq][EG];
    }

    // Pawn PST
    bb = w_pawns;
    while(bb) {
        uint8_t sq = pop_bit(bb);
        eval_mg += params.pst[WHITE][PAWN][sq][MG];
        eval_eg += params.pst[WHITE][PAWN][sq][EG];
    }

    bb = b_pawns;
    while(bb) {
        uint8_t sq = pop_bit(bb);
        eval_mg -= params.pst[BLACK][PAWN][sq][MG];
        eval_eg -= params.pst[BLACK][PAWN][sq][EG];
    }

    // Chain
    bb = chain[WHITE];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        eval_mg += params.chain_mg[rank - 2];
        eval_eg += params.chain_eg[rank - 2];
    }

    bb = chain[BLACK];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        eval_mg -= params.chain_mg[rank - 2];
        eval_eg -= params.chain_eg[rank - 2];
    }

    // Passed
    bb = passed[WHITE];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        eval_mg += params.passed_mg[rank - 1];
        eval_eg += params.passed_eg[rank - 1];
    }

    bb = passed[BLACK];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        eval_mg -= params.passed_mg[rank - 1];
        eval_eg -= params.passed_eg[rank - 1];
    }

    // Candidates
    bb = candidate[WHITE];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        eval_mg += params.candidate_mg[rank - 1];
        eval_eg += params.candidate_eg[rank - 1];
    }

    bb = candidate[BLACK];
    while(bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        eval_mg -= params.candidate_mg[rank - 1];
        eval_eg -= params.candidate_eg[rank - 1];
    }
}

void pawns::structure_t::eval_dynamic(const processed_params_t &params, const board_t &board, int &mg, int &eg) const {
    // These should only be done setwise - branches should be avoided whenever possible for the sake of performance
    U64 w_pawns = board.bb_pieces[WHITE][PAWN];
    U64 b_pawns = board.bb_pieces[BLACK][PAWN];

    U64 open_files = pawns::open_files(w_pawns, b_pawns);
    U64 half_open_files[2] = {
            pawns::half_or_open_files(w_pawns) ^ open_files,
            pawns::half_or_open_files(b_pawns) ^ open_files
    };

    // Blocked pawns
    U64 stop_squares[2] = {pawns::stop_squares<WHITE>(w_pawns), pawns::stop_squares<BLACK>(b_pawns)};
    int blocked_count[2] = {pop_count(stop_squares[WHITE] & board.bb_all), pop_count(stop_squares[BLACK] & board.bb_all)};

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

    // Summing up
    mg += (blocked_count[WHITE] - blocked_count[BLACK]) * params.blocked_mg;
    eg += (blocked_count[WHITE] - blocked_count[BLACK]) * params.blocked_eg;

    mg += (open_file_count[WHITE] - open_file_count[BLACK]) * params.pos_r_open_file_mg;
    mg += (own_half_open_file_count[WHITE] - own_half_open_file_count[BLACK]) * params.pos_r_own_half_open_file_mg;
    mg += (other_half_open_file_count[WHITE] - other_half_open_file_count[BLACK]) * params.pos_r_other_half_open_file_mg;
    eg += (open_file_count[WHITE] - open_file_count[BLACK]) * params.pos_r_open_file_eg;
    eg += (own_half_open_file_count[WHITE] - own_half_open_file_count[BLACK]) * params.pos_r_own_half_open_file_eg;
    eg += (other_half_open_file_count[WHITE] - other_half_open_file_count[BLACK]) * params.pos_r_other_half_open_file_eg;
}
