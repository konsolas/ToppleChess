//
// Created by Vincent Tang on 2019-01-04.
//

#include <iostream>
#include <sstream>
#include <fstream>

#include "../board.h"
#include "texel.h"
#include "../endgame.h"

#define TOPPLE_TUNE_VER "0.1.0"

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
    std::cout << "ToppleTexelTune v" << TOPPLE_TUNE_VER << " (c) Vincent Tang 2020" << std::endl;

    if(argc != 2) {
        std::cerr << "Usage: ToppleTexelTune <epd file>" << std::endl;
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

            if(pos != std::string::npos) {
                std::string fen = line.substr(0, pos);
                std::string result = line.substr(pos + 4, line.find(';') - pos - 4 - 1);

                boards.emplace_back(fen);
                results.push_back(get_result(result));
                continue;
            }

            pos = line.find("c1");
            if(pos != std::string::npos) {
                std::string fen = line.substr(0, pos);
                std::string result = line.substr(pos + 3, line.find(';') - pos - 3);
                if(result.find('*') != std::string::npos) {
                    continue;
                }

                boards.emplace_back(fen);
                results.push_back(get_result(result));
                continue;
            }

            if(!line.empty()) {
                std::cout << "warn: missing position: " << line << std::endl;
            }
        }
    } else {
        throw std::invalid_argument("file not found");
    }

    std::cout << "loaded " << boards.size() << " positions" << std::endl;

    texel_t tuner = texel_t(4, boards.size(), boards, results);

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
                std::string iterations;
                iss >> iterations;

                std::string max_iter;
                iss >> max_iter;
                int n_iter = std::stoi(max_iter);

                if(iterations == "once") {
                    tuner.optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int), n_iter);
                } else {
                    int times = std::stoi(iterations);
                    for(int i = 0; i < times; i++) {
                        tuner.optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int), n_iter);
                    }
                }

                tuner.print_params();
            } else if(cmd == "random_optimise") {
                std::string parameter;
                iss >> parameter;

                std::string max_iter;
                iss >> max_iter;
                int n_iter = std::stoi(max_iter);

                int times = std::stoi(parameter);
                for(int i = 0; i < times; i++) {
                    tuner.random_optimise(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int), n_iter);
                }

                tuner.print_params();
            } else if(cmd == "print") {
                tuner.print_params();
            } else if(cmd == "anneal") {
                int n_iter;
                iss >> n_iter;
                
                double temp;
                iss >> temp;
                
                double hc_frac;
                iss >> hc_frac;

                tuner.anneal(reinterpret_cast<int*> (tuner.get_current_params()), sizeof(eval_params_t) / sizeof(int), 
                        temp, hc_frac, n_iter);
            } else if(cmd == "threats") {
                std::string parameter;
                iss >> parameter;

                std::string max_iter;
                iss >> max_iter;
                int n_iter = std::stoi(max_iter);

                int times = std::stoi(parameter);
                for(int i = 0; i < times; i++) {
                    //tuner.random_optimise(reinterpret_cast<int*> (tuner.get_current_params()->undefended_mg), 50, n_iter);
                }

                tuner.print_params();
            }
        }
    }
}