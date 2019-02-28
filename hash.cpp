//
// Created by Vincent on 23/09/2017.
//

#include <random>
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
    tt::entry_t *bucket = table + index;

    if(bucket->hash == hash) {
        bucket->refresh(generation);
        entry = *bucket;
        return true;
    }

    for(size_t i = 1; i < bucket_size; i++) {
        if((bucket + i)->hash == hash) {
            (bucket + i)->refresh(generation);
            entry = *(bucket + i);
            return true;
        }
    }
    return false;
}

void tt::hash_t::prefetch(U64 hash) {
    const size_t index = (hash & num_entries) * bucket_size;
    tt::entry_t *bucket = table + index;

#if defined(__GNUC__)
    __builtin_prefetch(bucket);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
    _mm_prefetch((char*) bucket, _MM_HINT_T0);
#endif
}

void tt::hash_t::save(Bound bound, U64 hash, int depth, int ply, int static_eval, int score, move_t move) {
    const size_t index = (hash & num_entries) * bucket_size;
    tt::entry_t *bucket = table + index;

    if (score >= MINCHECKMATE) score += ply;
    if (score <= -MINCHECKMATE) score -= ply;

    if(bucket->hash == hash) {
        //if(bound == EXACT || depth >= bucket->depth() - 2) {
            bucket->update(bound, depth, generation, move, static_eval, score);
        //}
        return;
    }

    tt::entry_t *replace = bucket;
    for(size_t i = 1; i < bucket_size; i++) {
        // Always replace if same position
        if((bucket + i)->hash == hash) {
            //if(bound == EXACT || depth >= bucket->depth() - 2) {
                (bucket + i)->update(bound, depth, generation, move, static_eval, score);
            //}
            return;
        } else if((bucket + i)->about < replace->about) {
            replace = (bucket + i);
        }
    }

    // Replace best candidate
    replace->hash = hash;
    replace->update(bound, depth, generation, move, static_eval, score);
}

void tt::hash_t::age() {
    generation++;
    if(generation >= 127) {
        generation = 1;

        // Clear up hash
        for (size_t i = 0; i < num_entries * bucket_size; i++) {
            table[i].refresh(0);
        }
    }
}

size_t tt::hash_t::hash_full() {
    size_t cnt = 0;
    for (size_t i = 0; i < num_entries * bucket_size; i++) {
        if (table[i].generation() == generation) {
            cnt++;
        }
    }

    return (cnt * 1000) / (num_entries * bucket_size);
}
