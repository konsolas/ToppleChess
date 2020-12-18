//
// Created by Vincent on 01/06/2018.
//

#include <catch2/catch.hpp>
#include "util.h"
#include "../board.h"

TEST_CASE("Static Exchange Evaluation") {
    REQUIRE_NOTHROW(init_tables());

    SECTION("Petroff") {
        board_t board("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");

        REQUIRE(board.see(board.parse_move("f3e5")) == VAL[PAWN]);
        REQUIRE(board.see(board.parse_move("f3d4")) == -VAL[KNIGHT]);
        REQUIRE(board.see(board.parse_move("f1a6")) == -VAL[BISHOP]);
        REQUIRE(board.see(board.parse_move("f3g5")) == 0);
        REQUIRE(board.see(board.parse_move("d1e2")) == 0);
    }

    SECTION("Two Knight's defence") {
        board_t board("r1bqkb1r/ppp2ppp/2n2n2/3pp1N1/2B1P3/8/PPPP1PPP/RNBQK2R w KQkq - 0 5");

        REQUIRE(board.see(board.parse_move("e4d5")) == 0);
        REQUIRE(board.see(board.parse_move("c4d5")) == 0);
        REQUIRE(board.see(board.parse_move("g5f7")) == (VAL[PAWN] - VAL[KNIGHT]));
        REQUIRE(board.see(board.parse_move("d1g4")) == -VAL[QUEEN]);
        REQUIRE(board.see(board.parse_move("g5e6")) == -VAL[KNIGHT]);
    }

    SECTION("Grand Prix attack") {
        board_t board("r1bqk2r/pp2npbp/2n1p1p1/2pp4/4PP2/P1NP1N2/BPP3PP/R1BQK2R b KQkq - 1 8");

        REQUIRE(board.see(board.parse_move("d5e4")) == 0);
        REQUIRE(board.see(board.parse_move("g7c3")) == 0);
        REQUIRE(board.see(board.parse_move("c5c4")) == -VAL[PAWN]);
        REQUIRE(board.see(board.parse_move("d5d4")) == 0);
        REQUIRE(board.see(board.parse_move("g7e5")) == -VAL[BISHOP]);
        REQUIRE(board.see(board.parse_move("e8g8")) == 0);
    }

    SECTION("Benko gambit") {
        board_t board("rn1q1rk1/1b1pppbp/p4np1/1PpP4/P1B5/2N1P3/1P2NPPP/R1BQK2R b KQ - 4 9");

        REQUIRE(board.see(board.parse_move("b5a6")) == 0);
        REQUIRE(board.see(board.parse_move("d5d6")) == 0);
        REQUIRE(board.see(board.parse_move("b5b6")) == -VAL[PAWN]);
        REQUIRE(board.see(board.parse_move("e3e4")) == 0);
        REQUIRE(board.see(board.parse_move("d1d4")) == (VAL[PAWN] - VAL[QUEEN]));
    }
}