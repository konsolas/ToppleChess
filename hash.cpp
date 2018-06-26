//
// Created by Vincent on 23/09/2017.
//

#include <random>
#include "hash.h"

namespace zobrist {
    const U64 seed = 0xBEEF;

    U64 squares[64][2][6];
    U64 side;
    U64 ep[64];
    U64 castle[2][2];

    void init_hashes() {
        std::mt19937_64 gen(seed);
        std::uniform_int_distribution<U64> dist;

        side = dist(gen);
        castle[0][0] = dist(gen);
        castle[0][1] = dist(gen);
        castle[1][0] = dist(gen);
        castle[1][1] = dist(gen);

        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 2; j++)
                for (int k = 0; k < 6; k++)
                    squares[i][j][k] = dist(gen);
            ep[i] = dist(gen);
        }
    }
}

tt::hash_t::hash_t(size_t size) {
    // Divide size by the sizeof an entry
    size /= sizeof(tt::entry_t);

    if (size & (size - 1)) { // Check if size is a power of 2
        for (unsigned int i = 1; i < 64; i++) {
            size |= size >> i; // Fill bits to the right
        }

        size += 1; // Add one (so there is only one bit set)
        size >>= 1; // Shift left one
    }

    if (size < sizeof(tt::entry_t)) {
        num_entries = 0;
    } else {
        num_entries = size - 1;
        table = new tt::entry_t[num_entries + BUCKET_SIZE]();
    }
}

tt::hash_t::~hash_t() {
    delete[] table;
}

bool tt::hash_t::probe(U64 hash, tt::entry_t &entry) {
    const size_t loc = hash & num_entries;

    if (table[loc].hash == hash) {
        entry = table[loc];

        return true;
    }
    return false;
}

void tt::hash_t::save(Bound bound, U64 hash, int depth, int ply, int eval, move_t move) {
    const size_t loc = hash & num_entries;

    if(eval >= MINCHECKMATE) eval += ply;
    if(eval <= -MINCHECKMATE) eval -= ply;

    table[loc].hash = hash;
    table[loc].bound = bound;
    table[loc].depth = static_cast<uint8_t>(depth);
    table[loc].internal_value = static_cast<int16_t>(eval);
    table[loc].move = move;
}

size_t tt::hash_t::hash_full() {
    size_t cnt = 0;
    for (size_t i = 0; i < num_entries; i++) {
        if(table[i].hash) {
            cnt++;
        }
    }

    return (cnt * 1000) / num_entries;
}
