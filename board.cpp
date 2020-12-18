//
// Created by Vincent on 22/09/2017.
//

#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <cstring>

#include "board.h"
#include "move.h"
#include "hash.h"

void board_t::move(move_t move) {
    // Insert a new record
    record.push_back(record.back());
    record.back().prev_move = move;

    // Update side hash
    record.back().next_move = (Team) !record.back().next_move;
    record.back().hash ^= zobrist::side;

    // Update ep hash
    if (record.crbegin()[1].ep_square != 0) {
        record.back().ep_square = 0;
        record.back().hash ^= zobrist::ep[record.crbegin()[1].ep_square];
    }

    if (move != EMPTY_MOVE) {
        auto team = Team(move.info.team);
        auto x_team = Team(!move.info.team);

        // Update halfmove clock
        if (move.info.piece == PAWN || move.info.is_capture) {
            record.back().halfmove_clock = 0;
        } else {
            record.back().halfmove_clock++;
        }

        if (move.info.is_capture && move.info.captured_type == ROOK) {
            if (move.info.to == (x_team ? H8 : H1) && record.back().castle[x_team][0]) {
                record.back().castle[x_team][0] = false;
                record.back().hash ^= zobrist::castle[x_team][0];
            } else if (move.info.to == (x_team ? A8 : A1) && record.back().castle[x_team][1]) {
                record.back().castle[x_team][1] = false;
                record.back().hash ^= zobrist::castle[x_team][1];
            }
        }

        if (move.info.piece == PAWN) {
            if (move.info.is_ep) {
                // Remove captured pawn
                switch_piece<true>(x_team, PAWN, team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8));
            } else if (move.info.is_capture) {
                switch_piece<true>(x_team, (Piece) move.info.captured_type, move.info.to);
            }

            if (move.info.is_promotion) {
                switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
                switch_piece<true>(team, (Piece) move.info.promotion_type, move.info.to);
            } else {
                switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
                switch_piece<true>(team, (Piece) move.info.piece, move.info.to);
            }

            // Update en-passant square
            if (team ? move.info.to - move.info.from == -16 : move.info.to - move.info.from == 16) {
                record.back().ep_square = team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8);
                record.back().hash ^= zobrist::ep[record.back().ep_square];
            }
        } else {
            // Update castling hashes for moving rook
            if (move.info.piece == ROOK) {
                if (team) {
                    if (move.info.from == H8 && record.back().castle[team][0]) {
                        record.back().hash ^= zobrist::castle[team][0];
                        record.back().castle[team][0] = false;
                    } else if (move.info.from == A8 && record.back().castle[team][1]) {
                        record.back().hash ^= zobrist::castle[team][1];
                        record.back().castle[team][1] = false;
                    }
                } else {
                    if (move.info.from == H1 && record.back().castle[team][0]) {
                        record.back().hash ^= zobrist::castle[team][0];
                        record.back().castle[team][0] = false;
                    } else if (move.info.from == A1 && record.back().castle[team][1]) {
                        record.back().hash ^= zobrist::castle[team][1];
                        record.back().castle[team][1] = false;
                    }
                }
            } else if (move.info.piece == KING) {
                // Update castling hashes
                if (record.back().castle[team][0]) {
                    record.back().hash ^= zobrist::castle[team][0];
                    record.back().castle[team][0] = false;
                }
                if (record.back().castle[team][1]) {
                    record.back().hash ^= zobrist::castle[team][1];
                    record.back().castle[team][1] = false;
                }

                if (move.info.castle) {
                    // Move rook
                    switch_piece<true>(team, ROOK,
                                       move.info.castle_side ? (team ? A8 : A1) : (team ? H8 : H1));
                    switch_piece<true>(team, ROOK,
                                       move.info.castle_side ? (team ? D8 : D1) : (team ? F8 : F1));
                }
            }

            if (move.info.is_capture) {
                switch_piece<true>(x_team, (Piece) move.info.captured_type, move.info.to);
            }
            switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
            switch_piece<true>(team, (Piece) move.info.piece, move.info.to);
        }
    }
}

void board_t::unmove() {
    move_t move = record.back().prev_move;
    record.pop_back();

    if (move != EMPTY_MOVE) {
        if (move.info.piece == PAWN) {
            if (move.info.is_promotion) {
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
                switch_piece<false>((Team) move.info.team, (Piece) move.info.promotion_type, move.info.to);
            } else {
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.to);
            }

            if (move.info.is_ep) {
                // Replace captured pawn
                switch_piece<false>((Team) !move.info.team, PAWN,
                                    move.info.team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8));
            } else if (move.info.is_capture) {
                switch_piece<false>((Team) !move.info.team, (Piece) move.info.captured_type, move.info.to);
            }
        } else {
            if (move.info.castle) {
                // Move rook
                switch_piece<false>((Team) move.info.team, ROOK,
                                    move.info.castle_side ? (move.info.team ? A8 : A1) : (move.info.team ? H8 : H1));
                switch_piece<false>((Team) move.info.team, ROOK,
                                    move.info.castle_side ? (move.info.team ? D8 : D1) : (move.info.team ? F8 : F1));
            }

            switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
            switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.to);

            if (move.info.is_capture) {
                switch_piece<false>((Team) !move.info.team, (Piece) move.info.captured_type, move.info.to);
            }
        }
    }
}

board_t::board_t(const std::string &fen) {
    std::istringstream iss(fen);
    std::vector<std::string> split((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

    if (split.size() > 6 || split.size() < 4) {
        throw std::runtime_error("fen: not enough sections: " + std::to_string(split.size()));
    }

    record.emplace_back();

    // Parse board
    uint8_t file = 0, rank = 7;
    for (char ch : split[0]) {
        if (ch > '0' && ch < '9') {
            file += ch - '0';
        } else {
            uint8_t square = square_index(file, rank);
            if (ch == '/') {
                --rank;
                file = 0;
            } else {
                ++file;
                Team team = Team(!std::isupper(ch));
                Piece piece;
                switch (std::tolower(ch)) {
                    case 'p': piece = PAWN; break;
                    case 'n': piece = KNIGHT; break;
                    case 'b': piece = BISHOP; break;
                    case 'r': piece = ROOK; break;
                    case 'q': piece = QUEEN; break;
                    case 'k': piece = KING; break;
                    default: throw std::runtime_error(std::string("fen: invalid character: '") + ch + "'");
                }
                switch_piece<true>(team, piece, square);
            }
        }
    }

    // Parse colour
    if (split[1][0] == 'w') {
        record[0].next_move = WHITE;
    } else if (split[1][0] == 'b') {
        record[0].next_move = BLACK;
        record[0].hash ^= zobrist::side;
    } else {
        throw std::runtime_error("fen: invalid team: " + split[1]);
    }

    // Parse castling
    if (split[2] != "-") {
        for (char i : split[2]) {
            if (i == 'K') {
                record[0].castle[WHITE][0] = true;
                record[0].hash ^= zobrist::castle[WHITE][0];
            } else if (i == 'Q') {
                record[0].castle[WHITE][1] = true;
                record[0].hash ^= zobrist::castle[WHITE][1];
            } else if (i == 'k') {
                record[0].castle[BLACK][0] = true;
                record[0].hash ^= zobrist::castle[BLACK][0];
            } else if (i == 'q') {
                record[0].castle[BLACK][1] = true;
                record[0].hash ^= zobrist::castle[BLACK][1];
            } else {
                throw std::runtime_error("fen: invalid castling rights: " + std::string(1, i));
            }
        }
    }

    // Parse ep square
    if (split[3] != "-") {
        if (split[3].length() != 2) {
            throw std::runtime_error("en-passant square has invalid length");
        } else {
            record[0].ep_square = to_sq(split[3][0], split[3][1]);
        }
    }

    // Parse halfmove clock
    if (split.size() > 4) {
        record[0].halfmove_clock = std::stoi(split[4]);
    } else {
        record[0].halfmove_clock = 0;
    }

    // Fullmove number is not needed
}

// This does not properly consider legality:
//  - if promotion is actually possible
//  - if the piece can actually make the move
//  - if the piece actually exists
// If the move cannot be parsed, 0000 is returned
move_t board_t::parse_move(const std::string &str) const {
    packed_move_t packed_move = {};

    if (str.length() != 4 && str.length() != 5) {
        return EMPTY_MOVE;
    }
    
    try {
        packed_move.from = to_sq(str[0], str[1]);
        packed_move.to = to_sq(str[2], str[3]);
    } catch (std::runtime_error &e) {
        return EMPTY_MOVE;
    }
    
    if(str.length() == 5) {
        switch (str[4]) {
            case 'n':
                packed_move.type = KNIGHT;
                break;
            case 'b':
                packed_move.type = BISHOP;
                break;
            case 'r':
                packed_move.type = ROOK;
                break;
            case 'q':
                packed_move.type = QUEEN;
                break;
            default:
                return EMPTY_MOVE;
        }
    }

    return to_move(packed_move);
}

move_t board_t::to_move(packed_move_t packed_move) const {
    move_t move{};
    
    move.info.from = static_cast<uint8_t>(packed_move.from);
    move.info.to = static_cast<uint8_t>(packed_move.to);
    
    // Piece
    move.info.team = sq_data[move.info.from].team();
    move.info.piece = sq_data[move.info.from].piece();

    // Capture
    move.info.is_capture = static_cast<uint16_t>(sq_data[move.info.to].occupied());
    move.info.captured_type = sq_data[move.info.to].occupied() ? sq_data[move.info.to].piece() : 0;

    // Promotion
    move.info.is_promotion = static_cast<uint16_t>(packed_move.type != 0 && (move.info.to <= H1 || move.info.to >= A8));
    if (move.info.is_promotion) {
        move.info.promotion_type = packed_move.type;
    }

    // Castling
    if (move.info.piece == KING) {
        if (move.info.from == E1 || move.info.from == E8) {
            if (move.info.to == G1 || move.info.to == G8) {
                move.info.castle = 1;
                move.info.castle_side = 0;
            } else if (move.info.to == C1 || move.info.to == C8) {
                move.info.castle = 1;
                move.info.castle_side = 1;
            }
        }
    }

    // EP
    move.info.is_capture |= move.info.is_ep = static_cast<uint16_t>(record.back().ep_square != 0
                                            && move.info.piece == PAWN
                                            && move.info.to == record.back().ep_square);

    return move;
}

bool board_t::is_illegal() const {
    Team side = record.back().next_move;
    uint8_t king_square = bit_scan(bb_pieces[!side][KING]);

    return is_attacked(king_square, side);
}


bool board_t::is_incheck() const {
    Team side = record.back().next_move;
    uint8_t king_square = bit_scan(bb_pieces[side][KING]);

    return is_attacked(king_square, Team(!side));
}

template<bool HASH>
void board_t::switch_piece(Team side, Piece piece, uint8_t sq) {
    const U64 bb_square = single_bit(sq);

    // Occupancy
    sq_data[sq].occupied(!sq_data[sq].occupied());
    sq_data[sq].piece(piece);
    sq_data[sq].team(side);

    { // Update bitboards
        // No error checking! We assume that the piece exists
        bb_all ^= bb_square;
        bb_side[side] ^= bb_square;
        bb_pieces[side][piece] ^= bb_square;
    }

    if (HASH) { // Update hash
        U64 square_hash = zobrist::squares[sq][side][piece];
        record.back().hash ^= square_hash;
        if(piece == PAWN || piece == KING) record.back().kp_hash ^= square_hash;
        if(sq_data[sq].occupied()) record.back().material.inc(side, piece);
        else record.back().material.dec(side, piece);
    }
}

U64 board_t::attacks_to(uint8_t sq, Team side) const {
    auto x_side = Team(!side);
    U64 attacks =
            (find_moves<QUEEN>(side, sq, bb_all) & bb_pieces[x_side][QUEEN]) |
            (find_moves<ROOK>(side, sq, bb_all) & bb_pieces[x_side][ROOK]) |
            (find_moves<KNIGHT>(side, sq, bb_all) & bb_pieces[x_side][KNIGHT]) |
            (find_moves<BISHOP>(side, sq, bb_all) & bb_pieces[x_side][BISHOP]) |
            (find_moves<PAWN>(side, sq, bb_all) & bb_pieces[x_side][PAWN]) |
            (find_moves<KING>(side, sq, bb_all) & bb_pieces[x_side][KING]);

    return attacks;
}

bool board_t::is_attacked(uint8_t sq, Team side) const {
    return is_attacked(sq, side, bb_all);
}


bool board_t::is_attacked(uint8_t sq, Team side, U64 occupied) const {
    auto x_side = Team(!side);
    return (find_moves<QUEEN>(x_side, sq, occupied) & bb_pieces[side][QUEEN]) ||
           (find_moves<ROOK>(x_side, sq, occupied) & bb_pieces[side][ROOK]) ||
           (find_moves<KNIGHT>(x_side, sq, occupied) & bb_pieces[side][KNIGHT]) ||
           (find_moves<BISHOP>(x_side, sq, occupied) & bb_pieces[side][BISHOP]) ||
           (find_moves<PAWN>(x_side, sq, occupied) & bb_pieces[side][PAWN]) ||
           (find_moves<KING>(x_side, sq, occupied) & bb_pieces[side][KING]);
}


bool board_t::is_pseudo_legal(move_t move) const {
    if (move == EMPTY_MOVE) return false;

    auto team = Team(move.info.team);
    auto x_team = Team(!move.info.team);

    if (record.back().next_move != team) return false;

    if (move.info.castle) {
        if (record.back().castle[move.info.team][move.info.castle_side] == 0) return false;
        if (move.info.castle_side == 0) {
            return (bb_all & bits_between(team ? E8 : E1, team ? H8 : H1)) == 0 &&
                   !is_attacked(team ? E8 : E1, x_team) &&
                   !is_attacked(team ? F8 : F1, x_team) &&
                   !is_attacked(team ? G8 : G1, x_team);
        } else {
            return (bb_all & bits_between(team ? E8 : E1, team ? A8 : A1)) == 0 &&
                   !is_attacked(team ? E8 : E1, x_team) &&
                   !is_attacked(team ? D8 : D1, x_team) &&
                   !is_attacked(team ? C8 : C1, x_team);
        }
    }

    if (move.info.is_capture && !move.info.is_ep) {
        if ((bb_pieces[x_team][move.info.captured_type] & single_bit(move.info.to)) == 0) return false;
    } else {
        if ((bb_side[x_team] & single_bit(move.info.to)) != 0) return false;
    }

    if(move.info.is_ep) {
        if(record.back().ep_square == 0 || move.info.to != record.back().ep_square) {
            return false;
        }
    } else {
        if ((find_moves(Piece(move.info.piece), team, move.info.from, bb_all) & single_bit(move.info.to)) == 0) {
            return false;
        }
    }

    if(move.info.piece == PAWN) {
        if ((move.info.to <= H1 || move.info.to >= A8) == !move.info.is_promotion) return false;
    } else {
        if (move.info.is_promotion) return false;
    }

    return (bb_pieces[team][move.info.piece] & single_bit(move.info.from)) &&
           !(bb_side[team] & single_bit(move.info.to));
}

// Assumes the move is pseudo-legal
bool board_t::is_legal(move_t move) const {
    Team side = Team(move.info.team);
    Team x_side = Team(!side);
    uint8_t king_square = bit_scan(bb_pieces[side][KING]);

    U64 checkers = attacks_to(king_square, side);
    if(checkers != 0) {
        if(move.info.piece == KING) {
            U64 occupied = bb_all ^ single_bit(move.info.from);
            if(!move.info.is_capture) occupied ^= single_bit(move.info.to);

            return !is_attacked(move.info.to, x_side, occupied);
        } else {
            U64 occupied = bb_all ^ single_bit(move.info.from);
            if(move.info.is_ep) {
                if((checkers ^ single_bit(uint8_t(rel_offset(Team(move.info.team), D_S) + move.info.to))) == 0) {
                    occupied ^= single_bit(uint8_t(rel_offset(Team(move.info.team), D_S) + move.info.to))
                            | single_bit(move.info.to);
                    return !(find_moves<BISHOP>(side, king_square, occupied) & (bb_pieces[x_side][BISHOP] | bb_pieces[x_side][QUEEN]))
                           && !(find_moves<ROOK>(side, king_square, occupied) & (bb_pieces[x_side][ROOK] | bb_pieces[x_side][QUEEN]));
                } else {
                    return false;
                }
            } if(move.info.is_capture && (checkers ^ single_bit(move.info.to)) == 0) {
                return !(find_moves<BISHOP>(side, king_square, occupied) & (bb_pieces[x_side][BISHOP] | bb_pieces[x_side][QUEEN]) & ~checkers)
                       && !(find_moves<ROOK>(side, king_square, occupied) & (bb_pieces[x_side][ROOK] | bb_pieces[x_side][QUEEN]) & ~checkers);
            } else {
                occupied ^= single_bit(move.info.to);
                return !is_attacked(king_square, x_side, occupied);
            }
        }
    } else {
        if(move.info.piece == KING) {
            return !is_attacked(move.info.to, x_side);
        } else {
            U64 occupied = bb_all ^ single_bit(move.info.from);
            U64 captured = 0;
            if (move.info.is_ep) {
                occupied ^= single_bit(uint8_t(rel_offset(Team(move.info.team), D_S) + move.info.to));
                occupied ^= single_bit(move.info.to);
            } else if (!move.info.is_capture) {
                occupied ^= single_bit(move.info.to);
            } else {
                captured ^= single_bit(move.info.to);
            }

            return !(find_moves<BISHOP>(side, king_square, occupied) & (bb_pieces[x_side][BISHOP] | bb_pieces[x_side][QUEEN]) & ~captured)
                && !(find_moves<ROOK>(side, king_square, occupied) & (bb_pieces[x_side][ROOK] | bb_pieces[x_side][QUEEN]) & ~captured);
        }
    }
}

// Assumes the move is both pseudo legal and legal
bool board_t::gives_check(move_t move) const {
    Team side = Team(move.info.team);
    Team x_side = Team(!move.info.team);
    uint8_t king_square = bit_scan(bb_pieces[x_side][KING]);

    if(find_moves(Piece(move.info.piece), side, move.info.to, bb_all) & bb_pieces[x_side][KING]) {
        return true;
    } else if(move.info.castle) {
        U64 occupied = bb_all ^ single_bit(move.info.from) ^ single_bit(move.info.to);
        return (find_moves<ROOK>(side,
                                 side ? (move.info.castle_side ? D8 : F8) : (move.info.castle_side ? D1 : F1), occupied)
                & bb_pieces[x_side][KING]) != 0;
    } else if (move.info.is_promotion && (find_moves(Piece(move.info.promotion_type), side, move.info.to,
            (bb_all ^ single_bit(move.info.from)) | single_bit(move.info.to)) & bb_pieces[x_side][KING])) {
        return true;
    } else {
        U64 occupied = bb_all ^ single_bit(move.info.from);
        if (move.info.is_ep) {
            occupied ^= single_bit(uint8_t(rel_offset(Team(move.info.team), D_S) + move.info.to));
            occupied ^= single_bit(move.info.to);
        } else if (!move.info.is_capture) {
            occupied ^= single_bit(move.info.to);
        }

        return (find_moves<BISHOP>(x_side, king_square, occupied) & (bb_pieces[side][BISHOP] | bb_pieces[side][QUEEN]))
               || (find_moves<ROOK>(x_side, king_square, occupied) & (bb_pieces[side][ROOK] | bb_pieces[side][QUEEN]));
    }
}

bool board_t::is_repetition_draw(int search_ply) const {
    int rep = 1;

    int max = record.back().halfmove_clock;

    for (int i = 2; i <= max; i += 2) {
        if (record.crbegin()[i].hash == record.back().hash) rep++;
        if (rep >= 3) return true;
        if (rep >= 2 && i < search_ply) return true;
    }

    return false;
}

bool board_t::is_material_draw() const {
    if(record.back().material.count(WHITE, PAWN) || record.back().material.count(BLACK, PAWN) ||
            record.back().material.count(WHITE, QUEEN) || record.back().material.count(BLACK, QUEEN) ||
            record.back().material.count(WHITE, ROOK) || record.back().material.count(BLACK, ROOK)) {
        return false;
    } else {
        return record.back().material.count(WHITE, BISHOP) + record.back().material.count(BLACK, BISHOP)
               + record.back().material.count(WHITE, KNIGHT) + record.back().material.count(BLACK, KNIGHT) <= 1;
    }
}

void board_t::mirror() {
    // Mirror side
    record.back().next_move = (Team) !record.back().next_move;
    record.back().hash ^= zobrist::side;

    // Mirror en-passant
    if (record.back().ep_square != 0) {
        record.back().hash ^= zobrist::ep[record.back().ep_square];
        record.back().ep_square = MIRROR_TABLE[record.back().ep_square];
        record.back().hash ^= zobrist::ep[record.back().ep_square];
    }

    // Mirror castling rights
    if(record.back().castle[0][0] != record.back().castle[1][0]) {
        record.back().hash ^= zobrist::castle[0][0];
        record.back().hash ^= zobrist::castle[1][0];
        std::swap(record.back().castle[0][0], record.back().castle[1][0]);
    }
    if(record.back().castle[0][1] != record.back().castle[1][1]) {
        record.back().hash ^= zobrist::castle[0][1];
        record.back().hash ^= zobrist::castle[1][1];
        std::swap(record.back().castle[0][1], record.back().castle[1][1]);
    }

    // Mirror pieces
    sq_data_t old_data[64];
    memcpy(old_data, sq_data, 64 * sizeof(sq_data_t));
    for (uint8_t sq = 0; sq < 64; sq++) {
        if(old_data[sq].occupied()) switch_piece<true>(old_data[sq].team(), old_data[sq].piece(), sq);
    }
    for (uint8_t sq = 0; sq < 64; sq++) {
        if(old_data[sq].occupied()) switch_piece<true>(Team(!old_data[sq].team()), old_data[sq].piece(), MIRROR_TABLE[sq]);
    }
}

int board_t::see(move_t move) const {
    if(move == EMPTY_MOVE || move.info.is_ep)
        return 0;

    // State
    U64 attackers = attacks_to(move.info.to, Team(move.info.team)) | attacks_to(move.info.to, Team(!move.info.team));
    U64 occupation_mask = ONES;
    int current_target_val = 0;
    bool prom_rank = rank_index(move.info.to) == 0 || rank_index(move.info.to) == 7;
    auto next_move = Team(move.info.team);

    // Material table
    int num_capts = 0;
    int material[32];

    // Eval move
    material[num_capts] = sq_data[move.info.to].occupied() ? VAL[sq_data[move.info.to].piece()] : 0;
    current_target_val = VAL[move.info.piece];
    if (prom_rank && move.info.piece == PAWN) {
        material[num_capts] += VAL[move.info.promotion_type] - VAL[PAWN];
        current_target_val += VAL[move.info.promotion_type] - VAL[PAWN];
    }
    num_capts++;

    // Remove attacker
    attackers &= ~single_bit(move.info.from);
    occupation_mask &= ~single_bit(move.info.from);

    // Reveal next attacker
    attackers |= (find_moves<QUEEN>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][QUEEN] | bb_pieces[BLACK][QUEEN])) |
                 (find_moves<BISHOP>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][BISHOP] | bb_pieces[BLACK][BISHOP])) |
                 (find_moves<ROOK>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][ROOK] | bb_pieces[BLACK][ROOK]));
    attackers &= occupation_mask;

    next_move = Team(!next_move);

    while (attackers) {
        // Pick LVA
        Square from;
        if (!prom_rank && attackers & bb_pieces[next_move][PAWN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][PAWN]));
        else if (attackers & bb_pieces[next_move][KNIGHT])
            from = Square(bit_scan(attackers & bb_pieces[next_move][KNIGHT]));
        else if (attackers & bb_pieces[next_move][BISHOP])
            from = Square(bit_scan(attackers & bb_pieces[next_move][BISHOP]));
        else if (attackers & bb_pieces[next_move][ROOK])
            from = Square(bit_scan(attackers & bb_pieces[next_move][ROOK]));
        else if (prom_rank && attackers & bb_pieces[next_move][PAWN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][PAWN]));
        else if (attackers & bb_pieces[next_move][QUEEN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][QUEEN]));
        else if (attackers & bb_pieces[next_move][KING] && !(attackers & bb_side[!next_move]))
            from = Square(bit_scan(attackers & bb_pieces[next_move][KING]));
        else break;

        // Eval move
        material[num_capts] = -material[num_capts - 1] + current_target_val;
        current_target_val = VAL[sq_data[from].piece()];
        if (prom_rank && sq_data[from].piece() == PAWN) {
            material[num_capts] += VAL[QUEEN] - VAL[PAWN];
            current_target_val = VAL[QUEEN] - VAL[PAWN];
        }
        num_capts++;

        // Remove attacker
        attackers &= ~single_bit(from);
        occupation_mask &= ~single_bit(from);

        // Reveal next attacker
        attackers |= (find_moves<QUEEN>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][QUEEN] | bb_pieces[BLACK][QUEEN])) |
                     (find_moves<BISHOP>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][BISHOP] | bb_pieces[BLACK][BISHOP])) |
                     (find_moves<ROOK>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][ROOK] | bb_pieces[BLACK][ROOK]));
        attackers &= occupation_mask;

        next_move = Team(!next_move);
    }

    // Calculate SEE of first capture
    while (--num_capts)
        if (material[num_capts] > -material[num_capts - 1])
            material[num_capts - 1] = -material[num_capts];

    return material[0];
}

U64 board_t::non_pawn_material(Team side) const {
    return (bb_side[side] ^ bb_pieces[side][PAWN] ^ bb_pieces[side][KING]);
}

void board_t::print() {
    std::cout << *this;
}
