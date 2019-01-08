//
// Created by Vincent Tang on 2019-01-04.
//

#include <iostream>
#include <sstream>
#include <fstream>

#include "../board.h"
#include "tuner.h"
#include "../endgame.h"

#define TOPPLE_TUNE_VER "0.0.1"

double get_result(const std::string &result) {
    if(result == "1-0") {
        return 1.0;
    } else if(result == "1/2-1/2") {
        return 0.5;
    } else if(result == "0-1") {
        return 0.0;
    } else {
        throw std::invalid_argument("Invalid result " + result);
    }
}

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    evaluator_t::eval_init();
    eg_init();

    // Startup
    std::cout << "ToppleTune v" << TOPPLE_TUNE_VER << " (c) Vincent Tang 2019" << std::endl;

    if(argc != 2) {
        std::cerr << "Usage: ToppleTune <epd file>" << std::endl;
        return 1;
    }

    // Load file from first argument
    std::vector<board_t> boards;
    std::vector<double> results;

    std::cout << "reading " << argv[1] << std::endl;

    std::ifstream file;

    if((file = std::ifstream(argv[1]))) {
        std::string line;
        while (std::getline(file, line)) {
            std::string::size_type pos = line.find("c9");

            std::string fen = line.substr(0, pos);
            std::string result = line.substr(pos + 4, line.find(';') - pos - 4 - 1);

            boards.emplace_back(fen);
            results.push_back(get_result(result));
        }
    } else {
        throw std::invalid_argument("file not found");
    }

    std::cout << "loaded " << boards.size() << " positions" << std::endl;

    tuner_t tuner = tuner_t(boards.size(), boards, results);

    while (true) {
        std::string input;
        std::getline(std::cin, input);

        std::istringstream iss(input);

        {
            std::string cmd;
            iss >> cmd;

            if(cmd == "quit") {
                break;
            } else if(cmd == "optimise") {
                std::string parameter;
                iss >> parameter;

                if(parameter == "once") {
                    tuner.optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int));
                } else {
                    int times = std::stoi(parameter);
                    for(int i = 0; i < times; i++) {
                        tuner.optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int));
                    }
                }
            } else if(cmd == "random_optimise") {
                std::string parameter;
                iss >> parameter;

                if(parameter == "once") {
                    tuner.random_optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int));
                } else {
                    int times = std::stoi(parameter);
                    for(int i = 0; i < times; i++) {
                        tuner.random_optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int));
                    }
                }
            } else if(cmd == "print") {
                tuner.print_params();
            } else if(cmd == "optimise_pos") {
                tuner.optimise(&tuner.get_current_params()->pos_bishop_pair_mg, 1);
                tuner.optimise(&tuner.get_current_params()->pos_bishop_pair_eg, 1);
                tuner.optimise(&tuner.get_current_params()->mat_mg[BISHOP], 1);
                tuner.optimise(&tuner.get_current_params()->mat_eg[BISHOP], 1);
            }
        }
    }
}