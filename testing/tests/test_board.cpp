//
// Created by Vincent on 29/09/2017.
//

#include <iostream>
#include "../catch.hpp"
#include "../util.h"
#include "../../board.h"

TEST_CASE("Board representation") {
    REQUIRE_NOTHROW(init_tables());

    U64 expected;
    board_t board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    SECTION("Initialised correctly") {
        REQUIRE(board.record[board.now].next_move == WHITE);
        REQUIRE(board.record[board.now].ep_square == 0);
        REQUIRE(board.record[board.now].castle[0][0] == 1);
        REQUIRE(board.record[board.now].castle[0][1] == 1);
        REQUIRE(board.record[board.now].castle[1][0] == 1);
        REQUIRE(board.record[board.now].castle[1][1] == 1);
        REQUIRE(board.record[board.now].halfmove_clock == 0);

        expected = c_u64({1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1});
        REQUIRE(board.bb_all == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1});
        REQUIRE(board.bb_side[WHITE] == expected);

        expected = c_u64({1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_side[BLACK] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[WHITE][PAWN] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][PAWN] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 0, 0, 0, 0, 0, 0, 1});
        REQUIRE(board.bb_pieces[WHITE][ROOK] == expected);

        expected = c_u64({1, 0, 0, 0, 0, 0, 0, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][ROOK] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 1, 0, 0, 0, 0, 1, 0});
        REQUIRE(board.bb_pieces[WHITE][KNIGHT] == expected);

        expected = c_u64({0, 1, 0, 0, 0, 0, 1, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][KNIGHT] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 1, 0, 0, 1, 0, 0});
        REQUIRE(board.bb_pieces[WHITE][BISHOP] == expected);

        expected = c_u64({0, 0, 1, 0, 0, 1, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][BISHOP] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 1, 0, 0, 0});
        REQUIRE(board.bb_pieces[WHITE][KING] == expected);

        expected = c_u64({0, 0, 0, 0, 1, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][KING] == expected);

        expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 1, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[WHITE][QUEEN] == expected);

        expected = c_u64({0, 0, 0, 1, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0});
        REQUIRE(board.bb_pieces[BLACK][QUEEN] == expected);
    }

    SECTION("Basic move") {
        move_t move = EMPTY_MOVE;
        move.info.from = E2;
        move.info.to = E4;
        move.info.piece = PAWN;

        board.move(move);

        REQUIRE(board.record[board.now].next_move == BLACK);
        REQUIRE(board.record[board.now].ep_square == E3);
        REQUIRE(board.record[board.now].castle[0][0] == 1);
        REQUIRE(board.record[board.now].castle[0][1] == 1);
        REQUIRE(board.record[board.now].castle[1][0] == 1);
        REQUIRE(board.record[board.now].castle[1][1] == 1);
        REQUIRE(board.record[board.now].halfmove_clock == 0);

        expected = c_u64({1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 1, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 0, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1});
        REQUIRE(board.bb_all == expected);
        REQUIRE(!board.sq_data[E2].occupied);

        // Undo the move
        board.unmove();

        // Recheck original position
        REQUIRE(board.record[board.now].next_move == WHITE);
        REQUIRE(board.record[board.now].ep_square == 0);
        REQUIRE(board.record[board.now].castle[0][0] == 1);
        REQUIRE(board.record[board.now].castle[0][1] == 1);
        REQUIRE(board.record[board.now].castle[1][0] == 1);
        REQUIRE(board.record[board.now].castle[1][1] == 1);
        REQUIRE(board.record[board.now].halfmove_clock == 0);

        expected = c_u64({1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1});
        REQUIRE(board.bb_all == expected);
        REQUIRE(board.sq_data[E2].occupied);
    }
}

TEST_CASE("Legality") {
    board_t board1("2bq1bn1/2pppk2/np2N3/r3P1pB/p2N4/8/PPPPKPP1/RNB2r2 b - - 1 2");
    REQUIRE(board1.is_incheck());

    board_t board2("rnbqkb1r/ppp2ppp/3p4/8/4n3/3N4/PPPP1PPP/RNBQKB1R w KQkq - 0 5");
    INFO(((find_moves(PAWN, WHITE, D2, board2.bb_all) & single_bit(D4)) == 0))
    REQUIRE(!board2.is_pseudo_legal(board2.parse_move("d2d4")));
}