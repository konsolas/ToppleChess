//
// Created by Vincent on 30/09/2017.
//

#include "../catch.hpp"
#include "../util.h"
#include "../../board.h"
#include "../../movegen.h"
#include "../../eval.h"


const bool operator==(const board_t& lhs, const board_t& rhs) {
    for(uint8_t sq = 0; sq < 64; sq++) {
        if(lhs.sq_data[sq].occupied != rhs.sq_data[sq].occupied) return false;
        if(lhs.sq_data[sq].occupied) {
            if(lhs.sq_data[sq].piece != rhs.sq_data[sq].piece) return false;
            if(lhs.sq_data[sq].team != rhs.sq_data[sq].team) return false;
        }
    }

    return lhs.record[lhs.now].hash == rhs.record[rhs.now].hash;
}

const bool operator!=(const board_t& lhs, const board_t& rhs) {
    return !(lhs == rhs);
}

const void consistency_check(board_t &board) {
    int score = eval(board);
    board.mirror();
    int mirrorscore = eval(board);
    board.mirror();

    INFO("position: " << board);
    REQUIRE(score == -mirrorscore);
}

const void hash_check(const board_t &board) {
    // Generate hash of board
    U64 hash = 0;
    for(int i = 0; i < 64; i++) {
        if(board.sq_data[i].occupied) {
            hash ^= zobrist::squares[i][board.sq_data[i].team][board.sq_data[i].piece];
        }
    }

    if(board.record[board.now].next_move) hash ^= zobrist::side;

    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            if(board.record[board.now].castle[i][j]) hash ^= zobrist::castle[i][j];
        }
    }

    if(board.record[board.now].ep_square != 0) {
        hash ^= zobrist::ep[board.record[board.now].ep_square];
    }

    INFO("position: " << board);
    REQUIRE(hash == board.record[board.now].hash);
}

U64 perft(board_t &board, int depth) {
    //consistency_check(board);
    hash_check(board);

    if (depth == 0) {
        return 1;
    }

    U64 count = 0;
    board_t snapshot = board;

    movegen_t gen(board);
    gen.gen_normal();

    while (gen.has_next()) {
        move_t next = gen.next();

        board.move(next);

        if (!board.is_illegal()) {
            count += perft(board, depth - 1);
        }

        board.unmove();

        if(board != snapshot) {
            FAIL("Unmake move failed at position: " << board
                                                    << "expecting " << snapshot
                                                    << " move=" << from_sq(next.info.from) << from_sq(next.info.to));
        }
    }

    return count;
}

TEST_CASE("Perft") {
    init_tables();
    zobrist::init_hashes();
    eval_init();

    // Test perft
    SECTION("(test) rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ") {
        board_t board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ");
        REQUIRE(perft(board, 2) == 1486);
    }

    // Easy perft
    SECTION("(easy) rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -") {
        board_t board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
        REQUIRE(perft(board, 1) == 20);
    }
    SECTION("(easy) r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -") {
        board_t board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
        REQUIRE(perft(board, 1) == 48);
    }
    SECTION("(easy) 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -") {
        board_t board("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
        REQUIRE(perft(board, 1) == 14);
    }
    SECTION("(easy) r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -") {
        board_t board("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -");
        REQUIRE(perft(board, 1) == 6);
    }
    SECTION("(easy) r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -") {
        board_t board("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -");
        REQUIRE(perft(board, 1) == 6);
    }
    SECTION("(easy) rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ") {
        board_t board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -");
        REQUIRE(perft(board, 1) == 44);
    }
    SECTION("(easy) r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -") {
        board_t board("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -");
        REQUIRE(perft(board, 1) == 46);
    }


    // Hard perft
    SECTION("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -") {
        board_t board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
        REQUIRE(perft(board, 5) == 4865609);
    }

    SECTION("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -") {
        board_t board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
        REQUIRE(perft(board, 4) == 4085603);
    }
    SECTION("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -") {
        board_t board("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
        REQUIRE(perft(board, 6) == 11030083);
    }
    SECTION("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -") {
        board_t board("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -");
        REQUIRE(perft(board, 5) == 15833292);
    }
    SECTION("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -") {
        board_t board("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -");
        REQUIRE(perft(board, 5) == 15833292);
    }
    SECTION("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ") {
        board_t board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -");
        REQUIRE(perft(board, 4) == 2103487);
    }
    SECTION("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -") {
        board_t board("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -");
        REQUIRE(perft(board, 4) == 3894594);
    }
}