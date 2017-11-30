

#ifndef __GOL__H_
#define __GOL__H_

#include <iostream>
#include <unistd.h>
#include <vector>
#include <thread>
#include <parallel_for.hpp>
#include <map.hpp>
#include <taskf.hpp>
#include <assert.h>
#include "gol_config.hpp"


using namespace std;
using namespace ff;

typedef uint16_t value_type;


template <typename visualizer>
class gol {

private:
    value_type * t1;
    value_type * t2;

public:
    gol(int M, int N) : n_rows(M + 2), n_columns(N + 2) {

        /* Initialize the visualizer. */
        d = new visualizer(n_rows, n_columns);
        /*Compute the sow size in such way every row is cache aligned*/
        n_coloumns_mm = (n_columns % CACHE_LINE/sizeof(value_type) == 0? n_columns :
            n_columns + CACHE_LINE / sizeof(value_type) - n_columns % CACHE_LINE/sizeof(value_type));
        //n_columns_mm = n_columns;
        m1 = (value_type **) _mm_malloc(sizeof(value_type *)*n_rows, CACHE_LINE);
        m2 = (value_type **) _mm_malloc(sizeof(value_type *)*n_rows, CACHE_LINE);
        t1 = (value_type *) _mm_malloc(sizeof(value_type)*
            (n_coloumns_mm*n_rows + CACHE_LINE/sizeof(value_type)), CACHE_LINE);
        t2 = (value_type *) _mm_malloc(sizeof(value_type)*
            (n_coloumns_mm*n_rows + CACHE_LINE/sizeof(value_type)), CACHE_LINE);
        value_type * temp1 = t1 + CACHE_LINE/sizeof(value_type) - 1;
        value_type * temp2 = t2 + CACHE_LINE/sizeof(value_type) - 1;

        for (long i = 0; i < n_rows; i++) {
            m1[i] = temp1 + i*n_coloumns_mm + 1;
            m2[i] = temp2 + i*n_coloumns_mm + 1;
        }

    }


    /**
     * Compute and return the number of neighbors of the cell (i, j).
     */
    value_type inline count_neighbors(value_type **input, size_t i, size_t j) {
        return   input[i-1][j-1]
                 + input[i-1][j]
                 + input[i-1][j+1]
                 + input[i][j-1]
                 + input[i][j+1]
                 + input[i+1][j-1]
                 + input[i+1][j]
                 + input[i+1][j+1];
    }


    /**
     * Compute the next value of the cell (i, j).
     */
    void  inline compute_pixel(int32_t i, int32_t j){
        value_type count = count_neighbors(m1, i, j);
        m2[i][j] = (m1[i][j]? ((count == 3 || count == 2)? 1 : 0) : (count == 3? 1 : 0));
    }


    /**
     * Update function for matrices horizontally split.
     */
    void inline update_range_rows(uint32_t start, uint32_t stop){
        for (int i = start; i < stop; ++i) {
            for (int j = 1; j < n_columns - 1; ++j) {
                compute_pixel(i, j);
            }
        }
    }


    /**
     * Alternative rows update.
     */
    void inline update_range_rows_alternative(uint32_t start, uint32_t stop){
        for (int i = start; i < stop; i++) {
            value_type * h = m1[i-1] + 1;
            value_type * c = m1[i] + 1;
            value_type * l = m1[i+1] + 1;
#pragma ivdep
#pragma simd
#pragma vector nontemporal
            for ( uint16_t j = 1; j != n_columns - 1; ++c, ++h, ++l, ++j) {
                uint16_t count = *(c - 1) + *(c + 1) 
                                + *(h - 1) + *h + *(h + 1)
                                + *(l - 1) + *l + *(l + 1) ;
                m2[i][j] = (*c? ((count == 3 || count == 2)? 1 : 0) : (count == 3? 1 : 0));
            }
        }
    }


    /**
     * Update function for matrices horizontally split.
     */
    void inline update_row(uint32_t i){
#pragma ivdep
#pragma simd
#pragma vector nontemporal
        for (int j = 1; j < n_columns - 1; j++) {
            compute_pixel(i, j);
        }

    }

    /**
     * This function fills the border of the read matrix with the cell
     * in the other side of the matrix.
     */
    void inline fill_borders(){

        /* Angular pixels */
        m1[0][0] = m1[n_rows - 2][n_columns - 2];
        m1[0][n_columns - 1] = m1[n_rows - 2][1];
        m1[n_rows - 1][0] = m1[1][n_columns - 2];
        m1[n_rows - 1][n_columns - 1] = m1[1][1];

        /* Boarder rows */
        for (size_t i = 1; i < n_columns -1; i++) {
            m1[0][i] = m1[n_rows - 2][i];
            m1[n_rows - 1][i] = m1[1][i];
        }

        /* Boarder columns */
        for (size_t i = 1; i < n_rows -1; i++) {
            m1[i][0] = m1[i][n_columns - 2];
            m1[i][n_columns - 1] = m1[i][1];
        }
    }


    /**
     * Swap the two matrices.
     */
    void swap_matrices(){
        auto temp = m1;
        m1 = m2;
        m2 = temp;
    }


    /**
     * Final operation to perform after the simulation.
     * It may be useful to print some results depending from the matrix
     * and thus constraint the program to execute the simulation.
     */
    void  final_op(){
        d->end_draw(m2);
    }


    /**
     * Screen the current matrix.
     */
    void screen(){
        d->draw(m2);
    }


    /** Number of rows.     */
    size_t n_rows;
    /** Number of columns.  */
    size_t n_columns;
    /** Size to skip for the next row.
    * It is important in order to maintain rows cache line aligned. */
    size_t n_coloumns_mm;
    /** Matrix 1. */
    value_type **m1;
    /** Matrix 2. */
    value_type **m2;
    /** Object to screen the current state of game of life. */
    visualizer * d;


    void random_matrix_load(uint32_t perc){
        for (int i = 1; i < n_rows - 1; i++){
            for (int j = 1; j < n_columns - 1; j++){
                m1[i][j] = (rand()%100 < perc ? 1 : 0);
                m2[i][j] = 0;
            }
        }
    }


    void load_from_file(ifstream &input){
        for (int i = 1; i < n_rows - 1; i++){
            for (int j = 1; j < n_columns - 1; j++){
                char c;

                if (input.eof()){
                    cerr << "input file not properly formed" << endl;
                    exit(1);
                }

                input >> c;
                cout << c;
                switch (c){
                    case '0':
                        m1[i][j] = 0;
                        break;
                    case '1':
                        m1[i][j] = 1;
                        break;
                    default:
                        cerr << "input file not properly formed" << endl;
                        exit(1);

                }

                m2[i][j] = 0;
            }
        }
    }

    ~gol(){

        delete d;
        _mm_free(t1);
        _mm_free(t2);
        _mm_free(m1);
        _mm_free(m2);
    }

};

#endif
