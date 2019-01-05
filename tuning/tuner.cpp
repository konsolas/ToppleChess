//
// Created by Vincent Tang on 2019-01-04.
//

#include <utility>
#include <cmath>

#include "tuner.h"
#include "../movegen.h"
#include "../movesort.h"

tuner_t::tuner_t(size_t entries, std::vector<board_t> positions, std::vector<double> results)
    : entries(entries), positions(std::move(positions)), results(std::move(results)) {
    if(positions.size() != entries || results.size() != entries) throw std::invalid_argument("invalid entry count");

    // Pick scaling_constant
    momentum_optimise(&scaling_constant, mean_evaluation_error());
}

int tuner_t::quiesce(board_t &board, int alpha, int beta, evaluator_t &evaluator) {
    int stand_pat = evaluator.evaluate(board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    movegen_t gen(board); move_t next{};
    move_t buf[128] = {}; gen.gen_caps(buf);
    int idx = 0;

    while ((next = buf[idx++]) != EMPTY_MOVE) {
        if (next.info.captured_type == KING) return INF; // Ignore this position in case of a king capture
        if(board.see(next) < 0) {
            continue;
        }

        board.move(next);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            int score = -quiesce(board, -beta, -alpha, evaluator);
            board.unmove();

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
    }

    return alpha;
}

double tuner_t::find_scaling_constant() {
    return 0;
}

double tuner_t::sigmoid(double score) {
    return 1 / (1 + exp(-score * scaling_constant));
}

double tuner_t::mean_evaluation_error() {
    evaluator_t evaluator(current_params, 0);

    double total_squared_error = 0;
    for(size_t i = 0; i < entries; i++) {
        double eval = sigmoid(evaluator.evaluate(positions[i]));
        double error = eval - results[i];
        total_squared_error += error * error;
    }

    return total_squared_error / entries;
}

double tuner_t::momentum_optimise(int *parameter, double current_mea) {
    int original = *parameter;

    // Determine direction
    double adjusted_mea;
    *parameter = original + 1;
    if((adjusted_mea = mean_evaluation_error()) < current_mea) {
        while(adjusted_mea < current_mea) {
            current_mea = adjusted_mea;
            *parameter += 1;
            adjusted_mea = mean_evaluation_error();
        }

        *parameter -= 1;
    } else {
        *parameter = original - 1;
        adjusted_mea = mean_evaluation_error();
        while(adjusted_mea < current_mea) {
            current_mea = adjusted_mea;
            *parameter -= 1;
            adjusted_mea = mean_evaluation_error();
        }

        *parameter += 1;
    }

    return current_mea;
}
