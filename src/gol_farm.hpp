//
// Created by elia on 27/06/16.
//

#ifndef GAMEOFLIFE_FAMR_H
#define GAMEOFLIFE_FAMR_H

#include <iostream>
#include <vector>
#include <pipeline.hpp>
#include <farm.hpp>
#include <node.hpp>
#include <err.h>
#include <utils.hpp>
#include <gt.hpp>
#include <make_unique.hpp>
#include <multinode.hpp>
#include "config.hpp"

#include "immintrin.h"
#include "gol.hpp"

using namespace ff;
using namespace std;

/**
 * Farm worker operating over strip of columns of the matrix.
 */
template <typename visualizer>
class Worker: public ff_node {

public:
    gol<visualizer> * g;
    uint32_t const start;
    uint32_t const stop;
    uint32_t const size;

    Worker(gol<visualizer> * _g, uint32_t _start, uint32_t _stop) :
            g(_g),
            start(_start),
            stop(_stop),
            size(_stop - _start){ }

    void *svc(void *task) {
        for (uint32_t i = 1; i < g->n_rows - 1; i++){

#ifdef MMIC
//            __assume(start%16==1);
//            __assume(size%16==0);
//            __assume_aligned((void *)g->m1[i], 64);
//            __assume_aligned((void *)g->m2[i+1], 64);
//            __assume_aligned((void *)g->m2[i], 64);
//            __assume_aligned((void *)g->m1[i-1], 64);
//            __assume_aligned((void *)g->m2[i-1], 64);
//            __assume_aligned((void *)g->m1[i+1], 64);
#endif

            auto v = &g->m1[i+2][start];
/** Data needed in the next cycle are prefetched now. Use of #pragma novector does not seem to work.*/
#pragma prefetch v:_MM_HINT_T0:128
#pragma ivdep
#pragma simd
#pragma vector nontemporal
            for (uint32_t j = start; j < start + size; j++)
                g->compute_pixel(i, j);
            }

        return task;
    }
};


/**
 * Farm worker operating over strip of rows of the matrix.
 */
template <typename visualizer>
class Worker_Rows: public ff_node {
public:
    gol<visualizer> * g;
    uint32_t const start;
    uint32_t const stop;
    uint32_t const size;

    Worker_Rows(gol<visualizer> * _g, uint32_t _start, uint32_t _stop) :
            g(_g),
            start(_start),
            stop(_stop),
            size(_stop - _start){ }

    void *svc(void *task) {
        g->update_range_rows(start, stop);
        return task;
    }
};

/**
 * Get results from all workers.
 */
template<typename visualizer>
class collector : public ff_minode {
public:
    uint32_t n_iter;
    gol<visualizer> * g;
    uint32_t count;
    uint32_t n_workers;

    collector( uint32_t _n_workers, uint32_t _n_iter, gol<visualizer> * _g) :
            n_workers(_n_workers),
            n_iter(_n_iter),
            g(_g),
            count(0){ }

    void *svc(void * ) {
        count++;
        if (count == n_workers){
            count = 0;
            g->screen();
            g->swap_matrices();

            n_iter--;
            if (!n_iter) return EOS;
            g->fill_borders();
        }else
            return GO_ON;
    }
} ;

/**
 * Tell to all worker to start with computation.
 * The work space of each worker is decided when they are created and not by the emitter.
 *
 */
class emitter : public ff_node {
public:

    ff_loadbalancer * const lb;

    emitter(ff_loadbalancer * const _lb) : lb(_lb){

    }

    void *svc(void * ) {
        lb->broadcast_task(new int(42));
        return GO_ON;
    }
} ;


/**
 * Perform farm computation.
 */
template <typename painter>
void gol_farm(uint32_t n_iter, uint32_t n_workers, gol<painter> *g, bool use_columns) {


    g->fill_borders();
    std::vector<unique_ptr<ff_node > > Workers;
    uint32_t effective_n_workers = 0;
    uint32_t bucket = 0;
    if (use_columns) {
        bucket = g->n_columns / n_workers + 1;

        if (bucket % CACHE_LINE/sizeof(value_type) != 0)
            bucket += CACHE_LINE/sizeof(value_type) - bucket%CACHE_LINE/sizeof(value_type);
        uint32_t i;
        for ( i = 0; i < n_workers && (bucket * (i + 1) + 1 < (uint32_t) g->n_columns - 1); ++i, ++effective_n_workers) {
            Workers.push_back(make_unique<Worker<painter> >(g, bucket * i + 1,
                                                            std::min(bucket * (i + 1) + 1,
                                                                     (uint32_t) g->n_columns - 1)));

        }

        uint32_t reminder = (uint32_t) g->n_columns - (bucket * (i));
        if (reminder != 0){
            effective_n_workers++;
            reminder += CACHE_LINE/sizeof(value_type) - reminder%(CACHE_LINE/sizeof(value_type));
            Workers.push_back(make_unique<Worker<painter> >(g, bucket * i + 1,
                                                            bucket * i + 1 + reminder));
        }

    } else {
        bucket = g->n_rows / n_workers + 1;
        for (uint32_t i = 0; i < n_workers; ++i, ++effective_n_workers)
            Workers.push_back(make_unique<Worker_Rows<painter> >(g, bucket * i + 1,
                                                            std::min(bucket * (i + 1) + 1,
                                                                     (uint32_t) g->n_rows - 1)));
    }

    std::cout << "effective workers : " << effective_n_workers << std::endl;
    std::cout << "bucket size       : " << bucket << std::endl;

    ff_Farm<> farm( std::move(Workers));
    emitter e(farm.getlb());
    collector<painter> c(effective_n_workers, n_iter, g);

    farm.remove_collector();
    ff_Pipe<void *> pipe(e, farm, c);
    pipe.wrap_around();
    if (pipe.run_and_wait_end()) error("running farm");


    return;
}

#endif //GAMEOFLIFE_FAMR_H
