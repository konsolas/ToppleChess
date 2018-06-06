//
// Created by Vincent on 30/09/2017.
//

#include "board.h"
#include <sstream>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation datatypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Eval metadata
struct eval_data_t {
    U64 king_shield[2] = {0};
    int king_danger_balance[2] = {0, 0};

    U64 attacked_by_pawn[2] = {0};
    U64 attackable_by_pawn[2] = {0};
    uint8_t counts[2][6] = {{0}};
};

// Evaluation score (middlegame and endgame)
struct score_t {
    int mg;
    int eg;

    inline score_t operator+(const score_t &rhs) const {
        return score_t{this->mg + rhs.mg, this->eg + rhs.eg};
    }

    inline score_t operator-(const score_t &rhs) const {
        return score_t{this->mg - rhs.mg, this->eg - rhs.eg};
    }

    inline score_t operator*(const int &scalar) const {
        return score_t{this->mg * scalar, this->eg * scalar};
    }

    inline score_t &operator+=(const score_t &rhs) {
        this->mg += rhs.mg;
        this->eg += rhs.eg;
        return *this;
    }

    inline score_t &operator-=(const score_t &rhs) {
        this->mg -= rhs.mg;
        this->eg -= rhs.eg;
        return *this;
    }
};

inline std::ostream &operator<<(std::ostream &out, const score_t &s) {
    out << "S(" << s.mg << ", " << s.eg << ")";
    return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** EVAL_BEGIN **/

/// pawns
const score_t MATERIAL[6] = {score_t{85, 115}, score_t{300, 280}, score_t{305, 300}, score_t{490, 500},
                             score_t{925, 975}}; // Piece type
const score_t PASSED[8] = {score_t{0, 0}, score_t{8, 15}, score_t{10, 25}, score_t{14, 35}, score_t{25, 60},
                           score_t{50, 90}, score_t{80, 111}, score_t{0, 0}}; // Rank of passed pawn
const score_t ISOLATED[2] = {score_t{-5, -12}, score_t{-8, -12}}; // On open file?
/* no pawns in front, and less than 2 enemy pawns in passed bitmap */
const score_t CANDIDATE[8] = {score_t{0, 0}, score_t{4, 10}, score_t{5, 12}, score_t{7, 15}, score_t{10, 25},
                              score_t{20, 40}, score_t{0, 0}, score_t{0, 0}}; // Rank of candidate passed pawn
const score_t DOUBLED[2] = {score_t{-10, -20}, score_t{-7, -11}}; // On open file?

/// other postional
const score_t BISHOP_PAIR = score_t{15, 25};

const score_t ROOK_SEMI_OPEN_FILE = score_t{21, 12};
const score_t ROOK_OPEN_FILE = score_t{29, 12};

const score_t CENTRE_CONTROL[6] = {score_t{10, 0}, score_t{2, 3}, score_t{2, 3}, score_t{1, 3},
                                   score_t{-20, 40}}; // Piece type

/// mobility
const score_t MOBILITY[6] = {score_t{0, 10}, score_t{3, 5}, score_t{1, 3}, score_t{1, 10}, score_t{0, 1},
                             score_t{0, 0}}; // Piece type

/// king safety
const int KING_ATTACKER_WEIGHT[6] = {1, 2, 3, 5, 6};
const int KING_DEFENDER_WEIGHT[6] = {6, 4, 4, 4, 7};
const int KING_MIN_WEIGHT = -1000;

/* linear king safety function */
const score_t KING_DANGER_M = score_t{2, 3};
const score_t KING_DANGER_C = score_t{-26, -16};

/// tempo
const int TEMPO = 5;

/** EVAL_END **/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utility tables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

U64 BB_PASSED[2][64] = {{0}};
U64 BB_ISOLATED[2][64] = {{0}};
U64 BB_IN_FRONT[2][64] = {{0}};
U64 BB_HOLE[2][64] = {{0}};

U64 BB_KING_CIRCLE[64] = {0};

U64 BB_CENTRE = 0;

void eval_init() {
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
        if (file_index(sq) <= 5 && file_index(sq) >= 2 && rank_index(sq) <= 4 && rank_index(sq) >= 3) {
            BB_CENTRE |= single_bit(sq);
        }

        // King circle
        BB_KING_CIRCLE[sq] |= find_moves(KING, WHITE, sq, 0);
        BB_KING_CIRCLE[sq] |= find_moves(KNIGHT, WHITE, sq, 0);
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main evaluation functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

score_t eval_pawns(Team side, const board_t &board, eval_data_t &dat);
score_t eval_knights(Team side, const board_t &board, eval_data_t &dat);
score_t eval_bishops(Team side, const board_t &board, eval_data_t &dat);
score_t eval_rooks(Team side, const board_t &board, eval_data_t &dat);
score_t eval_queens(Team side, const board_t &board, eval_data_t &dat);
score_t eval_kings(Team side, const board_t &board, eval_data_t &dat);

int eval(const board_t &board) {
    score_t eval = {0, 0};
    eval_data_t dat = {0};

    {
        dat.king_shield[WHITE] = BB_KING_CIRCLE[bit_scan(board.bb_pieces[WHITE][KING])];
        dat.king_shield[BLACK] = BB_KING_CIRCLE[bit_scan(board.bb_pieces[BLACK][KING])];
    }

    eval += eval_pawns(WHITE, board, dat) - eval_pawns(BLACK, board, dat);
    eval += eval_knights(WHITE, board, dat) - eval_knights(BLACK, board, dat);
    eval += eval_bishops(WHITE, board, dat) - eval_bishops(BLACK, board, dat);
    eval += eval_rooks(WHITE, board, dat) - eval_rooks(BLACK, board, dat);
    eval += eval_queens(WHITE, board, dat) - eval_queens(BLACK, board, dat);
    eval += eval_kings(WHITE, board, dat) - eval_kings(BLACK, board, dat);
    
    int taperedPhase;
    {
        int val[5] = {0, 1, 1, 2, 4};
        int total = val[PAWN] * 16 + val[KNIGHT] * 4 + val[BISHOP] * 4 + val[ROOK] * 4 + val[QUEEN] * 2;

        taperedPhase = total;

        for (int piece = 0; piece < 5; piece++) {
            taperedPhase -= val[piece] * (dat.counts[0][piece] + dat.counts[1][piece]);
        }

        taperedPhase = (taperedPhase * 256 + (total / 2)) / total;
    }

    int finalScore = ((eval.mg * (256 - taperedPhase)) + (eval.eg * taperedPhase)) / 256;

    return board.record[board.now].next_move ? -finalScore - TEMPO : +finalScore + TEMPO;
}

score_t eval_pawns(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 pawns = board.bb_pieces[side][PAWN];
    while (pawns) {
        uint8_t sq = pop_bit(side, pawns);

        dat.counts[side][PAWN]++;

        // Material
        score += MATERIAL[PAWN];

        U64 moves = find_moves(PAWN, side, sq, 0xffffffffffffffff);
        dat.attacked_by_pawn[side] |= moves;

        // Update holes bitmap
        dat.attackable_by_pawn[side] |= BB_HOLE[side][sq];

        // Mobility
        score += MOBILITY[PAWN] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= pop_count(single_bit(sq) & dat.king_shield[side]) * KING_DEFENDER_WEIGHT[PAWN];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[PAWN];

        // Centre control
        score += CENTRE_CONTROL[PAWN] * pop_count(single_bit(sq) & BB_CENTRE);

        // Passed?
        U64 passed = BB_PASSED[side][sq] & board.bb_pieces[xside][PAWN];
        if (!passed) {
            score += PASSED[side ? 7 - rank_index(sq) : rank_index(sq)];
        }

        // Candidate?
        U64 in_front = BB_IN_FRONT[side][sq] & board.bb_pieces[xside][PAWN];
        if (passed && !in_front && pop_count(passed) < 2) {
            score += CANDIDATE[side ? 7 - rank_index(sq) : rank_index(sq)];
        }

        // Isolated?
        U64 isolated = BB_ISOLATED[side][sq] & board.bb_pieces[side][PAWN]; // Friendly pawns
        if (!isolated) {
            score += ISOLATED[!in_front]; // in_front is nonzero if closed file
        }

        // Doubled?
        if (BB_IN_FRONT[side][sq] & board.bb_pieces[side][PAWN]) { // Friendly pawns
            score += DOUBLED[!in_front];
        }
    }

    return score;
}

score_t eval_knights(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 knights = board.bb_pieces[side][KNIGHT];
    while (knights) {
        uint8_t sq = pop_bit(side, knights);

        dat.counts[side][KNIGHT]++;

        // Material
        score += MATERIAL[KNIGHT];

        U64 moves = find_moves(KNIGHT, side, sq, board.bb_all) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[KNIGHT] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[KNIGHT];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[KNIGHT];


        // Centre control
        score += CENTRE_CONTROL[KNIGHT] * pop_count(moves & BB_CENTRE);
    }

    return score;
}

score_t eval_bishops(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 bishops = board.bb_pieces[side][BISHOP];
    while (bishops) {
        uint8_t sq = pop_bit(side, bishops);

        dat.counts[side][BISHOP]++;

        // Material
        score += MATERIAL[BISHOP];

        U64 moves = find_moves(BISHOP, side, sq, board.bb_side[side]) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[BISHOP] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[BISHOP];
        dat.king_danger_balance[xside] += ((moves & dat.king_shield[xside]) != 0) * KING_ATTACKER_WEIGHT[BISHOP];


        // Centre control
        score += CENTRE_CONTROL[BISHOP] * pop_count(moves & BB_CENTRE);
    }

    if (dat.counts[side][BISHOP] >= 2) {
        score += BISHOP_PAIR;
    }

    return score;
}

score_t eval_rooks(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 rooks = board.bb_pieces[side][ROOK];
    while (rooks) {
        uint8_t sq = pop_bit(side, rooks);

        dat.counts[side][ROOK]++;

        score += MATERIAL[ROOK];

        U64 moves = find_moves(ROOK, side, sq, board.bb_side[side]) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[ROOK] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[ROOK];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[ROOK];

        // Centre control
        score += CENTRE_CONTROL[ROOK] * pop_count(moves & BB_CENTRE);

        // Open file
        if (!(BB_IN_FRONT[side][sq] & board.bb_pieces[side][PAWN])) {
            if (!(BB_IN_FRONT[side][sq] & board.bb_pieces[xside][PAWN])) {
                score += ROOK_OPEN_FILE;
            } else {
                score += ROOK_SEMI_OPEN_FILE;
            }
        }
    }

    return score;
}

score_t eval_queens(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 queens = board.bb_pieces[side][QUEEN];
    while (queens) {
        uint8_t sq = pop_bit(side, queens);

        dat.counts[side][QUEEN]++;

        score += MATERIAL[QUEEN];

        U64 moves = find_moves(QUEEN, side, sq, board.bb_all) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[QUEEN] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[QUEEN];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[QUEEN];

        // Centre control
        score += CENTRE_CONTROL[QUEEN] * pop_count(moves & BB_CENTRE);

    }

    return score;
}

score_t eval_kings(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};

    int balance = std::max(dat.king_danger_balance[side], KING_MIN_WEIGHT);
    score -= KING_DANGER_M * balance + KING_DANGER_C;

    score.eg = std::max(score.eg, 0);
    score.mg = std::max(score.mg, 0);

    return score;
}
