
#ifndef GAMEOFLIFE_EMPTY_DRAW_H
#define GAMEOFLIFE_EMPTY_DRAW_H

#include <iostream>

#include "gol.hpp"

using namespace std;

class empty_visualizer {

    uint32_t columns;
    uint32_t rows;

public:
    empty_visualizer(int n_rows_, int n_columns_) : rows(n_rows_), columns(n_columns_){ }
    void inline draw(value_type **matrix) {}
    void inline end_draw(value_type ** matrix){}
};




#endif //GAMEOFLIFE_EMPTY_DRAW_H
