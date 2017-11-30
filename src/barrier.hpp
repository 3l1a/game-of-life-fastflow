

#ifndef GAMEOFLIFE_BARRIER_H
#define GAMEOFLIFE_BARRIER_H


#include <mutex>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include "gol.hpp"

using namespace std;


/**
 * Synchronization barrier among threads.
 */
class barrier {

public:
     barrier(uint32_t _workers, uint32_t _iters)
             :  current_iter(0), n_worker(_workers) { }

    /*
     * At the end of each iteration all the workers have to stop in the barrier.
     */
    template
            <typename visualizer>
    void wait(gol<visualizer> &g){
        std::unique_lock<std::mutex> lock(m);
        uint32_t i = current_iter ;
        n_threads_finished[i]++;
        if (n_threads_finished[i] == n_worker){
            current_iter++;
            current_iter%=2;
            n_threads_finished[current_iter] = 0;
            g.screen();
            g.swap_matrices();
            g.fill_borders();
            c.notify_all();
        } else {
            c.wait(lock, [&](){return n_worker == n_threads_finished[i];});
        }
    }



private:

    uint32_t n_worker;
    uint8_t current_iter;
    uint32_t n_threads_finished[2] = {0, 0};
    mutex m;
    condition_variable c;

};

#endif //GAMEOFLIFE_BARRIER_H
