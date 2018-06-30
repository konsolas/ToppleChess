//
// Created by Vincent on 09/06/2018.
//

#include "endgame.h"

constexpr int PREFER = 100;
constexpr int KNOWN_WIN = 1000;
constexpr int VALUE[6] = {100, 280, 300, 500, 925, 0};

constexpr int CORNERS[64]{
        100, 90, 80, 70, 70, 80, 90, 100,
        90, 70, 60, 50, 50, 60, 70, 90,
        80, 60, 40, 30, 30, 40, 60, 80,
        70, 50, 30, 20, 20, 30, 50, 70,
        70, 50, 30, 20, 20, 30, 50, 70,
        80, 60, 40, 30, 30, 40, 60, 80,
        90, 70, 60, 50, 50, 60, 70, 90,
        100, 90, 80, 70, 70, 80, 90, 100
};

constexpr int KBNK[64]{
        100, 90, 80, 70, 60, 50, 40, 30,
        90, 80, 70, 60, 50, 40, 30, 40,
        80, 70, 60, 50, 40, 30, 40, 50,
        70, 60, 50, 40, 30, 40, 50, 60,
        60, 50, 40, 30, 40, 50, 60, 70,
        50, 40, 30, 40, 50, 60, 70, 80,
        40, 30, 40, 50, 60, 70, 80, 90,
        30, 40, 50, 60, 70, 80, 90, 100
};

constexpr int TOGETHER[8]{0, 20, 16, 12, 8, 4, 2, 0};

void eg_init() {
    // Reserved
}

int eval_material(int mat[2][6]) {
    int sum = 0;
    for (int i = 0; i < 6; i++) {
        sum += mat[0][i] * VALUE[i];
        sum -= mat[1][i] * VALUE[i];
    }

    return sum;
}

// Force the opponent king to the edge, move our pieces closer to it, move their pieces away from it
// Don't use this for endgames containing pawns.
int eval_universal(const board_t &board, Team team) {
    int eval = 0;

    auto x_team = Team(!team);
    uint8_t x_team_king = bit_scan(board.bb_pieces[x_team][KING]);

    eval += CORNERS[x_team_king];

    for (uint8_t piece = KNIGHT; piece <= KING; piece++) {
        U64 piece_bb = board.bb_pieces[team][piece];
        while (piece_bb) {
            uint8_t sq = pop_bit(piece_bb);

            eval += TOGETHER[distance(x_team_king, sq)];
        }
    }

    for (uint8_t piece = KNIGHT; piece < KING; piece++) {
        U64 piece_bb = board.bb_pieces[x_team][piece];
        while (piece_bb) {
            uint8_t sq = pop_bit(piece_bb);

            eval -= TOGETHER[distance(x_team_king, sq)];
        }
    }

    return eval;
}

int eval_kbnk(const board_t &board, Team team) {
    int eval = 0;

    auto x_team = Team(!team);
    uint8_t x_team_king = bit_scan(board.bb_pieces[x_team][KING]);

    // Force king to the correct corner
    eval += (same_colour(bit_scan(board.bb_pieces[team][BISHOP]), A1)) ? KBNK[x_team_king]
                                                                       : KBNK[MIRROR_TABLE[x_team_king]];

    return eval;
}

eg_eval_t eval_eg(const board_t &board) {
    // Count material
    int mat[2][6] = {0};
    int total_mat[2] = {0};
    for (uint8_t team = WHITE; team <= BLACK; team++) {
        for (uint8_t piece = PAWN; piece <= KING; piece++) {
            int count = pop_count(board.bb_pieces[team][piece]);
            mat[team][piece] = count;
            total_mat[team] += count;
        }
    }

    // Process material
    if ((total_mat[WHITE] == 1 && total_mat[BLACK] == 1)) {
        // KvK
        return {true, 0};
    } else if (mat[WHITE][PAWN] == 0 && mat[BLACK][PAWN] == 0) {
        // No pawns
        if (mat[WHITE][ROOK] == 0 && mat[WHITE][QUEEN] == 0 && mat[BLACK][ROOK] == 0 && mat[WHITE][ROOK] == 0) {
            // No major pieces
            if ((mat[WHITE][BISHOP] == 0 && mat[BLACK][BISHOP] == 0 && total_mat[WHITE] < 3 && total_mat[BLACK] < 3)
                || (abs(total_mat[WHITE] - total_mat[BLACK]) <= 1)) {
                return {true, 0};
            } else {
                // KBNvK
                if (mat[WHITE][BISHOP] == 1 && mat[WHITE][KNIGHT] == 1 && total_mat[BLACK] == 1) {
                    return {true, eval_material(mat) + eval_kbnk(board, WHITE) + PREFER};
                } else if (mat[BLACK][BISHOP] == 1 && mat[BLACK][KNIGHT] == 1 && total_mat[WHITE] == 1) {
                    return {true, eval_material(mat) - eval_kbnk(board, BLACK) - PREFER};
                }
            }
        }

        // Fall back on universal endgame evaluation
        return {true, eval_material(mat) + eval_universal(board, WHITE) - eval_universal(board, BLACK)};
    } else {
        // TODO: Pawn endgames
    }

    return {false, 0};
}
