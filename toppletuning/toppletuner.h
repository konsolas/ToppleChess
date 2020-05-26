//
// Created by Vincent on 23/05/2020.
//

#ifndef TOPPLE_TOPPLETUNER_H
#define TOPPLE_TOPPLETUNER_H

#include <utility>
#include <vector>
#include <random>

#include "game.h"

constexpr size_t N_PARAMS = sizeof(eval_params_t) / sizeof(int);

struct tuner_params_t {
    size_t size;
    double death;
    double mut_prob, mut_size;
    double init_dev;
};

class topple_tuner_t {
    struct organism_t {
        organism_t(const int &score, const eval_params_t &params) : score(score), params(params) {}

        int score = 0;
        eval_params_t params;
    };

    std::mt19937_64 rng;

    tuner_params_t params;
    search_limits_t limits;
    std::vector<board_t> openings;

    std::vector<organism_t> population;
public:
    explicit topple_tuner_t(tuner_params_t params, search_limits_t limits, std::vector<board_t> openings)
        : params(params), limits(std::move(limits)), openings(std::move(openings)) {

    }
    void init_population();
    eval_params_t evolve(size_t threads);
private:
    void mutate(eval_params_t &eval_params);
    eval_params_t cross(const organism_t &a, const organism_t &b);
};


#endif //TOPPLE_TOPPLETUNER_H
