//
// Created by Vincent on 23/09/2017.
//

#include "movegen.h"

movegen_t::movegen_t(board_t &board) : board(board) {
    idx = 0;
}

int movegen_t::gen_normal() {
    record = board.record[board.now];
    team = record.next_move;
    x_team = Team(!record.next_move);

    gen_caps();
    gen_quiets();

    return idx;
}

int movegen_t::gen_caps() {
    record = board.record[board.now];
    team = record.next_move;
    x_team = Team(!record.next_move);

    // En-passant capture
    gen_ep();

    move_t move = EMPTY_MOVE;
    move.team = team;
    move.is_capture = 1;

    // Loop through opponent pieces instead of own pieces to find captures
    // MVV
    for (uint8_t victim = QUEEN;
         victim <= QUEEN; victim--) { // victim <= QUEEN because uint8_t will go to 255 if you subtract 1 from 0
        move.captured_type = victim;
        U64 bb_victim = board.bb_pieces[x_team][victim];

        // Loop through LVA
        for (uint8_t attacker = PAWN; attacker <= KING; attacker++) {
            U64 bb_attacker = board.bb_pieces[team][attacker];
            move.piece = attacker;

            while (bb_attacker) {
                uint8_t from = popBit(team, bb_attacker);
                move.from = from;

                U64 bb_targets = find_moves(Piece(attacker), team, from, board.bb_all) & bb_victim; // Mask captures
                while (bb_targets) {
                    uint8_t to = popBit(team, bb_targets); // Prefer captures closer to our home rank
                    move.to = to;

                    // Promotions
                    if (attacker == PAWN && (team ? to <= H1 : to >= A8)) {
                        move.is_promotion = 1;
                        for (uint8_t i = QUEEN; i > PAWN; i--) {
                            move.promotion_type = i;

                            buf[idx++] = move;
                        }
                        move.is_promotion = 0;
                        move.promotion_type = 0;
                    } else {
                        buf[idx++] = move;
                    }

                }
            }
        }
    }

    return idx;
}

void movegen_t::gen_castling() {
    move_t move{};

    // Generate castling kingside
    if (record.castle[team][0]) {
        if ((board.bb_all & bits_between(team ? E8 : E1, team ? H8 : H1)) == 0 &&
            !board.is_attacked(team ? E8 : E1, x_team) &&
            !board.is_attacked(team ? F8 : F1, x_team) &&
            !board.is_attacked(team ? G8 : G1, x_team)) {
            // No pieces between, we can castle!
            move = EMPTY_MOVE;
            move.piece = KING;
            move.team = team;
            move.from = team ? E8 : E1;
            move.to = team ? G8 : G1;
            move.castle = 1;
            move.castle_side = 0;
            buf[idx++] = move;
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
            move.piece = KING;
            move.team = team;
            move.from = team ? E8 : E1;
            move.to = team ? C8 : C1;
            move.castle = 1;
            move.castle_side = 1;
            buf[idx++] = move;
        }
    }
}

void movegen_t::gen_ep() {
    move_t move = EMPTY_MOVE;
    move.team = team;

    // Generate en-passant capture
    if (record.ep_square != 0) {
        U64 ep_attacks = find_moves(PAWN, x_team, record.ep_square, board.bb_pieces[team][PAWN])
                         & board.bb_pieces[team][PAWN];

        while (ep_attacks) {
            uint8_t from = popBit(WHITE, ep_attacks);

            move.from = from;
            move.piece = PAWN;
            move.to = record.ep_square;
            move.is_ep = 1;
            buf[idx++] = move;
        }
    }
}

// Don't use :(
int movegen_t::gen_evasions() {
    record = board.record[board.now];
    team = record.next_move;
    x_team = Team(!record.next_move);

    move_t move{};

    U64 king_bb = board.bb_pieces[team][KING];
    uint8_t king_sq = popBit(WHITE, king_bb);

    U64 checkers = board.attacks_to(king_sq, x_team);

    // Just generate the en-passant capture in case it's relevant
    gen_ep();

    // If there's only one checker, we can capture it or block the check
    // No point in checking popcount, it's quite performance intensive so we skip it and assume makemove isn't that bad.
    U64 checkers_bb = checkers;
    uint8_t checker_sq = popBit(WHITE, checkers);

    // Try to capture checker (loop through LVA)
    move = EMPTY_MOVE;
    move.team = team;
    move.is_capture = 1;
    move.captured_type = board.sq_data[checker_sq].piece;
    for (uint8_t attacker = PAWN; attacker < KING; attacker++) {
        U64 bb_attacker = board.bb_pieces[team][attacker];
        move.piece = attacker;

        while (bb_attacker) {
            uint8_t from = popBit(team, bb_attacker);
            move.from = from;

            U64 bb_targets = find_moves(Piece(attacker), team, from, board.bb_all) & checkers_bb; // Mask captures

            if (bb_targets) {
                move.to = checker_sq;

                // Promotions
                if (attacker == PAWN && (team ? move.to <= H1 : move.to >= A8)) {
                    move.is_promotion = 1;
                    for (uint8_t i = QUEEN; i > PAWN; i--) {
                        move.promotion_type = i;

                        buf[idx++] = move;
                    }
                    move.is_promotion = 0;
                    move.promotion_type = 0;
                } else {
                    buf[idx++] = move;
                }
            }
        }
    }

    // If the checker is a slider, try to interpose
    Piece checker_type = board.sq_data[checker_sq].piece;
    if (checker_type == ROOK || checker_type == BISHOP || checker_type == QUEEN) {
        U64 interpose_bb = bits_between(king_sq, checker_sq);

        move = EMPTY_MOVE;
        move.team = team;
        move.piece = PAWN;
        U64 bb_pawns = board.bb_pieces[team][PAWN];
        while (bb_pawns) {
            uint8_t from = popBit(team, bb_pawns);
            move.from = from;

            U64 bb_targets = find_moves(PAWN, team, from, board.bb_all) & interpose_bb;

            while (bb_targets) {
                uint8_t to = popBit(x_team, bb_targets);
                move.to = to;

                // Promotions
                if (team ? to <= H1 : to >= A8) {
                    move.is_promotion = 1;
                    for (uint8_t i = QUEEN; i > PAWN; i--) {
                        move.promotion_type = i;

                        buf[idx++] = move;
                    }

                    move.is_promotion = 0;
                    move.promotion_type = 0;
                } else {
                    buf[idx++] = move;
                }
            }
        }

        // Generate piece moves (not pawns), least valuable piece first (can't interpose with king)
        for (uint8_t piece = KNIGHT; piece < KING; piece++) {
            move.piece = piece;
            U64 bb_piece = board.bb_pieces[team][piece];

            while (bb_piece) {
                uint8_t from = popBit(team, bb_piece);
                move.from = from;

                U64 bb_targets = find_moves((Piece) piece, team, from, board.bb_all) & interpose_bb;

                while (bb_targets) {
                    uint8_t to = popBit(x_team, bb_targets);
                    move.to = to;

                    buf[idx++] = move;
                }
            }
        }
    }

    // The only other option is moving the king
    move = EMPTY_MOVE;
    move.team = team;
    move.from = king_sq;
    move.piece = KING;
    U64 king_targets_bb = find_moves(KING, team, king_sq, board.bb_all) & ~board.bb_side[team];
    while (king_targets_bb) {
        uint8_t to = popBit(team, king_targets_bb);
        move.to = to;

        //if (!board.is_attacked(to, x_team)) {
            if (board.sq_data[to].occupied) {
                move.is_capture = 1;
                move.captured_type = board.sq_data[to].piece;

                buf[idx++] = move;

                move.is_capture = 0;
                move.captured_type = 0;
            } else {
                buf[idx++] = move;
            }
        //}
    }

    return idx;
}

int movegen_t::gen_quiets() {
    // Generates quiet moves only
    U64 mask = ~board.bb_all;

    gen_castling();

    // Pawn moves
    move_t move{};
    move = EMPTY_MOVE;
    move.team = team;
    move.piece = PAWN;
    U64 bb_pawns = board.bb_pieces[team][PAWN];

    while (bb_pawns) {
        uint8_t from = popBit(team, bb_pawns);
        move.from = from;

        U64 bb_targets = find_moves(PAWN, team, from, board.bb_all) & mask;

        while (bb_targets) {
            uint8_t to = popBit(x_team, bb_targets);
            move.to = to;

            // Promotions
            if (team ? to <= H1 : to >= A8) {
                move.is_promotion = 1;
                for (uint8_t i = QUEEN; i > PAWN; i--) {
                    move.promotion_type = i;

                    buf[idx++] = move;
                }

                move.is_promotion = 0;
                move.promotion_type = 0;
            } else {
                buf[idx++] = move;
            }
        }
    }

    // Generate piece moves (not pawns)
    for (uint8_t piece = 1; piece < 6; piece++) {
        move.piece = piece;
        U64 bb_piece = board.bb_pieces[team][piece];

        while (bb_piece) {
            uint8_t from = popBit(team, bb_piece);
            move.from = from;

            U64 bb_targets = find_moves((Piece) piece, team, from, board.bb_all) & mask;

            while (bb_targets) {
                uint8_t to = popBit(x_team, bb_targets);
                move.to = to;

                buf[idx++] = move;
            }
        }
    }

    return idx;
}

void movegen_t::add_move(move_t move) {
    buf[idx++] = move;
}
