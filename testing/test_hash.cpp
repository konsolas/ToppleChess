//
// Created by Vincent on 23/06/2019.
//

#include <catch2/catch.hpp>
#include <random>

#include "util.h"
#include "../board.h"

TEST_CASE("Hash entry") {
    std::mt19937 gen(0);
    std::uniform_int_distribution<U64> dist;
    
    U64 hash = dist(gen);
    tt::entry_t entry;
    entry.data = dist(gen);
    entry.coded_hash = hash ^ entry.data;
    
    packed_move_t move = entry.info.move;
    int16_t internal_value = entry.info.internal_value;
    int16_t static_eval = entry.info.static_eval;
    int depth = entry.depth();
    tt::Bound bound = entry.bound();
    
    entry.refresh(0);
    
    REQUIRE((entry.coded_hash ^ entry.data) == hash);
    REQUIRE(entry.generation() == 0);
    REQUIRE(entry.info.move.from == move.from);
    REQUIRE(entry.info.move.to == move.to);
    REQUIRE(entry.info.move.type == move.type);
    REQUIRE(entry.info.internal_value == internal_value);
    REQUIRE(entry.info.static_eval == static_eval);
    REQUIRE(entry.depth() == depth);
    REQUIRE(entry.bound() == bound);
}

TEST_CASE("Material hashes") {
    material_data_t data = {};

    REQUIRE(data.hash() == 0);

    data.inc(WHITE, PAWN);

    REQUIRE(data.count(WHITE, PAWN) == 1);
    REQUIRE(data.hash() == zobrist::squares[0][WHITE][PAWN]);

    data.dec(WHITE, PAWN);

    REQUIRE(data.hash() == 0);
}