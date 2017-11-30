

#ifndef GAMEOFLIFE_TERMINAL_DRAW_H
#define GAMEOFLIFE_TERMINAL_DRAW_H

#include <cstddef>
#include <iostream>
#include "gol.hpp"


#ifndef TERMINAL_SLEEP
#define TERMINAL_SLEEP 100000
#endif

using namespace std;

class terminal_visualizer {
public:
    terminal_visualizer(int n_rows_, int n_coloumns_) : 
    n_rows(n_rows_), 
    n_columns(n_coloumns_)
        { }

    void draw(value_type **matrix) {

        for (int k = 0; k < 100; k++) {
            std::cout << "\n" << std::endl;
        }

        for (size_t i = 1; i < n_rows - 1; ++i){
            for (size_t j = 1; j < n_columns - 1; ++j) {
                std::cout << (matrix[i][j]? "O" : ".");
            }
            std::cout << "\n";
        }

        std::cout << "\n" << std::endl;
        usleep(100000);
    }

    void inline end_draw(value_type ** matrix){
        uint64_t sum = 0;
        for (size_t i = 1; i < n_rows - 1; ++i){
            for (size_t j = 1; j < n_columns - 1; ++j) {
                sum += matrix[i][j];
            }
        }

        std::cout << sum << std::endl;
        usleep(TERMINAL_SLEEP);
    }


    size_t n_rows, n_columns;

};


#endif //GAMEOFLIFE_TERMINAL_DRAW_H
