//
// Created by Vincent on 23/09/2017.
//

#include "movegen.h"

movegen_t::movegen_t(board_t &board) : board(board) {}

int movegen_t::gen_normal() {
    record = board.record[board.now];
    team = record.next_move;
    x_team = Team(!record.next_move);

    gen_caps();
    gen_quiets();

    return buf_size;
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

    // Old MVV/LVA generation
    // MVV
    /*
    for (uint8_t victim = QUEEN;
         victim <= QUEEN; victim--) { // victim <= QUEEN because uint8_t will go to 255 if you subtract 1 from 0
        move.captured_type = victim;
        U64 bb_victim = board.bb_pieces[x_team][victim];

        // Loop through LVA
        for (uint8_t attacker = PAWN; attacker <= KING; attacker++) {
            U64 bb_attacker = board.bb_pieces[team][attacker];
            move.piece = attacker;

            while (bb_attacker) {
                uint8_t from = pop_bit(team, bb_attacker);
                move.from = from;

                U64 bb_targets = find_moves(Piece(attacker), team, from, board.bb_all) & bb_victim; // Mask captures
                while (bb_targets) {
                    uint8_t to = pop_bit(team, bb_targets); // Prefer captures closer to our home rank
                    move.to = to;

                    // Promotions
                    if (attacker == PAWN && (team ? to <= H1 : to >= A8)) {
                        move.is_promotion = 1;
                        for (uint8_t i = QUEEN; i > PAWN; i--) {
                            move.promotion_type = i;

                            buf[buf_size++] = move;
                        }
                        move.is_promotion = 0;
                        move.promotion_type = 0;
                    } else {
                        buf[buf_size++] = move;
                    }

                }
            }
        }
    }
    */

    // New max speed generation
    // Pawn moves
    move.piece = PAWN;
    U64 bb_pawns = board.bb_pieces[team][PAWN];

    while (bb_pawns) {
        uint8_t from = pop_bit(team, bb_pawns);
        move.from = from;

        U64 bb_targets = find_moves(PAWN, team, from, board.bb_all) & board.bb_side[x_team];

        while (bb_targets) {
            uint8_t to = pop_bit(x_team, bb_targets);
            move.to = to;
            move.captured_type = board.sq_data[to].piece;

            // Promotions
            if (team ? to <= H1 : to >= A8) {
                move.is_promotion = 1;
                for (uint8_t i = QUEEN; i > PAWN; i--) {
                    move.promotion_type = i;

                    buf[buf_size++] = move;
                }

                move.is_promotion = 0;
                move.promotion_type = 0;
            } else {
                buf[buf_size++] = move;
            }
        }
    }

    // Generate piece moves (not pawns)
    for (uint8_t piece = 1; piece < 6; piece++) {
        move.piece = piece;
        U64 bb_piece = board.bb_pieces[team][piece];

        while (bb_piece) {
            uint8_t from = pop_bit(team, bb_piece);
            move.from = from;

            U64 bb_targets = find_moves((Piece) piece, team, from, board.bb_all) & board.bb_side[x_team];

            while (bb_targets) {
                uint8_t to = pop_bit(x_team, bb_targets);
                move.to = to;
                move.captured_type = board.sq_data[to].piece;

                buf[buf_size++] = move;
            }
        }
    }

    return buf_size;
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
            move.piece = KING;
            move.team = team;
            move.from = team ? E8 : E1;
            move.to = team ? C8 : C1;
            move.castle = 1;
            move.castle_side = 1;
            buf[buf_size++] = move;
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
            uint8_t from = pop_bit(WHITE, ep_attacks);

            move.from = from;
            move.piece = PAWN;
            move.to = record.ep_square;
            move.is_ep = 1;
            buf[buf_size++] = move;
        }
    }
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
        uint8_t from = pop_bit(team, bb_pawns);
        move.from = from;

        U64 bb_targets = find_moves(PAWN, team, from, board.bb_all) & mask;

        while (bb_targets) {
            uint8_t to = pop_bit(x_team, bb_targets);
            move.to = to;

            // Promotions
            if (team ? to <= H1 : to >= A8) {
                move.is_promotion = 1;
                for (uint8_t i = QUEEN; i > PAWN; i--) {
                    move.promotion_type = i;

                    buf[buf_size++] = move;
                }

                move.is_promotion = 0;
                move.promotion_type = 0;
            } else {
                buf[buf_size++] = move;
            }
        }
    }

    // Generate piece moves (not pawns)
    for (uint8_t piece = 1; piece < 6; piece++) {
        move.piece = piece;
        U64 bb_piece = board.bb_pieces[team][piece];

        while (bb_piece) {
            uint8_t from = pop_bit(team, bb_piece);
            move.from = from;

            U64 bb_targets = find_moves((Piece) piece, team, from, board.bb_all) & mask;

            while (bb_targets) {
                uint8_t to = pop_bit(x_team, bb_targets);
                move.to = to;

                buf[buf_size++] = move;
            }
        }
    }

    return buf_size;
}

move_t movegen_t::next(GenStage &stage, search_t &search, move_t hash_move, int ply) {
    const int CAPT_BASE = 30000;

    if(stage < GEN_HASH) {
        if(hash_move != EMPTY_MOVE) {
            for (int i = idx; i < buf_size; i++) {
                if(hash_move == buf[i]) {
                    buf_swap(i, idx);
                    stage = GEN_HASH;
                    return next();
                }
            }
        }
    }

    // Score moves
    if(!scored) {
        for (int i = idx; i < buf_size; i++) {
            move_t move = buf[i];

            if(!move.is_capture) {
                if (search.killer_heur.primary(ply) == move) {
                    buf_scores[i] = 20003;
                } else if (search.killer_heur.primary(ply) == move) {
                    buf_scores[i] = 20002;
                } else if(ply >= 2 && (search.killer_heur.primary(ply - 2) == move)) {
                    buf_scores[i] = 20001;
                } else {
                    buf_scores[i] = search.history_heur.get(move);
                }
            } else {
                buf_scores[i] = CAPT_BASE;
                buf_scores[i] += (board.see(move)
                                  + record.next_move ? rank_index(move.to) + 1 : 8 - rank_index(move.to));
            }
        }

        scored = true;
    }

    // Pick moves
    int best_idx = idx;
    for(int i = idx; i < buf_size; i++) {
        if(buf_scores[i] > buf_scores[best_idx]) {
            best_idx = idx;
        }
    }

    buf_swap(best_idx, idx);
    int move_score = buf_scores[idx];
    move_t move = next();
    if(move.is_capture && move_score >= CAPT_BASE) {
        stage = GEN_GOOD_CAPT;
    } else if(move == search.killer_heur.primary(ply)
              || move == search.killer_heur.secondary(ply)
              || (ply >= 2 && (search.killer_heur.primary(ply - 2) == move))) {
        stage = GEN_KILLERS;
    } else if(!move.is_capture) {
        stage = GEN_QUIETS;
    } else {
        stage = GEN_BAD_CAPT;
    }

    return move;
}

move_t movegen_t::next() {
    return buf[idx++];
}

bool movegen_t::has_next() {
    return idx < buf_size;
}

void movegen_t::buf_swap(int i, int j) {
    move_t temp = buf[i];
    buf[i] = buf[j];
    buf[j] = temp;

    int temp_score = buf_scores[i];
    buf_scores[i] = buf_scores[j];
    buf_scores[j] = temp_score;
}

move_t *movegen_t::get_searched(int &len) {
    len = idx;
    return buf;
}
