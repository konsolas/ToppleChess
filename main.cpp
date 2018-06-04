#include <iostream>
#include <iomanip>
#include <future>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"

U64 perft(board_t &, int);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    eval_init();

    // Board
    board_t *board = nullptr;

    // Hash
    tt::hash_t *tt = new tt::hash_t(1073741824);

    // Search
    search_t *search = nullptr;
    std::atomic_bool search_abort;
    std::future<void> future;

    while (true) {
        std::string input;
        std::getline(std::cin, input);
        std::istringstream iss(input);

        {
            std::string cmd;
            iss >> cmd;

            if (cmd == "uci") {
                std::cout << "id name Topple" << std::endl;
                std::cout << "id author Vincent Tang" << std::endl;
                std::cout << "uciok" << std::endl;
            } else if (cmd == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (cmd == "stop") {
                search_abort = true;
            } else if (cmd == "position") {
                std::string type;
                while (iss >> type) {
                    if (type == "startpos") {
                        delete board;
                        board = new board_t("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    } else if (type == "fen") {
                        std::string fen;
                        for (int i = 0; i < 6; i++) {
                            std::string component;
                            iss >> component;
                            fen += component + " ";
                        }

                        delete board;
                        board = new board_t(fen);
                    } else if (type == "moves") {
                        // Read moves
                        std::string move;
                        while (iss >> move) {
                            board->move(board->parse_move(move));
                        }
                    }
                }
            } else if (cmd == "go") {
                if (board != nullptr) {
                    // Setup
                    delete search;
                    search_abort = false;
                    search = new search_t(*board, tt);

                    // Parse parameters
                    int max_time = 500;
                    int max_depth = MAX_PLY;

                    std::string param;
                    while (iss >> param) {
                        if(param == "infinite") {
                            max_time = INT_MAX;
                            max_depth = MAX_PLY;
                        } else if(param == "depth") {
                            iss >> max_depth;
                        } else if(param == "movetime") {
                            iss >> max_time;
                        } else if(param == "wtime") {
                            if(board->record[board->now].next_move == WHITE) {
                                int wtime; iss >> wtime;
                                max_time -= 10;
                                max_time += wtime / std::max(50 - board->now, 30);
                                if(max_time < 10) max_time = 10;
                            }
                        } else if(param == "btime") {
                            if(board->record[board->now].next_move == BLACK) {
                                int btime; iss >> btime;
                                max_time -= 10;
                                max_time += btime / std::max(50 - board->now, 30);
                                if(max_time < 10) max_time = 10;
                            }
                        } else if(param == "winc") {
                            if(board->record[board->now].next_move == WHITE) {
                                int winc; iss >> winc;
                                max_time += winc;
                            }
                        } else if(param == "binc") {
                            if(board->record[board->now].next_move == BLACK) {
                                int binc; iss >> binc;
                                max_time += binc;
                            }
                        }
                    }

                    // Start search
                    future = std::async(std::launch::async,
                               [search, &search_abort, max_time, max_depth] {
                                    std::future<move_t> bm = std::async(std::launch::async, &search_t::think,
                                        search, 1, max_depth, std::ref(search_abort));

                                    move_t best_move;
                                    if(bm.wait_for(std::chrono::milliseconds(max_time)) == std::future_status::ready) {
                                        best_move = bm.get();
                                    } else {
                                        search_abort = true;
                                        best_move = bm.get();
                                    }

                                    std::cout << "bestmove " << best_move << std::endl;
                               }
                    );
                }
            } else if (cmd == "print") {
                if (board == nullptr) {
                    std::cout << "nullptr" << std::endl;
                } else {
                    std::cout << *board << std::endl;
                }
            } else if (cmd == "quit" || cmd == "exit") {
                std::cout << "exit" << std::endl;
                break;
            }
        }
    }

    return 0;
}
