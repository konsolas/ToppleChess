//
// Created by Vincent Tang on 2019-01-04.
//

#ifndef TOPPLE_TEXEL_H
#define TOPPLE_TEXEL_H

#include <vector>

#include "../board.h"
#include "../eval.h"

class texel_t {
    eval_params_t current_params;
    int scaling_constant = 130;

    unsigned int threads;
    size_t entries;
    std::vector<board_t> &positions;
    std::vector<double> &results;

    double current_error;
public:
    texel_t(unsigned int threads, size_t entries, std::vector<board_t> &positions, std::vector<double> &results);

    eval_params_t* get_current_params() { return &current_params; }
    void print_params();
    double get_current_error() { return current_error; }

    void optimise(int *parameter, size_t count, int max_iter);
    void random_optimise(int *parameter, size_t count, int max_iter);
    void anneal(int *parameter, size_t count, double base_temp, double hc_frac, int max_iter);
private:
    double momentum_optimise(int *parameter, double current_mea, int max_iter, int step);
    double sigmoid(double score);
    double mean_evaluation_error();
};


#endif //TOPPLE_TEXEL_H
