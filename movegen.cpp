//
// Created by Vincent on 23/09/2017.
//

#include "movegen.h"
#include "move.h"

constexpr U64 PROMOTING[2] = {
        0x00FF000000000000,
        0x000000000000FF00
};

constexpr U64 STARTING[2] = {
        0x000000000000FF00,
        0x00FF000000000000
};

movegen_t::movegen_t(const board_t &board) : board(board) {
    team = board.now().next_move;
    x_team = Team(!board.now().next_move);
}

int movegen_t::gen_normal(move_t *buf) {
    int noisy = gen_noisy(buf);
    int quiets = gen_quiets(buf + noisy);

    return noisy + quiets;
}

int movegen_t::gen_noisy(move_t *buf) {
    // En-passant capture
    int buf_size = gen_ep(buf);

    // Promotions
    buf_size += gen_prom(buf + buf_size);

    move_t move = EMPTY_MOVE;
    move.info.team = team;
    move.info.is_capture = 1;

    // Pawn caps
    move.info.piece = PAWN;
    U64 bb_pawns = board.pieces(team, PAWN) & ~PROMOTING[team];

    U64 capt_left = (team ? bb_shifts::shift<D_SE>(bb_pawns) : bb_shifts::shift<D_NW>(bb_pawns)) & board.side(x_team);
    U64 capt_right = (team ? bb_shifts::shift<D_SW>(bb_pawns) : bb_shifts::shift<D_NE>(bb_pawns)) & board.side(x_team);

    while (capt_left) {
        uint8_t to = pop_bit(capt_left);
        move.info.to = to;
        move.info.from = to - rel_offset(team, D_NW);
        move.info.captured_type = board.sq(to).piece();

        buf[buf_size++] = move;
    }

    while (capt_right) {
        uint8_t to = pop_bit(capt_right);
        move.info.to = to;
        move.info.from = to - rel_offset(team, D_NE);
        move.info.captured_type = board.sq(to).piece();

        buf[buf_size++] = move;
    }

    // Generate piece caps (not pawns)
    gen_piece_caps<KNIGHT>(buf, buf_size, move);
    gen_piece_caps<BISHOP>(buf, buf_size, move);
    gen_piece_caps<ROOK>(buf, buf_size, move);
    gen_piece_caps<QUEEN>(buf, buf_size, move);
    gen_piece_caps<KING>(buf, buf_size, move);

    return buf_size;
}

int movegen_t::gen_castling(move_t *buf) {
    int buf_size = 0;
    move_t move = EMPTY_MOVE;

    // Generate castling kingside
    if (board.now().castle[team][0]) {
        if ((board.all() & bits_between(team ? E8 : E1, team ? H8 : H1)) == 0 &&
            !board.is_attacked(team ? E8 : E1, x_team) &&
            !board.is_attacked(team ? F8 : F1, x_team) &&
            !board.is_attacked(team ? G8 : G1, x_team)) {
            // No pieces between, we can castle!
            move = EMPTY_MOVE;
            move.info.piece = KING;
            move.info.team = team;
            move.info.from = team ? E8 : E1;
            move.info.to = team ? G8 : G1;
            move.info.castle = 1;
            move.info.castle_side = 0;
            buf[buf_size++] = move;
        }
    }

    // Generate castling queenside
    if (board.now().castle[team][1]) {
        if ((board.all() & bits_between(team ? E8 : E1, team ? A8 : A1)) == 0 &&
            !board.is_attacked(team ? E8 : E1, x_team) &&
            !board.is_attacked(team ? D8 : D1, x_team) &&
            !board.is_attacked(team ? C8 : C1, x_team)) {
            // No pieces between, we can castle!
            move = EMPTY_MOVE;
            move.info.piece = KING;
            move.info.team = team;
            move.info.from = team ? E8 : E1;
            move.info.to = team ? C8 : C1;
            move.info.castle = 1;
            move.info.castle_side = 1;
            buf[buf_size++] = move;
        }
    }

    return buf_size;
}

int movegen_t::gen_prom(move_t *buf) {
    int buf_size = 0;
    move_t move = EMPTY_MOVE;
    move.info.team = team;
    move.info.piece = PAWN;
    move.info.is_promotion = 1;

    // Capturing promotions
    move.info.is_capture = 1;
    U64 bb_promotable = board.pieces(team, PAWN) & PROMOTING[team];
    while (bb_promotable) {
        uint8_t from = pop_bit(bb_promotable);
        move.info.from = from;

        U64 bb_targets = pawn_caps(team, from) & board.side(x_team);

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;
            move.info.captured_type = board.sq(to).piece();

            // Promotions
            for (uint8_t i = QUEEN; i > PAWN; i--) {
                move.info.promotion_type = i;

                buf[buf_size++] = move;
            }
        }
    }

    // Non-capturing promotions
    U64 mask = ~board.all();
    move.info.is_capture = 0;
    move.info.captured_type = 0;
    bb_promotable = board.pieces(team, PAWN) & PROMOTING[team];
    while (bb_promotable) {
        uint8_t from = pop_bit(bb_promotable);
        move.info.from = from;

        U64 bb_targets = find_moves<PAWN>(team, from, board.all()) & mask;

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;

            // Promotions
            for (uint8_t i = QUEEN; i > PAWN; i--) {
                move.info.promotion_type = i;

                buf[buf_size++] = move;
            }
        }
    }

    return buf_size;
}

int movegen_t::gen_ep(move_t *buf) {
    int buf_size = 0;
    move_t move = EMPTY_MOVE;
    move.info.team = team;

    // Generate en-passant capture
    if (board.now().ep_square != 0) {
        U64 ep_attacks = pawn_caps(x_team, board.now().ep_square) & board.pieces(team, PAWN);

        while (ep_attacks) {
            uint8_t from = pop_bit(ep_attacks);

            move.info.from = from;
            move.info.piece = PAWN;
            move.info.to = board.now().ep_square;
            move.info.is_ep = 1;
            move.info.is_capture = 1;
            move.info.captured_type = PAWN;
            buf[buf_size++] = move;
        }
    }

    return buf_size;
}

int movegen_t::gen_quiets(move_t *buf) {
    int buf_size = gen_castling(buf);

    // Generates quiet moves only
    U64 mask = ~board.all();

    // Pawn moves
    move_t move = EMPTY_MOVE;
    move.info.team = team;
    move.info.piece = PAWN;
    U64 bb_pawns = board.pieces(team, PAWN) & ~PROMOTING[team];

    U64 blocked_x1, blocked_x2;
    if (team) {
        blocked_x1 = bb_shifts::shift<D_N>(board.all());
        blocked_x2 = bb_shifts::shift<D_N>(blocked_x1) | blocked_x1;
    } else {
        blocked_x1 = bb_shifts::shift<D_S>(board.all());
        blocked_x2 = bb_shifts::shift<D_S>(blocked_x1) | blocked_x1;
    }

    U64 single_movers = bb_pawns & ~blocked_x1;
    U64 double_movers = bb_pawns & STARTING[team] & ~blocked_x2;
    int single_offset = rel_offset(team, D_N);
    int double_offset = single_offset * 2;

    while (single_movers) {
        uint8_t from = pop_bit(single_movers);
        move.info.from = from;
        move.info.to = from + single_offset;

        buf[buf_size++] = move;
    }

    while (double_movers) {
        uint8_t from = pop_bit(double_movers);
        move.info.from = from;
        move.info.to = from + double_offset;

        buf[buf_size++] = move;
    }

    // Generate piece moves (not pawns)
    gen_piece_quiets<KNIGHT>(buf, buf_size, move, mask);
    gen_piece_quiets<BISHOP>(buf, buf_size, move, mask);
    gen_piece_quiets<ROOK>(buf, buf_size, move, mask);
    gen_piece_quiets<QUEEN>(buf, buf_size, move, mask);
    gen_piece_quiets<KING>(buf, buf_size, move, mask);

    return buf_size;
}


template<Piece TYPE>
void movegen_t::gen_piece_quiets(move_t *buf, int &buf_size, move_t move, U64 mask) {
    move.info.piece = TYPE;
    U64 bb_piece = board.pieces(team, TYPE);

    while (bb_piece) {
        uint8_t from = pop_bit(bb_piece);
        move.info.from = from;

        U64 bb_targets = find_moves<TYPE>(team, from, board.all()) & mask;

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;

            buf[buf_size++] = move;
        }
    }
}

template<Piece TYPE>
void movegen_t::gen_piece_caps(move_t *buf, int &buf_size, move_t move) {
    move.info.piece = TYPE;
    U64 bb_piece = board.pieces(team, TYPE);

    while (bb_piece) {
        uint8_t from = pop_bit(bb_piece);
        move.info.from = from;

        U64 bb_targets = find_moves<TYPE>(team, from, board.all()) & board.side(x_team);

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;
            move.info.captured_type = board.sq(to).piece();

            buf[buf_size++] = move;
        }
    }
}
