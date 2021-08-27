//
// Created by Vincent on 27/09/2017.
//

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "../bb.h"
#include "../hash.h"
#include "../eval.h"

int main(int argc, char* argv[]) {
    init_tables();
    evaluator_t::eval_init();

    return Catch::Session().run(argc, argv);
}