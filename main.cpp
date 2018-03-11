#include <iostream>
#include <iomanip>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"

U64 perft(board_t&, int);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    eval_init();

    // Variables
    board_t *board = nullptr;
    search_t *search = nullptr;
    tt::hash_t *tt = new tt::hash_t(1073741824);

    while (true) {
        std::string input;
        std::getline(std::cin, input);
        std::istringstream iss(input);

        {
            std::string cmd;
            iss >> cmd;

            if(cmd == "uci") {
                std::cout << "id name Topple" << std::endl;
                std::cout << "id author Vincent Tang" << std::endl;
                std::cout << "uciok" << std::endl;
            } else if(cmd == "isready") {
                std::cout << "readyok" << std::endl;
            } else if(cmd == "stop") {
                std::cout << "quit" << std::endl;
                break;
            } else if(cmd == "position") {
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
                        iss >> move;
                        if (move == "moves") {
                            while (iss >> move) {
                                // TODO: Parse moves
                            }
                        }
                    }
                }
            } else if(cmd == "go") {
                if(board != nullptr) {
                    delete search;
                    search = new search_t(*board, tt);

                    move_t bm = search->think(0, MAX_PLY);
                    std::cout << "bestmove " << bm << std::endl;
                }
            }
        }
    }

    return 0;
}
