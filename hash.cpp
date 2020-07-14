//
// Created by Vincent on 23/09/2017.
//

#include <random>
#include <memory>
#include <cstring>
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
    size /= (sizeof(tt::entry_t) * bucket_size);
    size = lower_power_of_2(size);
    if (size < sizeof(tt::entry_t)) {
        num_entries = 0;
    } else {
        num_entries = size - 1;
        table = new tt::entry_t[num_entries * bucket_size + bucket_size]();
    }
}

tt::hash_t::~hash_t() {
    delete[] table;
}

bool tt::hash_t::probe(U64 hash, tt::entry_t &entry) {
    const size_t index = (hash & num_entries) * bucket_size;
    uint16_t hash16 = hash >> 48u;
    tt::entry_t *bucket = table + index;

    for (size_t i = 0; i < bucket_size; i++) {
        if (bucket[i].hash16() == hash16) {
            entry = bucket[i];
            return true;
        }
    }

    return false;
}

void tt::hash_t::save(Bound bound, U64 hash, int depth, int ply, int score, move_t move) {
    const size_t index = (hash & num_entries) * bucket_size;
    uint16_t hash16 = hash >> 48u;
    tt::entry_t *bucket = table + index;

    tt::entry_t updated = {bound, hash, depth, ply, score, move, generation};

    tt::entry_t *replace = bucket;
    for (size_t i = 0; i < bucket_size; i++) {
        if (bucket[i].hash16() == hash16) {
            if (bound == EXACT || depth >= bucket[i].depth() - 2) bucket[i] = updated;
            return;
        } else if (bucket[i].utility() < replace->utility()) {
            replace = bucket + i;
        }
    }

    *replace = updated;
}

void tt::hash_t::age() {
    generation++;
    if(generation >= 63) {
        generation = 1;

        // Clear up hash
        for (size_t i = 0; i < num_entries * bucket_size; i++) {
            table[i].reset_gen();
        }
    }
}

size_t tt::hash_t::hash_full() {
    constexpr size_t sample_size = 4000;
    constexpr size_t divisor = sample_size / 1000;

    assert(num_entries * bucket_size > sample_size);

    size_t cnt = 0;
    for (size_t i = 0; i < sample_size; i++) {
        if (table[i].generation() == generation) {
            cnt++;
        }
    }

    return cnt / divisor;
}
