//
// Created by Vincent on 27/09/2017.
//

#include "../catch.hpp"
#include "../util.h"

TEST_CASE("Bitboard engine") {
    SECTION("Table initialisation") {
        REQUIRE_NOTHROW(init_tables());
    }

    SECTION("General operations") {
        SECTION("Single bit") {
            REQUIRE(single_bit(E4) == U64(1) << E4);
            REQUIRE(single_bit(A1) == U64(1) << A1);
            REQUIRE(single_bit(H8) == U64(1) << H8);
            REQUIRE(single_bit(A8) == U64(1) << A8);
            REQUIRE(single_bit(H1) == U64(1) << H1);
            REQUIRE(single_bit(G2) == U64(1) << G2);
        }

        SECTION("popLSB") {
            U64 bb = c_u64({0, 0, 0, 0, 0, 0, 1, 0,
                            0, 1, 0, 1, 0, 0, 0, 1,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 1, 1, 1, 0, 0, 0});

            REQUIRE(pop_bit(bb) == C1);
            REQUIRE(pop_bit(bb) == D1);
            REQUIRE(pop_bit(bb) == E1);
            REQUIRE(pop_bit(bb) == F2);
            REQUIRE(pop_bit(bb) == F4);
            REQUIRE(pop_bit(bb) == A5);
            REQUIRE(pop_bit(bb) == F6);
            REQUIRE(pop_bit(bb) == B7);
            REQUIRE(pop_bit(bb) == D7);
            REQUIRE(pop_bit(bb) == H7);
            REQUIRE(pop_bit(bb) == G8);

            REQUIRE(bb == 0);
        }
/*
        SECTION("popMSB") {
            U64 bb = c_u64({0, 0, 0, 0, 0, 0, 1, 0,
                            0, 1, 0, 1, 0, 0, 0, 1,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 1, 1, 1, 0, 0, 0});

            REQUIRE(pop_bit(BLACK, bb) == G8);
            REQUIRE(pop_bit(BLACK, bb) == H7);
            REQUIRE(pop_bit(BLACK, bb) == D7);
            REQUIRE(pop_bit(BLACK, bb) == B7);
            REQUIRE(pop_bit(BLACK, bb) == F6);
            REQUIRE(pop_bit(BLACK, bb) == A5);
            REQUIRE(pop_bit(BLACK, bb) == F4);
            REQUIRE(pop_bit(BLACK, bb) == F2);
            REQUIRE(pop_bit(BLACK, bb) == E1);
            REQUIRE(pop_bit(BLACK, bb) == D1);
            REQUIRE(pop_bit(BLACK, bb) == C1);

            REQUIRE(bb == 0);
        }
*/
        SECTION("Square index lookup") {
            REQUIRE(square_index(0, 0) == A1);
            REQUIRE(square_index(2, 2) == C3);
            REQUIRE(square_index(7, 7) == H8);
            REQUIRE(square_index(3, 2) == D3);
            REQUIRE(square_index(4, 6) == E7);
        }

        SECTION("String conversion") {
            SECTION("From string") {
                REQUIRE(to_sq('e', '4') == E4);
                REQUIRE(to_sq('f', '8') == F8);
                REQUIRE(to_sq('a', '1') == A1);
                REQUIRE(to_sq('g', '2') == G2);
                REQUIRE(to_sq('h', '8') == H8);

                REQUIRE_THROWS(to_sq('!', 'k'));
                REQUIRE_THROWS(to_sq('i', '4'));
                REQUIRE_THROWS(to_sq('b', '0'));
                REQUIRE_THROWS(to_sq('c', '9'));
            }

            SECTION("To string") {
                REQUIRE(from_sq(A4) == "a4");
                REQUIRE(from_sq(B8) == "b8");
                REQUIRE(from_sq(C5) == "c5");
                REQUIRE(from_sq(H8) == "h8");
                REQUIRE(from_sq(D4) == "d4");
            }
        }

        SECTION("Alignment") {
            REQUIRE(aligned(A1, B2));
            REQUIRE(aligned(A1, B1));
            REQUIRE(aligned(A1, A2));
            REQUIRE(!aligned(E4, B5));
            REQUIRE(aligned(B2, G7));
            REQUIRE(!aligned(A1, C2));

            REQUIRE(aligned(A1, B2, F6));
            REQUIRE(aligned(A1, D1, G1));
            REQUIRE(aligned(B2, B6, B4));
            REQUIRE(!aligned(E4, B5));
            REQUIRE(!aligned(B2, D5, G7));
            REQUIRE(!aligned(A1, C2));
        }

        SECTION("Population count") {
            REQUIRE(pop_count(0b0001100100100011000100100111000000000000000000010000000000000000) == 12);
            REQUIRE(pop_count(0b0000000000000000000000000000000000000000000000000000000000000000) == 0);
            REQUIRE(pop_count(0b1111111111111111111111111111111111111111111111111111111111111111) == 64);
            REQUIRE(pop_count(0b1000100111100011111100110111000000000110001000011111000000010010) == 27);
        }
    }

    SECTION("Bitboard move generation") {
        Square square;
        U64 expected;

        SECTION("Pawn tables") {
            square = A2;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              1, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(bb_normal_moves::pawn_moves_x1[WHITE][square] == expected);

            square = A7;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              1, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(bb_normal_moves::pawn_moves_x1[BLACK][square] == expected);
        }

        SECTION("King") {
            square = E5;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 1, 1, 0, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 0, 1, 1, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = A1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              1, 1, 0, 0, 0, 0, 0, 0,
                              0, 1, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = H8;
            expected = c_u64({0, 0, 0, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 0, 1, 1,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = D8;
            expected = c_u64({0, 0, 1, 0, 1, 0, 0, 0,
                              0, 0, 1, 1, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = E1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 1, 1, 0, 0,
                              0, 0, 0, 1, 0, 1, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = H3;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 1, 1,
                              0, 0, 0, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 0, 1, 1,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);

            square = A7;
            expected = c_u64({1, 1, 0, 0, 0, 0, 0, 0,
                              0, 1, 0, 0, 0, 0, 0, 0,
                              1, 1, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KING>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KING>(WHITE, square, 0) == expected);
        }

        SECTION("Knight") {
            square = E5;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 1, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 0, 0, 1, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = A1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 1, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = H8;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = G4;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 1,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 1,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = E1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 1, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = H3;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 1, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);

            square = A7;
            expected = c_u64({0, 0, 1, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 0,
                              0, 1, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<KNIGHT>(BLACK, square, 0) == expected);
            REQUIRE(find_moves<KNIGHT>(WHITE, square, 0) == expected);
        }

        SECTION("Pawn") {
            U64 occupied;

            SECTION("Normal moves") {
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0}); // C4

                REQUIRE(find_moves<PAWN>(BLACK, C5, 0) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, C3, 0) == expected);

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0}); // D5

                REQUIRE(find_moves<PAWN>(BLACK, D6, 0) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, D4, 0) == expected);

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  1, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0}); // C4

                REQUIRE(find_moves<PAWN>(WHITE, A5, 0) == expected);

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 1, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0}); // C4

                REQUIRE(find_moves<PAWN>(BLACK, G3, 0) == expected);
            }

            SECTION("Double moves") {
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 1, 0,
                                  0, 0, 0, 0, 0, 0, 1, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, G2, 0) != expected);
                REQUIRE(find_moves<PAWN>(WHITE, G2, 0) == expected);

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, C7, 0) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, C7, 0) != expected);
            }

            SECTION("Square in front occupied") {
                occupied = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  0, 1, 0, 0, 1, 0, 0, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(WHITE, B4, occupied) == expected);
                REQUIRE(find_moves<PAWN>(BLACK, B6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, D6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(BLACK, E6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, E4, occupied) == expected);
                REQUIRE(find_moves<PAWN>(BLACK, F7, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, F5, occupied) == expected);
                REQUIRE(find_moves<PAWN>(BLACK, F4, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, F2, occupied) == expected);
                REQUIRE(find_moves<PAWN>(BLACK, H6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, H4, occupied) == expected);

                // Blocking the second square of the double move
                occupied = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 1, 0, 0, 1, 0, 0, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 1, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, E7, occupied) == expected);

                // Blocking the second square of the double move
                occupied = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 1, 0, 0, 1, 0, 0, 1,
                                  0, 1, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 1, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(WHITE, B2, occupied) == expected);
            }

            SECTION("Normal moves with captures") {
                occupied = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  0, 1, 1, 0, 1, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 1, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 1, 1, 1, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, D6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, D4, occupied) == expected);

                occupied = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 1, 0,
                                  0, 0, 0, 1, 0, 0, 1, 0,
                                  0, 0, 0, 0, 1, 0, 1, 0,
                                  0, 0, 0, 0, 0, 0, 1, 0,
                                  0, 0, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 1, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, H6, occupied) == expected);
                REQUIRE(find_moves<PAWN>(WHITE, H4, occupied) == expected);
            }

            SECTION("Mixed situations") {
                occupied = c_u64({0, 0, 0, 0, 1, 0, 0, 0,
                                  0, 0, 1, 1, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 1, 0, 0,
                                  0, 1, 0, 0, 1, 0, 1, 1,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  1, 1, 1, 0, 0, 0, 1, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                square = B7;
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 1, 1, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, square, occupied) == expected);

                square = D7;
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 1, 1, 0, 0, 0, 0,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(BLACK, square, occupied) == expected);

                square = E3;
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 1, 1, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(WHITE, square, occupied) == expected);

                square = G4;
                expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0});

                REQUIRE(find_moves<PAWN>(WHITE, square, occupied) == expected);
            }
        }

        SECTION("Bishop") {
            U64 occupied = c_u64({0, 0, 0, 0, 1, 0, 0, 0,
                                  0, 0, 1, 1, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 1, 0, 0,
                                  0, 1, 0, 0, 1, 0, 1, 1,
                                  0, 0, 0, 1, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  1, 1, 1, 0, 0, 0, 1, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0});

            square = E4;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 1,
                              0, 0, 1, 0, 0, 0, 1, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 1, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<BISHOP>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<BISHOP>(BLACK, square, occupied) == expected);

            square = C1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 1, 0,
                              0, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              0, 1, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<BISHOP>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<BISHOP>(BLACK, square, occupied) == expected);

            square = D3;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 1,
                              0, 0, 0, 0, 0, 0, 1, 0,
                              0, 1, 0, 0, 0, 1, 0, 0,
                              0, 0, 1, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 0});

            REQUIRE(find_moves<BISHOP>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<BISHOP>(BLACK, square, occupied) == expected);

            square = D8;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 0, 1, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<BISHOP>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<BISHOP>(BLACK, square, occupied) == expected);
        }

        SECTION("Rook") {
            U64 occupied = c_u64({0, 0, 0, 0, 1, 0, 0, 0,
                                  0, 0, 1, 1, 0, 0, 0, 0,
                                  0, 0, 1, 0, 0, 1, 0, 0,
                                  0, 1, 0, 0, 1, 0, 0, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 1, 0, 0,
                                  1, 1, 1, 0, 0, 0, 1, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0});

            square = E4;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              1, 1, 1, 1, 0, 1, 1, 1,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 1, 0, 0, 0,
                              0, 0, 0, 0, 1, 0, 0, 0});

            REQUIRE(find_moves<ROOK>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<ROOK>(BLACK, square, occupied) == expected);

            square = C1;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 1, 0, 0, 0, 0, 0,
                              1, 1, 0, 1, 1, 1, 1, 1});

            REQUIRE(find_moves<ROOK>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<ROOK>(BLACK, square, occupied) == expected);

            square = D3;
            expected = c_u64({0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              1, 1, 1, 0, 1, 1, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0});

            REQUIRE(find_moves<ROOK>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<ROOK>(BLACK, square, occupied) == expected);

            square = D8;
            expected = c_u64({1, 1, 1, 0, 1, 0, 0, 0,
                              0, 0, 0, 1, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0});

            REQUIRE(find_moves<ROOK>(WHITE, square, occupied) == expected);
            REQUIRE(find_moves<ROOK>(BLACK, square, occupied) == expected);
        }
    }

    SECTION("Utility") {
        REQUIRE(E4 + rel_offset(WHITE, D_N) == E5);
        REQUIRE(E5 + rel_offset(BLACK, D_N) == E4);
        REQUIRE(E4 + rel_offset(WHITE, D_NE) == F5);
        REQUIRE(E4 + rel_offset(WHITE, D_NW) == D5);
        REQUIRE(E5 + rel_offset(BLACK, D_NE) == D4);
        REQUIRE(E5 + rel_offset(BLACK, D_NW) == F4);
    }
}