//
// Created by Vincent on 01/06/2019.
//

#include "pawns.h"
#include "eval.h"

pawns::structure_t::structure_t(const processed_params_t &params, U64 kp_hash, U64 w_pawns, U64 b_pawns, U64 w_king,
                                U64 b_king)
        : hash(kp_hash) {
    v4si_t score = {0, 0, 0, 0};
    
    // Find pawns
    U64 open[2] = {pawns::open_pawns<WHITE>(w_pawns, b_pawns), pawns::open_pawns<BLACK>(b_pawns, w_pawns)};
    U64 isolated[2] = {pawns::isolated(w_pawns), pawns::isolated(b_pawns)};
    U64 backwards[2] = {pawns::backward<WHITE>(w_pawns, b_pawns), pawns::backward<BLACK>(b_pawns, w_pawns)};
    U64 semi_backwards[2] = {pawns::semi_backward<WHITE>(w_pawns, b_pawns),
                             pawns::semi_backward<BLACK>(b_pawns, w_pawns)};
    U64 paired[2] = {pawns::paired(w_pawns), pawns::paired(b_pawns)};
    U64 detached[2] = {pawns::detached<WHITE>(w_pawns, b_pawns), pawns::detached<BLACK>(b_pawns, w_pawns)};
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
    int semi_backwards_counts[2][2] = {
            {pop_count(semi_backwards[WHITE] & ~open[WHITE]), pop_count(semi_backwards[WHITE] & open[WHITE])},
            {pop_count(semi_backwards[BLACK] & ~open[BLACK]), pop_count(semi_backwards[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]
    int paired_counts[2][2] = {
            {pop_count(paired[WHITE] & ~open[WHITE]), pop_count(paired[WHITE] & open[WHITE])},
            {pop_count(paired[BLACK] & ~open[BLACK]), pop_count(paired[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]
    int detached_counts[2][2] = {
            {pop_count(detached[WHITE] & ~open[WHITE]), pop_count(detached[WHITE] & open[WHITE])},
            {pop_count(detached[BLACK] & ~open[BLACK]), pop_count(detached[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]
    int doubled_counts[2][2] = {
            {pop_count(doubled[WHITE] & ~open[WHITE]), pop_count(doubled[WHITE] & open[WHITE])},
            {pop_count(doubled[BLACK] & ~open[BLACK]), pop_count(doubled[BLACK] & open[BLACK])}
    }; // [TEAM][OPEN]

    // Add to the scores
    score += (isolated_counts[WHITE][0] - isolated_counts[BLACK][0]) * params.isolated[0];
    score += (isolated_counts[WHITE][1] - isolated_counts[BLACK][1]) * params.isolated[1];

    score += (backwards_counts[WHITE][0] - backwards_counts[BLACK][0]) * params.backwards[0];
    score += (backwards_counts[WHITE][1] - backwards_counts[BLACK][1]) * params.backwards[1];

    score += (semi_backwards_counts[WHITE][0] - semi_backwards_counts[BLACK][0]) * params.semi_backwards[0];
    score += (semi_backwards_counts[WHITE][1] - semi_backwards_counts[BLACK][1]) * params.semi_backwards[1];

    score += (paired_counts[WHITE][0] - paired_counts[BLACK][0]) * params.paired[0];
    score += (paired_counts[WHITE][1] - paired_counts[BLACK][1]) * params.paired[1];

    score += (detached_counts[WHITE][0] - detached_counts[BLACK][0]) * params.detached[0];
    score += (detached_counts[WHITE][1] - detached_counts[BLACK][1]) * params.detached[1];

    score += (doubled_counts[WHITE][0] - doubled_counts[BLACK][0]) * params.doubled[0];
    score += (doubled_counts[WHITE][1] - doubled_counts[BLACK][1]) * params.doubled[1];

    U64 bb;

    // King PST
    int king_loc[2] = {bit_scan(w_king), bit_scan(b_king)};
    score += params.pst[WHITE][KING][king_loc[WHITE]];
    score -= params.pst[BLACK][KING][king_loc[BLACK]];

    // Pawn PST
    bb = w_pawns;
    while (bb) {
        uint8_t sq = pop_bit(bb);
        score += params.pst[WHITE][PAWN][sq];

        score += distance(king_loc[WHITE], sq) * params.king_tropism[0];
        score += distance(king_loc[BLACK], sq) * params.king_tropism[1];
    }

    bb = b_pawns;
    while (bb) {
        uint8_t sq = pop_bit(bb);
        score -= params.pst[BLACK][PAWN][sq];

        score -= distance(king_loc[BLACK], sq) * params.king_tropism[0];
        score -= distance(king_loc[WHITE], sq) * params.king_tropism[1];
    }

    // Chain
    bb = chain[WHITE];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        score += params.chain[rank - 2];
    }

    bb = chain[BLACK];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        score -= params.chain[rank - 2];
    }

    // Passed
    bb = passed[WHITE];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        score += params.passed[rank - 1];

        score += distance(king_loc[WHITE], sq) * params.passer_tropism[0];
        score += distance(king_loc[BLACK], sq) * params.passer_tropism[1];
    }

    bb = passed[BLACK];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        score -= params.passed[rank - 1];

        score -= distance(king_loc[BLACK], sq) * params.passer_tropism[0];
        score -= distance(king_loc[WHITE], sq) * params.passer_tropism[1];
    }

    // Candidates
    bb = candidate[WHITE];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(WHITE, rank_index(sq));
        score += params.candidate[rank - 1];
    }

    bb = candidate[BLACK];
    while (bb) {
        uint8_t sq = pop_bit(bb);
        uint8_t rank = rel_rank(BLACK, rank_index(sq));
        score -= params.candidate[rank - 1];
    }

    // Calculate tapering factor
    this->taper = (float) ((pop_count(w_pawns) + pop_count(b_pawns)) / 16.0);
    this->eval_mg = taper * score[0] + (1 - taper) * score[1];
    this->eval_eg = taper * score[2] + (1 - taper) * score[3];
}