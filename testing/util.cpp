//
// Created by Vincent on 27/09/2017.
//

#include <array>
#include <sstream>
#include <iostream>

#include "util.h"

std::string u64_to_string(U64 v) {
    std::cout << std::endl << v << std::endl;

    char boardc[64];

    for (int i = 0; i < 64; i++) {
        if (v & single_bit(uint8_t(i))) boardc[i] = '1';
        else boardc[i] = '.';
    }

    std::ostringstream os;

    os << " " << std::endl;

    for (int rank = 7; rank >= 0; rank--) {
        os << " " << rank << " ";
        for (int file = 0; file <= 7; file++) {
            os << boardc[square_index(uint8_t(file), uint8_t(rank))];
        }
        os << std::endl;
    }

    os << std::endl << "   abcdefgh" << std::endl;
    os << std::endl;

    return os.str();
}

U64 c_u64(std::array<int, 64> array) {
    U64 num = 0;
    int cnt = 0;
    for(int i = 7; i >= 0; i--) {
        for (int j = 0; j < 8; j++) {
            num |= U64(array[cnt++]) << (i * 8 + j);
        }
    }

    return num;
}

