//
// Created by Vincent on 23/09/2017.
//

#include <random>
#include <memory>
#include <cstring>
#include "hash.h"

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

    if((bucket->coded_hash ^ bucket->data) == hash) {
        bucket->refresh(generation);
        entry = *bucket;
        return true;
    }

    for(size_t i = 1; i < bucket_size; i++) {
        if(((bucket + i)->coded_hash ^ (bucket + i)->data) == hash) {
            (bucket + i)->refresh(generation);
            entry = *(bucket + i);
            return true;
        }
    }
    return false;
}

void tt::hash_t::save(Bound bound, U64 hash, int depth, int ply, int static_eval, int score, move_t move) {
    const size_t index = (hash & num_entries) * bucket_size;
    tt::entry_t *bucket = table + index;

    if (score >= MINCHECKMATE) score += ply;
    if (score <= -MINCHECKMATE) score -= ply;

    tt::entry_t updated = {};
    updated.info.move = compress(move);
    updated.info.static_eval = static_cast<int16_t>(static_eval);
    updated.info.internal_value = static_cast<int16_t>(score);
    updated.info.about = (generation << 10u) | (uint16_t(bound) << 8u) | uint16_t(depth);
    updated.coded_hash = hash ^ updated.data;

    if((bucket->coded_hash ^ bucket->data) == hash) {
        if(bound == EXACT || depth >= bucket->depth() - 2) {
            *bucket = updated;
        }
        return;
    }

    tt::entry_t *replace = bucket;
    for(size_t i = 1; i < bucket_size; i++) {
        if(((bucket + i)->coded_hash ^ (bucket + i)->data) == hash) {
            if(bound == EXACT || depth >= bucket->depth() - 2) {
                *(bucket + i) = updated;
            }
            return;
        } else if((bucket + i)->info.about < replace->info.about) {
            replace = (bucket + i);
        }
    }

    // Replace best candidate
    *replace = updated;
}

void tt::hash_t::age() {
    generation++;
    if(generation >= 63) {
        generation = 1;

        // Clear up hash
        for (size_t i = 0; i < num_entries * bucket_size; i++) {
            table[i].refresh(0);
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
