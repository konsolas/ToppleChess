//
// Created by Vincent on 23/05/2020.
//

#include <map>
#include <iomanip>

#include "toppletuner.h"
#include "ctpl_stl.h"

void topple_tuner_t::init_population() {
    population.emplace_back(0, eval_params_t());
    while (population.size() < params.size) {
        eval_params_t eval_params;

        std::normal_distribution<> amount(0, params.init_dev);
        int *array = reinterpret_cast<int *>(&eval_params);
        for (int i = 0; i < N_PARAMS; i++) {
            array[i] += std::lround(amount(rng));
        }

        population.emplace_back(0, eval_params);
    }
}

eval_params_t topple_tuner_t::evolve(size_t threads) {
    // Reset scores
    for (organism_t &o : population) {
        o.score = 0;
    }

    // Thread pool

    // Score
    std::vector<std::atomic_int> scores(population.size());
    size_t max_games = population.size() * (population.size() - 1);
    {
        std::atomic_int games = 0;
        ctpl::thread_pool pool(threads);
        for (size_t w = 0; w < population.size(); w++) {
            for (size_t b = 0; b < population.size(); b++) {
                if (w == b) continue;
                pool.push([this, &scores, &games, max_games, w, b] (int id) {
                    game_t game(processed_params_t(population[w].params), processed_params_t(population[b].params), limits);
                    for (const board_t &opening : openings) {
                        GameResult result = game.play(opening);
                        //std::cout << " " << ++games;
                        switch (result) {
                            case UNKNOWN:
                                std::cerr << "warn: unknown game result" << std::endl;
                                break;
                            case WIN:
                                //std::cout << "w" << std::endl;
                                scores[w] += 2;
                                break;
                            case DRAW:
                                //std::cout << "d" << std::endl;
                                scores[w] += 1;
                                scores[b] += 1;
                                break;
                            case LOSS:
                                //std::cout << "l" << std::endl;
                                scores[b] += 2;
                                break;
                        }
                    }
                });
            }
        }

        std::cout << max_games << " games submitted" << std::endl;
    }
    std::cout << std::endl;

    // Copy scores to population
    for (int i = 0; i < population.size(); i++) {
        population[i].score = scores[i].load();
    }

    // Sort
    std::sort(population.begin(), population.end(),
              [](const organism_t &a, const organism_t &b) { return a.score > b.score; });

    auto max_points = 2 * (population.size() * 2 - 2);
    std::cout << "scores: " << std::endl;
    for (auto &o : population) {
        std::cout << std::fixed << std::setprecision(2) << 100 * ((double) o.score / max_points) << "% ";
    }
    std::cout << std::endl;

    // Kill
    population.erase(population.end() - (size_t) (params.death * population.size()), population.end());

    // Reproduce
    size_t survived = population.size();
    std::cout << "survivors: " << survived << std::endl;
    std::uniform_int_distribution<size_t> survivor(0, survived - 1);
    while (population.size() < params.size) {
        population.emplace_back(0, cross(population[survivor(rng)], population[survivor(rng)]));
    }

    // Mutate (except the best)
    std::bernoulli_distribution should_mutate(params.mut_prob);
    for (size_t i = 1; i < population.size(); i++) {
        if (should_mutate(rng)) mutate(population[i].params);
    }

    return population[0].params;
}

void topple_tuner_t::mutate(eval_params_t &eval_params) {
    std::uniform_int_distribution<size_t> gene(0, N_PARAMS - 1);
    std::normal_distribution<> amount(0, params.mut_size);
    int *array = reinterpret_cast<int *>(&eval_params);
    array[gene(rng)] += std::lround(amount(rng));
}

eval_params_t topple_tuner_t::cross(const topple_tuner_t::organism_t &a, const topple_tuner_t::organism_t &b) {
    eval_params_t child;

    int *target = reinterpret_cast<int *>(&child);
    const int *source_a = reinterpret_cast<const int *>(&a.params);
    const int *source_b = reinterpret_cast<const int *>(&b.params);

    std::bernoulli_distribution choice;

    for (size_t i = 0; i < N_PARAMS; i++) {
        //target[i] = choice(rng) ? source_a[i] : source_b[i];
        target[i] = (source_a[i] + source_b[i]) / 2;
    }

    return child;
}
