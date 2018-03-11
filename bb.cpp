//
// Created by Vincent on 22/09/2017.
//

#include <cassert>
#include <stdexcept>
#include <iostream>

#include "bb.h"

/**
 * =====================================================================================================================
 * MAGIC MOVE BIT BOARD GENERATION
 * =====================================================================================================================
 */
namespace bb_magics {
    /*
     * Magic bitboards by Pradyumna Kannan:
     *
     * Copyright (C) 2007 Pradyumna Kannan.
     *
     * This code is provided 'as-is', without any expressed or implied warranty.
     * In no event will the authors be held liable for any damages arising from
     * the use of this code. Permission is granted to anyone to use this
     * code for any purpose, including commercial applications, and to alter
     * it and redistribute it freely, subject to the following restrictions:
     *
     * 1. The origin of this code must not be misrepresented; you must not
     * claim that you wrote the original code. If you use this code in a
     * product, an acknowledgment in the product documentation would be
     * appreciated but is not required.
     *
     * 2. Altered source versions must be plainly marked as such, and must not be
     * misrepresented as being the original code.
     *
     * 3. This notice may not be removed or altered from any source distribution.
     */
    const unsigned int r_shift[64] =
            {
                    52, 53, 53, 53, 53, 53, 53, 52,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 54, 54, 54, 54, 53,
                    53, 54, 54, 53, 53, 53, 53, 53
            };

    const U64 r_magics[64] =
            {
                    (0x0080001020400080), (0x0040001000200040), (0x0080081000200080), (0x0080040800100080),
                    (0x0080020400080080), (0x0080010200040080), (0x0080008001000200), (0x0080002040800100),
                    (0x0000800020400080), (0x0000400020005000), (0x0000801000200080), (0x0000800800100080),
                    (0x0000800400080080), (0x0000800200040080), (0x0000800100020080), (0x0000800040800100),
                    (0x0000208000400080), (0x0000404000201000), (0x0000808010002000), (0x0000808008001000),
                    (0x0000808004000800), (0x0000808002000400), (0x0000010100020004), (0x0000020000408104),
                    (0x0000208080004000), (0x0000200040005000), (0x0000100080200080), (0x0000080080100080),
                    (0x0000040080080080), (0x0000020080040080), (0x0000010080800200), (0x0000800080004100),
                    (0x0000204000800080), (0x0000200040401000), (0x0000100080802000), (0x0000080080801000),
                    (0x0000040080800800), (0x0000020080800400), (0x0000020001010004), (0x0000800040800100),
                    (0x0000204000808000), (0x0000200040008080), (0x0000100020008080), (0x0000080010008080),
                    (0x0000040008008080), (0x0000020004008080), (0x0000010002008080), (0x0000004081020004),
                    (0x0000204000800080), (0x0000200040008080), (0x0000100020008080), (0x0000080010008080),
                    (0x0000040008008080), (0x0000020004008080), (0x0000800100020080), (0x0000800041000080),
                    (0x00FFFCDDFCED714A), (0x007FFCDDFCED714A), (0x003FFFCDFFD88096), (0x0000040810002101),
                    (0x0001000204080011), (0x0001000204000801), (0x0001000082000401), (0x0001FFFAABFAD1A2)
            };
    const U64 r_mask[64] =
            {
                    (0x000101010101017E), (0x000202020202027C), (0x000404040404047A), (0x0008080808080876),
                    (0x001010101010106E), (0x002020202020205E), (0x004040404040403E), (0x008080808080807E),
                    (0x0001010101017E00), (0x0002020202027C00), (0x0004040404047A00), (0x0008080808087600),
                    (0x0010101010106E00), (0x0020202020205E00), (0x0040404040403E00), (0x0080808080807E00),
                    (0x00010101017E0100), (0x00020202027C0200), (0x00040404047A0400), (0x0008080808760800),
                    (0x00101010106E1000), (0x00202020205E2000), (0x00404040403E4000), (0x00808080807E8000),
                    (0x000101017E010100), (0x000202027C020200), (0x000404047A040400), (0x0008080876080800),
                    (0x001010106E101000), (0x002020205E202000), (0x004040403E404000), (0x008080807E808000),
                    (0x0001017E01010100), (0x0002027C02020200), (0x0004047A04040400), (0x0008087608080800),
                    (0x0010106E10101000), (0x0020205E20202000), (0x0040403E40404000), (0x0080807E80808000),
                    (0x00017E0101010100), (0x00027C0202020200), (0x00047A0404040400), (0x0008760808080800),
                    (0x00106E1010101000), (0x00205E2020202000), (0x00403E4040404000), (0x00807E8080808000),
                    (0x007E010101010100), (0x007C020202020200), (0x007A040404040400), (0x0076080808080800),
                    (0x006E101010101000), (0x005E202020202000), (0x003E404040404000), (0x007E808080808000),
                    (0x7E01010101010100), (0x7C02020202020200), (0x7A04040404040400), (0x7608080808080800),
                    (0x6E10101010101000), (0x5E20202020202000), (0x3E40404040404000), (0x7E80808080808000)
            };

    const unsigned int b_shift[64] =
            {
                    58, 59, 59, 59, 59, 59, 59, 58,
                    59, 59, 59, 59, 59, 59, 59, 59,
                    59, 59, 57, 57, 57, 57, 59, 59,
                    59, 59, 57, 55, 55, 57, 59, 59,
                    59, 59, 57, 55, 55, 57, 59, 59,
                    59, 59, 57, 57, 57, 57, 59, 59,
                    59, 59, 59, 59, 59, 59, 59, 59,
                    58, 59, 59, 59, 59, 59, 59, 58
            };

    const U64 b_magics[64] =
            {
                    (0x0002020202020200), (0x0002020202020000), (0x0004010202000000), (0x0004040080000000),
                    (0x0001104000000000), (0x0000821040000000), (0x0000410410400000), (0x0000104104104000),
                    (0x0000040404040400), (0x0000020202020200), (0x0000040102020000), (0x0000040400800000),
                    (0x0000011040000000), (0x0000008210400000), (0x0000004104104000), (0x0000002082082000),
                    (0x0004000808080800), (0x0002000404040400), (0x0001000202020200), (0x0000800802004000),
                    (0x0000800400A00000), (0x0000200100884000), (0x0000400082082000), (0x0000200041041000),
                    (0x0002080010101000), (0x0001040008080800), (0x0000208004010400), (0x0000404004010200),
                    (0x0000840000802000), (0x0000404002011000), (0x0000808001041000), (0x0000404000820800),
                    (0x0001041000202000), (0x0000820800101000), (0x0000104400080800), (0x0000020080080080),
                    (0x0000404040040100), (0x0000808100020100), (0x0001010100020800), (0x0000808080010400),
                    (0x0000820820004000), (0x0000410410002000), (0x0000082088001000), (0x0000002011000800),
                    (0x0000080100400400), (0x0001010101000200), (0x0002020202000400), (0x0001010101000200),
                    (0x0000410410400000), (0x0000208208200000), (0x0000002084100000), (0x0000000020880000),
                    (0x0000001002020000), (0x0000040408020000), (0x0004040404040000), (0x0002020202020000),
                    (0x0000104104104000), (0x0000002082082000), (0x0000000020841000), (0x0000000000208800),
                    (0x0000000010020200), (0x0000000404080200), (0x0000040404040400), (0x0002020202020200)
            };


    const U64 b_mask[64] =
            {
                    (0x0040201008040200), (0x0000402010080400), (0x0000004020100A00), (0x0000000040221400),
                    (0x0000000002442800), (0x0000000204085000), (0x0000020408102000), (0x0002040810204000),
                    (0x0020100804020000), (0x0040201008040000), (0x00004020100A0000), (0x0000004022140000),
                    (0x0000000244280000), (0x0000020408500000), (0x0002040810200000), (0x0004081020400000),
                    (0x0010080402000200), (0x0020100804000400), (0x004020100A000A00), (0x0000402214001400),
                    (0x0000024428002800), (0x0002040850005000), (0x0004081020002000), (0x0008102040004000),
                    (0x0008040200020400), (0x0010080400040800), (0x0020100A000A1000), (0x0040221400142200),
                    (0x0002442800284400), (0x0004085000500800), (0x0008102000201000), (0x0010204000402000),
                    (0x0004020002040800), (0x0008040004081000), (0x00100A000A102000), (0x0022140014224000),
                    (0x0044280028440200), (0x0008500050080400), (0x0010200020100800), (0x0020400040201000),
                    (0x0002000204081000), (0x0004000408102000), (0x000A000A10204000), (0x0014001422400000),
                    (0x0028002844020000), (0x0050005008040200), (0x0020002010080400), (0x0040004020100800),
                    (0x0000020408102000), (0x0000040810204000), (0x00000A1020400000), (0x0000142240000000),
                    (0x0000284402000000), (0x0000500804020000), (0x0000201008040200), (0x0000402010080400),
                    (0x0002040810204000), (0x0004081020400000), (0x000A102040000000), (0x0014224000000000),
                    (0x0028440200000000), (0x0050080402000000), (0x0020100804020000), (0x0040201008040200)
            };

    U64 b_db[5248];
    const U64 *b_index[64] =
            {
                    b_db + 4992, b_db + 2624, b_db + 256, b_db + 896,
                    b_db + 1280, b_db + 1664, b_db + 4800, b_db + 5120,
                    b_db + 2560, b_db + 2656, b_db + 288, b_db + 928,
                    b_db + 1312, b_db + 1696, b_db + 4832, b_db + 4928,
                    b_db + 0, b_db + 128, b_db + 320, b_db + 960,
                    b_db + 1344, b_db + 1728, b_db + 2304, b_db + 2432,
                    b_db + 32, b_db + 160, b_db + 448, b_db + 2752,
                    b_db + 3776, b_db + 1856, b_db + 2336, b_db + 2464,
                    b_db + 64, b_db + 192, b_db + 576, b_db + 3264,
                    b_db + 4288, b_db + 1984, b_db + 2368, b_db + 2496,
                    b_db + 96, b_db + 224, b_db + 704, b_db + 1088,
                    b_db + 1472, b_db + 2112, b_db + 2400, b_db + 2528,
                    b_db + 2592, b_db + 2688, b_db + 832, b_db + 1216,
                    b_db + 1600, b_db + 2240, b_db + 4864, b_db + 4960,
                    b_db + 5056, b_db + 2720, b_db + 864, b_db + 1248,
                    b_db + 1632, b_db + 2272, b_db + 4896, b_db + 5184
            };


    U64 r_db[102400];
    const U64 *r_index[64] =
            {
                    r_db + 86016, r_db + 73728, r_db + 36864, r_db + 43008,
                    r_db + 47104, r_db + 51200, r_db + 77824, r_db + 94208,
                    r_db + 69632, r_db + 32768, r_db + 38912, r_db + 10240,
                    r_db + 14336, r_db + 53248, r_db + 57344, r_db + 81920,
                    r_db + 24576, r_db + 33792, r_db + 6144, r_db + 11264,
                    r_db + 15360, r_db + 18432, r_db + 58368, r_db + 61440,
                    r_db + 26624, r_db + 4096, r_db + 7168, r_db + 0,
                    r_db + 2048, r_db + 19456, r_db + 22528, r_db + 63488,
                    r_db + 28672, r_db + 5120, r_db + 8192, r_db + 1024,
                    r_db + 3072, r_db + 20480, r_db + 23552, r_db + 65536,
                    r_db + 30720, r_db + 34816, r_db + 9216, r_db + 12288,
                    r_db + 16384, r_db + 21504, r_db + 59392, r_db + 67584,
                    r_db + 71680, r_db + 35840, r_db + 39936, r_db + 13312,
                    r_db + 17408, r_db + 54272, r_db + 60416, r_db + 83968,
                    r_db + 90112, r_db + 75776, r_db + 40960, r_db + 45056,
                    r_db + 49152, r_db + 55296, r_db + 79872, r_db + 98304
            };


    U64 init_magic_occ(const int *squares, const int numSquares, const U64 linocc) {
        int i;
        U64 ret = 0;
        for (i = 0; i < numSquares; i++)
            if (linocc & (((U64) (1)) << i)) ret |= (((U64) (1)) << squares[i]);
        return ret;
    }

    U64 compute_rookmoves(const int square, const U64 occ) {
        U64 ret = 0;
        U64 bit;
        U64 rowbits = (((U64) 0xFF) << (8 * (square / 8)));

        bit = (((U64) (1)) << square);
        do {
            bit <<= 8;
            ret |= bit;
        } while (bit && !(bit & occ));
        bit = (((U64) (1)) << square);
        do {
            bit >>= 8;
            ret |= bit;
        } while (bit && !(bit & occ));
        bit = (((U64) (1)) << square);
        do {
            bit <<= 1;
            if (bit & rowbits) ret |= bit;
            else break;
        } while (!(bit & occ));
        bit = (((U64) (1)) << square);
        do {
            bit >>= 1;
            if (bit & rowbits) ret |= bit;
            else break;
        } while (!(bit & occ));
        return ret;
    }

    U64 compute_bishopmoves(const int square, const U64 occ) {
        U64 ret = 0;
        U64 bit;
        U64 bit2;
        U64 rowbits = (((U64) 0xFF) << (8 * (square / 8)));

        bit = (((U64) (1)) << square);
        bit2 = bit;
        do {
            bit <<= 8 - 1;
            bit2 >>= 1;
            if (bit2 & rowbits) ret |= bit;
            else break;
        } while (bit && !(bit & occ));
        bit = (((U64) (1)) << square);
        bit2 = bit;
        do {
            bit <<= 8 + 1;
            bit2 <<= 1;
            if (bit2 & rowbits) ret |= bit;
            else break;
        } while (bit && !(bit & occ));
        bit = (((U64) (1)) << square);
        bit2 = bit;
        do {
            bit >>= 8 - 1;
            bit2 <<= 1;
            if (bit2 & rowbits) ret |= bit;
            else break;
        } while (bit && !(bit & occ));
        bit = (((U64) (1)) << square);
        bit2 = bit;
        do {
            bit >>= 8 + 1;
            bit2 >>= 1;
            if (bit2 & rowbits) ret |= bit;
            else break;
        } while (bit && !(bit & occ));
        return ret;
    }

    void initmagicmoves() {
        int i;

        int bitscan_bitpos64[64] = {
                63, 0, 58, 1, 59, 47, 53, 2,
                60, 39, 48, 27, 54, 33, 42, 3,
                61, 51, 37, 40, 49, 18, 28, 20,
                55, 30, 34, 11, 43, 14, 22, 4,
                62, 57, 46, 52, 38, 26, 32, 41,
                50, 36, 17, 19, 29, 10, 13, 21,
                56, 45, 25, 31, 35, 16, 9, 12,
                44, 24, 15, 8, 23, 7, 6, 5};

        // indices without the const modifier
        U64 *mm_b_indices[64] =
                {
                        b_db + 4992, b_db + 2624, b_db + 256, b_db + 896,
                        b_db + 1280, b_db + 1664, b_db + 4800, b_db + 5120,
                        b_db + 2560, b_db + 2656, b_db + 288, b_db + 928,
                        b_db + 1312, b_db + 1696, b_db + 4832, b_db + 4928,
                        b_db + 0, b_db + 128, b_db + 320, b_db + 960,
                        b_db + 1344, b_db + 1728, b_db + 2304, b_db + 2432,
                        b_db + 32, b_db + 160, b_db + 448, b_db + 2752,
                        b_db + 3776, b_db + 1856, b_db + 2336, b_db + 2464,
                        b_db + 64, b_db + 192, b_db + 576, b_db + 3264,
                        b_db + 4288, b_db + 1984, b_db + 2368, b_db + 2496,
                        b_db + 96, b_db + 224, b_db + 704, b_db + 1088,
                        b_db + 1472, b_db + 2112, b_db + 2400, b_db + 2528,
                        b_db + 2592, b_db + 2688, b_db + 832, b_db + 1216,
                        b_db + 1600, b_db + 2240, b_db + 4864, b_db + 4960,
                        b_db + 5056, b_db + 2720, b_db + 864, b_db + 1248,
                        b_db + 1632, b_db + 2272, b_db + 4896, b_db + 5184
                };
        U64 *mm_r_indices[64] =
                {
                        r_db + 86016, r_db + 73728, r_db + 36864, r_db + 43008,
                        r_db + 47104, r_db + 51200, r_db + 77824, r_db + 94208,
                        r_db + 69632, r_db + 32768, r_db + 38912, r_db + 10240,
                        r_db + 14336, r_db + 53248, r_db + 57344, r_db + 81920,
                        r_db + 24576, r_db + 33792, r_db + 6144, r_db + 11264,
                        r_db + 15360, r_db + 18432, r_db + 58368, r_db + 61440,
                        r_db + 26624, r_db + 4096, r_db + 7168, r_db + 0,
                        r_db + 2048, r_db + 19456, r_db + 22528, r_db + 63488,
                        r_db + 28672, r_db + 5120, r_db + 8192, r_db + 1024,
                        r_db + 3072, r_db + 20480, r_db + 23552, r_db + 65536,
                        r_db + 30720, r_db + 34816, r_db + 9216, r_db + 12288,
                        r_db + 16384, r_db + 21504, r_db + 59392, r_db + 67584,
                        r_db + 71680, r_db + 35840, r_db + 39936, r_db + 13312,
                        r_db + 17408, r_db + 54272, r_db + 60416, r_db + 83968,
                        r_db + 90112, r_db + 75776, r_db + 40960, r_db + 45056,
                        r_db + 49152, r_db + 55296, r_db + 79872, r_db + 98304
                };

        for (i = 0; i < 64; i++) {
            int squares[64];
            int n_squares = 0;
            U64 temp = b_mask[i];
            while (temp) {
                U64 bit = temp & -temp;
                squares[n_squares++] = bitscan_bitpos64[(bit * (0x07EDD5E59A4E28C2)) >> 58];
                temp ^= bit;
            }
            for (temp = 0; temp < (((U64) (1)) << n_squares); temp++) {
                U64 tempocc = init_magic_occ(squares, n_squares, temp);
                *(mm_b_indices[i] + (((tempocc) * b_magics[i]) >> b_shift[i])) = compute_bishopmoves(i, tempocc);
            }
        }
        for (i = 0; i < 64; i++) {
            int squares[64];
            int n_squares = 0;
            U64 temp = r_mask[i];
            while (temp) {
                U64 bit = temp & -temp;
                squares[n_squares++] = bitscan_bitpos64[(bit * (0x07EDD5E59A4E28C2)) >> 58];
                temp ^= bit;
            }
            for (temp = 0; temp < (((U64) (1)) << n_squares); temp++) {
                U64 tempocc = init_magic_occ(squares, n_squares, temp);
                *(mm_r_indices[i] + (((tempocc) * r_magics[i]) >> r_shift[i])) = compute_rookmoves(i, tempocc);
            }
        }
    }

    inline U64 bishop_moves(const uint8_t &square, const U64 &occupancy) {
        return *(b_index[square] + (((occupancy & b_mask[square]) * b_magics[square])
                >> b_shift[square]));
    }

    inline U64 rook_moves(const uint8_t &square, const U64 &occupancy) {
        return *(r_index[square] + (((occupancy & r_mask[square]) * r_magics[square])
                >> r_shift[square]));
    }
}

/**
 * =====================================================================================================================
 * UTLITY BITBOARDS
 * =====================================================================================================================
 */
namespace bb_util {
    U64 single_bit[64];
    uint8_t sq_index[8][8];
    uint8_t file_index[64];
    uint8_t rank_index[64];

    U64 between[64][64];

    void init_util() {
        single_bit[0] = 0x1;
        for (int i = 1; i < 64; i++) {
            single_bit[i] = single_bit[i - 1] << 1;
        }

        for (uint8_t rank = 0; rank < 8; rank++) {
            for (uint8_t file = 0; file < 8; file++) {
                uint8_t square = (rank) * uint8_t(8) + file;
                sq_index[file][rank] = square;
                file_index[square] = file;
                rank_index[square] = rank;
            }
        }

        for (uint8_t a = 0; a < 64; a++) {
            for (uint8_t b = 0; b < 64; b++) {
                U64 occupied = single_bit[a] | single_bit[b];
                if (bb_magics::bishop_moves(a, 0) & single_bit[b]) {
                    between[a][b] = bb_magics::bishop_moves(a, occupied) &
                                    bb_magics::bishop_moves(b, occupied);
                } else if (bb_magics::rook_moves(a, 0) & single_bit[b]) {
                    between[a][b] = bb_magics::rook_moves(a, occupied) &
                                    bb_magics::rook_moves(b, occupied);
                }
            }
        }
    }
}

/**
 * =====================================================================================================================
 * GENERATION FOR NON-SLIDING PIECES
 * =====================================================================================================================
 */
namespace bb_normal_moves {
    U64 king_moves[64];
    U64 knight_moves[64];
    U64 pawn_moves_x1[2][64];
    U64 pawn_moves_x2[2][64];
    U64 pawn_caps[2][64];

    inline bool valid_square(const int &file, const int &rank) {
        return file >= 0 && file < 8 && rank >= 0 && rank < 8;
    }

    inline void update_array(U64 *arr, const uint8_t &file, const uint8_t &rank,
                             int file_offset, int rank_offset) {
        if (valid_square(file + file_offset, rank + rank_offset)) {
            arr[bb_util::sq_index[file][rank]] |= bb_util::single_bit[bb_util::sq_index[file + file_offset][rank + rank_offset]];
        }
    }

    void init_normal_moves() {
        for (uint8_t file_from = 0; file_from < 8; file_from++) {
            for (uint8_t rank_from = 0; rank_from < 8; rank_from++) {
                // King moves
                update_array(king_moves, file_from, rank_from, -1, -1);
                update_array(king_moves, file_from, rank_from, -1, 0);
                update_array(king_moves, file_from, rank_from, -1, 1);
                update_array(king_moves, file_from, rank_from, 0, -1);
                update_array(king_moves, file_from, rank_from, 0, 1);
                update_array(king_moves, file_from, rank_from, 1, -1);
                update_array(king_moves, file_from, rank_from, 1, 0);
                update_array(king_moves, file_from, rank_from, 1, 1);

                // Knight moves
                update_array(knight_moves, file_from, rank_from, -2, -1);
                update_array(knight_moves, file_from, rank_from, -2, 1);
                update_array(knight_moves, file_from, rank_from, -1, -2);
                update_array(knight_moves, file_from, rank_from, -1, 2);
                update_array(knight_moves, file_from, rank_from, 1, -2);
                update_array(knight_moves, file_from, rank_from, 1, 2);
                update_array(knight_moves, file_from, rank_from, 2, -1);
                update_array(knight_moves, file_from, rank_from, 2, 1);

                // Pawn moves
                update_array(pawn_moves_x1[WHITE], file_from, rank_from, 0, 1);
                update_array(pawn_moves_x1[BLACK], file_from, rank_from, 0, -1);

                if (rank_from == 1) {
                    update_array(pawn_moves_x2[WHITE], file_from, rank_from, 0, 2);
                } else if (rank_from == 6) {
                    update_array(pawn_moves_x2[BLACK], file_from, rank_from, 0, -2);
                }

                // Pawn captures
                update_array(pawn_caps[WHITE], file_from, rank_from, 1, 1);
                update_array(pawn_caps[WHITE], file_from, rank_from, -1, 1);
                update_array(pawn_caps[BLACK], file_from, rank_from, 1, -1);
                update_array(pawn_caps[BLACK], file_from, rank_from, -1, -1);
            }
        }
    }
}

/**
 * =====================================================================================================================
 * INTRINSICS
 * =====================================================================================================================
 */
namespace bb_intrin {
    inline uint8_t lsb(U64 b) {
        assert(b);
#if defined(__GNUC__)
        return Square(__builtin_ctzll(b));
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        unsigned long idx;
        _BitScanForward64(&idx, b);
        return Square(idx);
#else
#error "No intrinsic lsb/bitscanforward/ctz available"
#endif
    }

    inline uint8_t msb(U64 b) {
        assert(b);
#if defined(__GNUC__)
        return Square(63 - __builtin_clzll(b));
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        unsigned long idx;
        _BitScanReverse64(&idx, b);
        return Square(idx);
#else
#error "No intrinsic msb/bitscanreverse/clz available"
#endif
    }

    inline int pop_count(const U64 &b) {
#if defined(__GNUC__)
        return __builtin_popcountll(b);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
        return (int) _mm_popcnt_u64(b);
#else
#error "No intrinsic popcnt available"
#endif
    }
}

/**
 * =====================================================================================================================
 * HEADER IMPLEMENTATION
 * =====================================================================================================================
 */
void init_tables() {
    bb_magics::initmagicmoves();
    bb_util::init_util();
    bb_normal_moves::init_normal_moves();
}

U64 find_moves(Piece type, Team side, uint8_t square, U64 occupied) {
    switch (type) {
        case PAWN: {
            U64 result = 0;
            result |= occupied & bb_normal_moves::pawn_caps[side][square];
            if (!(occupied & bb_normal_moves::pawn_moves_x1[side][square])) {
                result |= bb_normal_moves::pawn_moves_x1[side][square];
                if (!(occupied & bb_normal_moves::pawn_moves_x2[side][square])) {
                    result |= bb_normal_moves::pawn_moves_x2[side][square];
                }
            }
            return result;
        }
        case KNIGHT:
            return bb_normal_moves::knight_moves[square];
        case BISHOP:
            return bb_magics::bishop_moves(square, occupied);
        case ROOK:
            return bb_magics::rook_moves(square, occupied);
        case QUEEN:
            return bb_magics::bishop_moves(square, occupied) | bb_magics::rook_moves(square, occupied);
        case KING:
            return bb_normal_moves::king_moves[square];
    }

    return 0; // Can't happen
}

U64 single_bit(uint8_t square) {
    return bb_util::single_bit[square];
}

U64 bits_between(uint8_t a, uint8_t b) {
    return bb_util::between[a][b];
}

uint8_t square_index(uint8_t file, uint8_t rank) {
    return bb_util::sq_index[file][rank];
}

uint8_t popBit(Team team, U64 &bb) {
    if(team) {
        const uint8_t s = bb_intrin::msb(bb);
        bb &= ~single_bit(s);
        return s;
    } else {
        const uint8_t s = bb_intrin::lsb(bb);
        bb &= bb - 1;
        return s;
    }
}

int pop_count(U64 bb) {
    return bb_intrin::pop_count(bb);
}

uint8_t to_sq(char file, char rank) {
    if (file >= 'a' && file <= 'h' && rank >= '1' && rank <= '8') {
        return square_index(uint8_t(file - 'a'), uint8_t(rank - '1'));
    } else {
        throw std::runtime_error(std::string("invalid square descriptor: '") + file + rank + "'");
    }
}

std::string from_sq(uint8_t sq) {
    switch (Square(sq)) {
        case A1: return std::string("a1");
        case B1: return std::string("b1");
        case C1: return std::string("c1");
        case D1: return std::string("d1");
        case E1: return std::string("e1");
        case F1: return std::string("f1");
        case G1: return std::string("g1");
        case H1: return std::string("h1");
        case A2: return std::string("a2");
        case B2: return std::string("b2");
        case C2: return std::string("c2");
        case D2: return std::string("d2");
        case E2: return std::string("e2");
        case F2: return std::string("f2");
        case G2: return std::string("g2");
        case H2: return std::string("h2");
        case A3: return std::string("a3");
        case B3: return std::string("b3");
        case C3: return std::string("c3");
        case D3: return std::string("d3");
        case E3: return std::string("e3");
        case F3: return std::string("f3");
        case G3: return std::string("g3");
        case H3: return std::string("h3");
        case A4: return std::string("a4");
        case B4: return std::string("b4");
        case C4: return std::string("c4");
        case D4: return std::string("d4");
        case E4: return std::string("e4");
        case F4: return std::string("f4");
        case G4: return std::string("g4");
        case H4: return std::string("h4");
        case A5: return std::string("a5");
        case B5: return std::string("b5");
        case C5: return std::string("c5");
        case D5: return std::string("d5");
        case E5: return std::string("e5");
        case F5: return std::string("f5");
        case G5: return std::string("g5");
        case H5: return std::string("h5");
        case A6: return std::string("a6");
        case B6: return std::string("b6");
        case C6: return std::string("c6");
        case D6: return std::string("d6");
        case E6: return std::string("e6");
        case F6: return std::string("f6");
        case G6: return std::string("g6");
        case H6: return std::string("h6");
        case A7: return std::string("a7");
        case B7: return std::string("b7");
        case C7: return std::string("c7");
        case D7: return std::string("d7");
        case E7: return std::string("e7");
        case F7: return std::string("f7");
        case G7: return std::string("g7");
        case H7: return std::string("h7");
        case A8: return std::string("a8");
        case B8: return std::string("b8");
        case C8: return std::string("c8");
        case D8: return std::string("d8");
        case E8: return std::string("e8");
        case F8: return std::string("f8");
        case G8: return std::string("g8");
        case H8: return std::string("h8");
    }

    return std::string(); // Can't happen
}

uint8_t rank_index(uint8_t sq_index) {
    return bb_util::rank_index[sq_index];
}

uint8_t file_index(uint8_t sq_index) {
    return bb_util::file_index[sq_index];
}
