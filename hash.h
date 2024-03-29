//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_HASH_H
#define TOPPLE_HASH_H

#include <mutex>

#include "types.h"
#include "move.h"

/**
 * Random hash values used for Zobrist hashing.
 *
 * Each feature in a position is given a unique integer hash. The Zobrist hash of a position
 * is the XOR product of all of these features. This allows for fast incremental hash updates,
 * e.g. in move and unmove and avoids recomputing the hash.
 */
namespace zobrist {
    // SplitMix64 PRNG (public domain)
    struct split_mix_64_state { uint64_t x; };
    constexpr uint64_t split_mix_64(split_mix_64_state &state) {
        uint64_t z = (state.x += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }

    // Deterministically generates a random n-dimensional array of uint64_t at compile time
    // Uses variadic templates and recursion to generate the nested array type
    template<int Size, int... Rest>
    constexpr auto random_array(split_mix_64_state &state) {
        if constexpr (sizeof...(Rest) != 0) {
            std::array<decltype(random_array<Rest...>(state)), Size> arr = {};
            for (int i = 0; i < Size; ++i) {
                arr[i] = random_array<Rest...>(state);
            }
            return arr;
        } else {
            std::array<uint64_t, Size> arr = {};
            for (int i = 0; i < Size; ++i) {
                arr[i] = split_mix_64(state);
            }
            return arr;
        }
    }

    template<int... Dims>
    constexpr auto random_array(uint64_t seed) {
        split_mix_64_state state { seed };
        return random_array<Dims...>(state);
    }

    constexpr auto random_uint(uint64_t seed) {
        split_mix_64_state state { seed };
        return split_mix_64(state);
    }

    inline constexpr auto squares = random_array<64, 2, 6>(0);
    inline constexpr auto side = random_uint(1);
    inline constexpr auto ep = random_array<64>(2);
    inline constexpr auto castle = random_array<2, 2>(3);
}

/**
 * Holds incrementally updated material counts for a position, along with a Zobrist hash
 */
class material_data_t {
    uint8_t counts[2][6] = {};
    U64 z_hash = 0;
public:
    material_data_t() = default;
    constexpr material_data_t(
            int w_pawns, int w_knights, int w_bishops, int w_rooks, int w_queens,
            int b_pawns, int b_knights, int b_bishops, int b_rooks, int b_queens
    ) : counts{}, z_hash(0) {
        // Add kings
        inc(WHITE, KING);
        inc(BLACK, KING);

        // Add all other pieces
        while (w_pawns-- > 0) inc(WHITE, PAWN);
        while (w_knights-- > 0) inc(WHITE, KNIGHT);
        while (w_bishops-- > 0) inc(WHITE, BISHOP);
        while (w_rooks-- > 0) inc(WHITE, ROOK);
        while (w_queens-- > 0) inc(WHITE, QUEEN);
        while (b_pawns-- > 0) inc(BLACK, PAWN);
        while (b_knights-- > 0) inc(BLACK, KNIGHT);
        while (b_bishops-- > 0) inc(BLACK, BISHOP);
        while (b_rooks-- > 0) inc(BLACK, ROOK);
        while (b_queens-- > 0) inc(BLACK, QUEEN);
    }

    // Accessors
    [[nodiscard]] constexpr int count(Team team, Piece piece) const { return counts[team][piece]; }
    [[nodiscard]] constexpr U64 hash() const { return z_hash; }

    constexpr void inc(Team team, Piece piece) {
        // We reuse piece-square hashes for material hashing. The square used corresponds to the count
        // e.g. the first pawn has the hash of a pawn on square 0, the second on square 1, etc.
        z_hash ^= zobrist::squares[counts[team][piece]++][team][piece];
        assert(counts[team][piece] >= 0);
    }
    constexpr void dec(Team team, Piece piece) {
        // Note the prefix instead of postfix operator here.
        z_hash ^= zobrist::squares[--counts[team][piece]][team][piece];
        assert(counts[team][piece] >= 0);
    }
};

namespace tt {
    // Hash scores can be upper bounds, lower bounds, or exact
    enum Bound : uint8_t {
        NONE=0, UPPER, LOWER, EXACT
    };

    // Only use power of 2 sizes for efficiency (& rather than % for hashing)
    inline size_t lower_power_of_2(size_t size) {
        if (size & (size - 1)) { // Check if size is a power of 2
            for (unsigned int i = 1; i < 64; i++) {
                size |= size >> i; // Fill bits to the right
            }

            size += 1; // Add one (so there is only one bit set)
            size >>= 1; // Shift left one
        }

        return size;
    }

    /**
     * An entry in the hash table
     */
    struct entry_t { // 16 bytes
        U64 coded_hash; // 8 bytes, = hash ^ data. Used to prevent races between threads.
        union {
            struct {
                packed_move_t move;
                int16_t static_eval;
                int16_t internal_value;
                uint16_t about; // 6G 2B 8D
            } info;

            U64 data;
        };

        /**
         * Return the value of this hash entry, with checkmate scores adjusted for the given ply
         *
         * @param ply search ply to adjust checkmate scores
         * @return value of this entry
         */
        [[nodiscard]] int value(int ply) const {
            if (info.internal_value >= MINCHECKMATE) {
                return info.internal_value - ply;
            } else if (info.internal_value <= -MINCHECKMATE) {
                return info.internal_value + ply;
            } else {
                return info.internal_value;
            }
        }

        /**
         * Set the generation of this hash entry, recoding the hash value as necessary
         *
         * @param gen new generation
         */
        void refresh(unsigned gen) {
            U64 hash = coded_hash ^ data;
            info.about = uint16_t((info.about & 1023u) | (gen << 10u));
            coded_hash = hash ^ data;
        }

        // Other accessors
        [[nodiscard]] inline unsigned generation() const { return info.about >> 10u; }
        [[nodiscard]] inline Bound bound() const { return Bound((info.about >> 8) & 3u); }
        [[nodiscard]] inline int depth() const { return int(info.about & 255u); }
    };
    static_assert(sizeof(entry_t) == 16);

    class hash_t {
        static constexpr size_t bucket_size = 4;
    public:
        /**
         * Construct a new hash table with the given size in bytes
         *
         * @param size size of table
         */
        explicit hash_t(size_t size);
        ~hash_t();

        // Hash tables are huge, don't copy them
        hash_t(const hash_t &) = delete;

        /**
         * Prefetch an entry in the hash table
         *
         * @param hash hash of the entry to prefetch
         */
        inline void prefetch(U64 hash) {
            const size_t index = (hash & num_entries) * bucket_size;
            tt::entry_t *bucket = table + index;

            __builtin_prefetch(bucket);
        }

        /**
         * Probe the hash table with the given hash.
         * Returns true on hit, and saves the result to entry
         *
         * @param hash hash to probe
         * @param entry entry to store result
         * @return true on hit, false otherwise
         */
        bool probe(U64 hash, entry_t &entry);

        /**
         * Save an entry in the hash table. This method chooses an entry to replace and may not
         * write anything at all if this entry does not appear to be valuable.
         *
         * @param bound
         * @param hash
         * @param depth
         * @param ply
         * @param static_eval
         * @param score
         * @param move
         */
        void save(Bound bound, U64 hash, int depth, int ply, int static_eval, int score, move_t move);

        /**
         * Increase the generation of this hash table by 1, rendering existing entries out of date
         */
        void age();

        /**
         * Calculate how full the hash table is, based on a convenience sample of the first few elements
         *
         * @return hash table use, permill
         */
        size_t hash_full();
    private:
        size_t num_entries;
        unsigned generation = 1;
        entry_t *table;
    };
}

#endif //TOPPLE_HASH_H
