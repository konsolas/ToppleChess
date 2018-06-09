//
// Created by Vincent on 30/09/2017.
//

#include "board.h"
#include "endgame.h"
#include <sstream>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation datatypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Eval metadata
struct eval_data_t {
    uint8_t king_loc[2];

    U64 king_shield[2] = {0};
    int king_danger_balance[2] = {0, 0};

    U64 attacked_by_pawn[2] = {0};
    U64 attackable_by_pawn[2] = {0};
    uint8_t counts[2][6] = {{0}};
};

// Evaluation score (middlegame and endgame)
typedef struct score_t {
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
} S;

inline std::ostream &operator<<(std::ostream &out, const score_t &s) {
    out << "S(" << s.mg << ", " << s.eg << ")";
    return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Evaluation parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** EVAL_BEGIN **/

S PST[6][64] =
        {
                {// PAWN
                        S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0},
                        S{-8, 20}, S{0, 25}, S{0, 30}, S{0, 35}, S{0, 35}, S{0, 30}, S{0, 25}, S{-8, 20},
                        S{-8, 9}, S{0, 15}, S{0, 20}, S{0, 26}, S{0, 26}, S{0, 20}, S{0, 15}, S{-8, 9},
                        S{-8, 5}, S{0, 10}, S{6, 15}, S{10, 19}, S{10, 19}, S{6, 15}, S{0, 10}, S{-8, 5},
                        S{-8, 1}, S{0, 5}, S{14, 9}, S{18, 15}, S{18, 15}, S{14, 9}, S{0, 5}, S{-8, 1},
                        S{-8, -1}, S{0, -1}, S{7, -1}, S{7, -1}, S{7, -1}, S{7, -1}, S{0, -1}, S{-8, -1},
                        S{0, -6}, S{0, -6}, S{-5, -6}, S{-9, -6}, S{-9, -6}, S{0, -6}, S{0, -6}, S{0, -6}, // Asymmetric
                        S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0}, S{0, 0},
                },
                {// KNIGHT
                        S{-80, -39}, S{-26, -33}, S{-16, -20}, S{-11, -6}, S{-11, -6}, S{-16, -20}, S{-26, -33}, S{-80, -39},
                        S{-25, -27}, S{-7, -21}, S{2, -8}, S{6, 5}, S{6, 5}, S{2, -8}, S{-7, -21}, S{-25, -27},
                        S{-4, -21}, S{14, -15}, S{23, -2}, S{28, 11}, S{28, 11}, S{23, -2}, S{14, -15}, S{-4, -21},
                        S{-12, -16}, S{6, -10}, S{15, 2}, S{20, 16}, S{20, 16}, S{15, 2}, S{6, -10}, S{-12, -16},
                        S{-11, -16}, S{6, -10}, S{16, 2}, S{21, 16}, S{21, 16}, S{16, 2}, S{6, -10}, S{-11, -16},
                        S{-33, -21}, S{-1, -15}, S{0, -2}, S{4, 11}, S{4, 11}, S{0, -2}, S{-1, -15}, S{-27, -33}, // Asymmetric
                        S{-35, -27}, S{-17, -21}, S{-10, -8}, S{-8, 5}, S{-8, 5}, S{-10, -8}, S{-17, -21}, S{-35, -27},
                        S{-57, -39}, S{-27, -33}, S{-34, -20}, S{-29, -6}, S{-29, -6}, S{-34, -20}, S{-27, -33}, S{-57, -39} // Asymmetric
                },
                {// BISHOP
                        S{-19, -26}, S{-8, -16}, S{-11, -17}, S{-15, -10}, S{-15, -10}, S{-11, -17}, S{-8, -16}, S{-19, -26},
                        S{-12, -17}, S{2, -8}, S{0, -8}, S{-4, -1}, S{-4, -1}, S{0, -8}, S{2, -8}, S{-12, -17},
                        S{-11, -13}, S{0, -4}, S{0, -4}, S{-2, 2}, S{-2, 2}, S{0, -4}, S{0, -4}, S{-11, -13},
                        S{-8, -14}, S{6, -4}, S{3, -5}, S{0, 1}, S{0, 1}, S{3, -5}, S{6, -4}, S{-8, -14},
                        S{0, -14}, S{7, -4}, S{4, -5}, S{0, 1}, S{0, 1}, S{4, -5}, S{7, -4}, S{0, -14},
                        S{-8, -13}, S{8, -4}, S{4, -4}, S{0, 2}, S{0, 2}, S{4, -4}, S{8, -4}, S{-8, -13},
                        S{10, -17}, S{3, -8}, S{0, -8}, S{-3, -1}, S{-3, -1}, S{0, -8}, S{3, -8}, S{10, -17},
                        S{-21, -26}, S{-10, -16}, S{-13, -17}, S{-17, -10}, S{-17, -10}, S{-13, -17}, S{-10, -16}, S{-21, -26}
                },
                {// ROOK
                        S{-8, 0}, S{-6, 0}, S{-4, 0}, S{-3, 0}, S{-3, 0}, S{-4, 0}, S{-6, 0}, S{-8, 0},
                        S{16, 0}, S{18, 0}, S{19, 0}, S{19, 0}, S{19, 0}, S{19, 0}, S{18, 0}, S{16, 0},
                        S{-8, 0}, S{-2, 0}, S{0, 0}, S{3, 0}, S{3, 0}, S{0, 0}, S{-2, 0}, S{-8, 0},
                        S{-8, 0}, S{-2, 0}, S{0, 0}, S{3, 0}, S{3, 0}, S{0, 0}, S{-2, 0}, S{-8, 0},
                        S{-8, 0}, S{-2, 0}, S{0, 0}, S{3, 0}, S{3, 0}, S{0, 0}, S{-2, 0}, S{-8, 0},
                        S{-8, 0}, S{-1, 0}, S{1, 0}, S{3, 0}, S{3, 0}, S{1, 0}, S{-1, 0}, S{-8, 0},
                        S{-8, 0}, S{-2, 0}, S{0, 0}, S{3, 0}, S{3, 0}, S{0, 0}, S{-2, 0}, S{-8, 0},
                        S{-8, 0}, S{-6, 0}, S{-4, 0}, S{5, 0}, S{5, 0}, S{-4, 0}, S{-6, 0}, S{-8, 0}
                },
                {// QUEEN
                        S{-1, -32}, S{-1, -21}, S{-1, -16}, S{-1, -12}, S{-1, -12}, S{-1, -16}, S{-1, -21}, S{-1, -32},
                        S{-1, -21}, S{3, -12}, S{3, -7}, S{3, -2}, S{3, -2}, S{3, -7}, S{3, -12}, S{-1, -21},
                        S{-1, -16}, S{3, -7}, S{3, -2}, S{3, 2}, S{3, 2}, S{3, -2}, S{3, -7}, S{-1, -16},
                        S{-1, -12}, S{3, -2}, S{3, 2}, S{3, 7}, S{3, 7}, S{3, 2}, S{3, -2}, S{-1, -12},
                        S{-1, -12}, S{3, -2}, S{3, 2}, S{3, 7}, S{3, 7}, S{3, 2}, S{3, -2}, S{-1, -12},
                        S{-1, -16}, S{3, -7}, S{3, -2}, S{3, 2}, S{3, 2}, S{3, -2}, S{3, -7}, S{-1, -16},
                        S{-1, -21}, S{4, -12}, S{4, -7}, S{4, -2}, S{4, -2}, S{4, -7}, S{4, -12}, S{-1, -21},
                        S{-1, -32}, S{4, -21}, S{4, -16}, S{4, -12}, S{4, -12}, S{4, -16}, S{4, -21}, S{-1, -32}
                },
                {// KING
                        S{39, 10}, S{52, 32}, S{29, 43}, S{10, 46}, S{10, 46}, S{29, 43}, S{52, 32}, S{39, 10},
                        S{47, 29}, S{61, 51}, S{37, 62}, S{18, 65}, S{18, 65}, S{37, 62}, S{61, 51}, S{47, 29},
                        S{58, 44}, S{72, 66}, S{48, 76}, S{29, 80}, S{29, 80}, S{48, 76}, S{72, 66}, S{58, 44},
                        S{69, 54}, S{82, 75}, S{59, 86}, S{40, 89}, S{40, 89}, S{59, 86}, S{82, 75}, S{69, 54},
                        S{78, 54}, S{92, 75}, S{68, 86}, S{49, 89}, S{49, 89}, S{68, 86}, S{92, 75}, S{78, 54},
                        S{89, 44}, S{103, 66}, S{79, 76}, S{60, 80}, S{60, 80}, S{79, 76}, S{103, 66}, S{89, 44},
                        S{114, 29}, S{128, 51}, S{104, 62}, S{85, 65}, S{85, 65}, S{104, 62}, S{128, 51}, S{114, 29},
                        S{119, 10}, S{140, 32}, S{139, 43}, S{60, 46}, S{90, 46}, S{60, 43}, S{140, 32}, S{119, 10}
                },
        };

constexpr S MATERIAL[6] = {S{85, 100}, S{300, 300}, S{305, 300}, S{490, 500},
                           S{925, 975}}; // Piece type

/// pawns
constexpr S PASSED[8] = {S{0, 0}, S{8, 15}, S{10, 25}, S{14, 35}, S{25, 60},
                         S{50, 90}, S{80, 111}, S{0, 0}}; // Rank of passed pawn
constexpr S ISOLATED[2] = {S{-5, -12}, S{-8, -12}}; // On open file?
/* no pawns in front, and less than 2 enemy pawns in passed bitmap */
constexpr S CANDIDATE[8] = {S{0, 0}, S{4, 10}, S{5, 12}, S{7, 15}, S{10, 25},
                            S{20, 40}, S{0, 0}, S{0, 0}}; // Rank of candidate passed pawn
constexpr S DOUBLED[2] = {S{-10, -20}, S{-7, -11}}; // On open file?
constexpr S KING_SHIELD[2] = {S{18, 9}, S{4, 0}}; // [near/far]

/// other positional
constexpr S BISHOP_PAIR = S{15, 25};

constexpr S ROOK_SEMI_OPEN_FILE = S{21, 12};
constexpr S ROOK_OPEN_FILE = S{29, 12};

/// mobility
constexpr S MOBILITY[6] = {S{0, 10}, S{2, 5}, S{2, 3}, S{1, 10}, S{0, 1},
                           S{0, 0}}; // Piece type
constexpr S BLOCKED_PAWN = S{-12, -15};

/// king safety
constexpr int KING_ATTACKER_WEIGHT[6] = {1, 2, 2, 3, 5};
constexpr int KING_DEFENDER_WEIGHT[6] = {1, 2, 3, 4, 6};
const int KING_MIN_WEIGHT = -1000;

/* linear king safety function */
const score_t KING_DANGER_M = score_t{3, 2};
const score_t KING_DANGER_C = score_t{-31, -120};

/// tempo
constexpr int TEMPO = 5;

/** EVAL_END **/

// Reading eval tables
template <typename T> T read(T *table, int sq, Team side) {
    if(side) {
        return table[sq];
    } else {
        return table[MIRROR_TABLE[sq]];
    }
}

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
    // Check endgame
    eg_eval_t eg_eval = eval_eg(board);
    if(eg_eval.valid) {
        return board.record[board.now].next_move ? -eg_eval.eval : +eg_eval.eval;
    }

    score_t eval = {0, 0};
    eval_data_t dat = {0};

    {
        dat.king_loc[WHITE] = bit_scan(board.bb_pieces[WHITE][KING]);
        dat.king_loc[BLACK] = bit_scan(board.bb_pieces[BLACK][KING]);
        dat.king_shield[WHITE] = BB_KING_CIRCLE[dat.king_loc[WHITE]];
        dat.king_shield[BLACK] = BB_KING_CIRCLE[dat.king_loc[BLACK]];
    }

    eval += eval_pawns(WHITE, board, dat) - eval_pawns(BLACK, board, dat);
    eval += eval_knights(WHITE, board, dat) - eval_knights(BLACK, board, dat);
    eval += eval_bishops(WHITE, board, dat) - eval_bishops(BLACK, board, dat);
    eval += eval_rooks(WHITE, board, dat) - eval_rooks(BLACK, board, dat);
    eval += eval_queens(WHITE, board, dat) - eval_queens(BLACK, board, dat);
    eval += eval_kings(WHITE, board, dat) - eval_kings(BLACK, board, dat);

    // Aim to exchange when up material, or avoid exchanges when down material
    {
        int exchange_score = 0;
        int white_mat = 1 * dat.counts[WHITE][PAWN] +
                        3 * dat.counts[WHITE][KNIGHT] +
                        3 * dat.counts[WHITE][BISHOP] +
                        5 * dat.counts[WHITE][ROOK] +
                        10 * dat.counts[WHITE][QUEEN];
        int black_mat = 1 * dat.counts[BLACK][PAWN] +
                        3 * dat.counts[BLACK][KNIGHT] +
                        3 * dat.counts[BLACK][BISHOP] +
                        5 * dat.counts[BLACK][ROOK] +
                        10 * dat.counts[BLACK][QUEEN];
        if (white_mat > black_mat) {
            exchange_score = +40 + (3 * white_mat - 5 * black_mat);
        } else if (black_mat > white_mat) {
            exchange_score = -40 - (3 * black_mat - 5 * white_mat);
        }

        eval += S{exchange_score, exchange_score * 2};
    }

    int tapered_phase;
    {
        int val[5] = {0, 1, 1, 2, 4};
        int total = val[PAWN] * 16 + val[KNIGHT] * 4 + val[BISHOP] * 4 + val[ROOK] * 4 + val[QUEEN] * 2;

        tapered_phase = total;

        for (int piece = 0; piece < 5; piece++) {
            tapered_phase -= val[piece] * (dat.counts[0][piece] + dat.counts[1][piece]);
        }

        tapered_phase = (tapered_phase * 256 + (total / 2)) / total;
    }

    int final_score = ((eval.mg * (256 - tapered_phase)) + (eval.eg * tapered_phase)) / 256;

    return board.record[board.now].next_move ? -final_score - TEMPO : +final_score + TEMPO;
}

score_t eval_pawns(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};
    auto xside = Team(!side);

    U64 pawns = board.bb_pieces[side][PAWN];
    while (pawns) {
        uint8_t sq = pop_bit(side, pawns);

        dat.counts[side][PAWN]++;

        // Material & PST
        score += MATERIAL[PAWN] + read(PST[PAWN], sq, side);

        U64 moves = find_moves<PAWN>(side, sq, 0xffffffffffffffff);
        dat.attacked_by_pawn[side] |= moves;

        // Update holes bitmap
        dat.attackable_by_pawn[side] |= BB_HOLE[side][sq];

        // Blocked?
        if(find_moves<PAWN>(side, sq, 0) & board.bb_side[side]) {
            score += BLOCKED_PAWN;
        }

        // King safety
        dat.king_danger_balance[side] -= pop_count(single_bit(sq) & dat.king_shield[side]) * KING_DEFENDER_WEIGHT[PAWN];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[PAWN];

        // King shield
        if(single_bit(sq) & dat.king_shield[side]) {
            if(distance(sq, dat.king_loc[side]) <= 1) {
                score += KING_SHIELD[0];
            } else {
                score += KING_SHIELD[1];
            }
        }

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
        score += MATERIAL[KNIGHT] + read(PST[KNIGHT], sq, side);

        U64 moves = find_moves<KNIGHT>(side, sq, board.bb_all) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[KNIGHT] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[KNIGHT];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[KNIGHT];
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
        score += MATERIAL[BISHOP] + read(PST[BISHOP], sq, side);

        U64 moves = find_moves<BISHOP>(side, sq, board.bb_side[side]) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[BISHOP] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[BISHOP];
        dat.king_danger_balance[xside] += ((moves & dat.king_shield[xside]) != 0) * KING_ATTACKER_WEIGHT[BISHOP];
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

        score += MATERIAL[ROOK] + read(PST[ROOK], sq, side);

        U64 moves = find_moves<ROOK>(side, sq, board.bb_side[side]) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[ROOK] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[ROOK];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[ROOK];

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

        score += MATERIAL[QUEEN] + read(PST[QUEEN], sq, side);

        U64 moves = find_moves<QUEEN>(side, sq, board.bb_all) & ~board.bb_side[side] &
                    (~dat.attacked_by_pawn[xside] | board.bb_side[xside]); // Find non-losing moves

        // Mobility
        score += MOBILITY[QUEEN] * pop_count(moves);

        // King safety
        dat.king_danger_balance[side] -= ((moves & dat.king_shield[side]) != 0) * KING_DEFENDER_WEIGHT[QUEEN];
        dat.king_danger_balance[xside] += pop_count(moves & dat.king_shield[xside]) * KING_ATTACKER_WEIGHT[QUEEN];
    }

    return score;
}

score_t eval_kings(Team side, const board_t &board, eval_data_t &dat) {
    score_t score = {0, 0};

    int balance = std::max(dat.king_danger_balance[side], KING_MIN_WEIGHT);
    score -= KING_DANGER_M * balance + KING_DANGER_C;

    score.eg = std::max(score.eg, 0);
    score.mg = std::max(score.mg, 0);

    score += read(PST[KING], dat.king_loc[side], side);
    return score;
}
