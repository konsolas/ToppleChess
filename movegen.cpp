//
// Created by Vincent on 23/09/2017.
//

#include "movegen.h"

movegen_t::movegen_t(board_t &board) : board(board) {
    record = board.record[board.now];
    team = record.next_move;
    x_team = Team(!record.next_move);
}

int movegen_t::gen_normal(move_t *buf) {
    int caps = gen_caps(buf);
    int quiets = gen_quiets(buf + caps);

    return caps + quiets;
}

int movegen_t::gen_caps(move_t *buf) {
    // En-passant capture
    int buf_size = gen_ep(buf);

    move_t move = EMPTY_MOVE;
    move.info.team = team;
    move.info.is_capture = 1;

    // Pawn caps
    move.info.piece = PAWN;
    U64 bb_pawns = board.bb_pieces[team][PAWN];

    while (bb_pawns) {
        uint8_t from = pop_bit(bb_pawns);
        move.info.from = from;

        U64 bb_targets = pawn_caps(team, from) & board.bb_side[x_team];

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;
            move.info.captured_type = board.sq_data[to].piece;

            // Promotions
            if (team ? to <= H1 : to >= A8) {
                move.info.is_promotion = 1;
                for (uint8_t i = QUEEN; i > PAWN; i--) {
                    move.info.promotion_type = i;

                    buf[buf_size++] = move;
                }

                move.info.is_promotion = 0;
                move.info.promotion_type = 0;
            } else {
                buf[buf_size++] = move;
            }
        }
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
    move_t move{};

    // Generate castling kingside
    if (record.castle[team][0]) {
        if ((board.bb_all & bits_between(team ? E8 : E1, team ? H8 : H1)) == 0 &&
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
    if (record.castle[team][1]) {
        if ((board.bb_all & bits_between(team ? E8 : E1, team ? A8 : A1)) == 0 &&
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

int movegen_t::gen_ep(move_t *buf) {
    int buf_size = 0;
    move_t move = EMPTY_MOVE;
    move.info.team = team;

    // Generate en-passant capture
    if (record.ep_square != 0) {
        U64 ep_attacks = pawn_caps(x_team, record.ep_square) & board.bb_pieces[team][PAWN];

        while (ep_attacks) {
            uint8_t from = pop_bit(ep_attacks);

            move.info.from = from;
            move.info.piece = PAWN;
            move.info.to = record.ep_square;
            move.info.is_ep = 1;
            buf[buf_size++] = move;
        }
    }

    return buf_size;
}

int movegen_t::gen_quiets(move_t *buf) {
    int buf_size = gen_castling(buf);

    // Generates quiet moves only
    U64 mask = ~board.bb_all;

    // Pawn moves
    move_t move{};
    move = EMPTY_MOVE;
    move.info.team = team;
    move.info.piece = PAWN;
    U64 bb_pawns = board.bb_pieces[team][PAWN];

    while (bb_pawns) {
        uint8_t from = pop_bit(bb_pawns);
        move.info.from = from;

        U64 bb_targets = find_moves<PAWN>(team, from, board.bb_all) & mask;

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;

            // Promotions
            if (team ? to <= H1 : to >= A8) {
                move.info.is_promotion = 1;
                for (uint8_t i = QUEEN; i > PAWN; i--) {
                    move.info.promotion_type = i;

                    buf[buf_size++] = move;
                }

                move.info.is_promotion = 0;
                move.info.promotion_type = 0;
            } else {
                buf[buf_size++] = move;
            }
        }
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
    U64 bb_piece = board.bb_pieces[team][TYPE];

    while (bb_piece) {
        uint8_t from = pop_bit(bb_piece);
        move.info.from = from;

        U64 bb_targets = find_moves<TYPE>(team, from, board.bb_all) & mask;

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
    U64 bb_piece = board.bb_pieces[team][TYPE];

    while (bb_piece) {
        uint8_t from = pop_bit(bb_piece);
        move.info.from = from;

        U64 bb_targets = find_moves<TYPE>(team, from, board.bb_all) & board.bb_side[x_team];

        while (bb_targets) {
            uint8_t to = pop_bit(bb_targets);
            move.info.to = to;
            move.info.captured_type = board.sq_data[to].piece;

            buf[buf_size++] = move;
        }
    }
}