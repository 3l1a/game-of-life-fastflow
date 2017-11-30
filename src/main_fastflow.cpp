
#include "gol.hpp"

#ifdef OPENCV_SCREEN
#include "opencv_visualizer.hpp"
#endif

#include "empty_visualizer.hpp"
#include "terminal_visualizer.hpp"
#include <iostream>
#include <unistd.h>
#include <parallel_for.hpp>
#include <utils.hpp>
#include <pthread.h>
#include <sched.h>
#include <getopt.h>
#include <fstream>
#include "barrier.hpp"
#include "gol_farm.hpp"
#include "gol_config.hpp"

using namespace std;
using namespace ff;

void worker(std::function<void(uint32_t, uint32_t)> f, uint32_t start, uint32_t stop ){
    f(start, stop);
}


/**
 * Fuction used in order to abstract over template type of gol.
 */
template
<typename visualizer>
void execute_gol(gol<visualizer> &g, size_t n_workers, uint32_t n_iter, uint32_t perc,
                 std::string type, bool input_set, ifstream &input, bool output_set, ofstream &output){

    if (input_set) {
        g.load_from_file(input);
        input.close();
    } else {
        g.random_matrix_load(perc);
    }

    ff::ffTime(ff::START_TIME);

    if (type ==  "parallel_for"){
        std::cout << "parallel_for with n_workers = " << n_workers << std::endl;
        ParallelFor pf(n_workers, true, false);
        for (int i = 0; i < n_iter; ++i) {
            g.fill_borders();
            pf.parallel_for_static(1, g.n_rows - 1, 1,0, [&g](uint32_t i) {
                g.update_range_rows(i, i+1);
            }, n_workers);
            g.screen();
            g.swap_matrices();
        }

    } else if (type == "parallel_for_idx"){

        ParallelFor pf(n_workers, true, false);
        std::cout << "parallel_for_idx with n_workers = " << n_workers << std::endl;
        for (int i = 0; i < n_iter; i++){
            g.fill_borders();
            pf.parallel_for_idx(1, g.n_rows - 1, 1, 0 , [&g] (uint32_t start, uint32_t end, uint32_t thid){
                g.update_range_rows(start, end);
            }, n_workers);
            g.screen();
            g.swap_matrices();
        }

    }  else if (type == "threads"){

        std::cout << " c++ threads with n_workers = " << n_workers << std::endl;
        std::vector<std::thread> threads;
        barrier b(n_workers, n_iter);
        size_t bucket = g.n_rows / n_workers + 1;
        g.fill_borders();

        auto body = [&](size_t start, size_t stop) {
            for (uint32_t t = 0; t < n_iter ; ++t){
                g.update_range_rows(start, stop);
                b.wait(g);
            }
            return;
        };

        for (uint32_t w = 0; w < n_workers; ++w){
            threads.push_back( std::thread(worker, body,  bucket * w + 1,
                                           std::min(bucket * (w + 1) + 1, g.n_rows - 1)));
        }

        for (uint32_t w = 0; w < n_workers; ++w){
            threads[w].join();
        }

    } else if (type == "farm_v"){
        std::cout << "farm with vertical split with n_workers = " << n_workers << std::endl;
        gol_farm(n_iter, n_workers, &g, true);

    } else if (type == "farm_h"){
        std::cout << "farm with horizontal split with n_workers = " << n_workers << std::endl;
        gol_farm(n_iter, n_workers, &g, false);

    } else if (type == "seq"){
        std::cout << " sequential" << std::endl;
        for (int i = 0; i < n_iter; ++i){
            g.fill_borders();
            g.update_range_rows(1, g.n_rows - 1);
            g.screen();
            g.swap_matrices();
        }

    } else {
        std::cout << " no valid type of parallelism chosen " << std::endl;
    }

    ff::ffTime(ff::STOP_TIME);
    g.final_op();

    /*
     * print over standard error in order to easily get stats.
     */
    std::cout << ff::ffTime(ff::GET_TIME) << " milliseconds" << std::endl;


    if (output_set){
        for (int i = 1; i < g.n_rows-1; i++) {
            for (int j = 1; j < g.n_columns - 1; j++) {
                char c = (char) g.m1[i][j] + '0';
                output << c;
            }
            output << endl;
        }
        output.close();
    }

}

void print_error_message(){
    std::cout << " execute with following params: "<< std::endl
              << "./gol N, M, K" << std::endl
              << " N : Number of Rows" << std::endl
              << " M : Number of Columns" << std::endl
              << " N : Number of Iterations" << std::endl
              << " optional parameters" << std::endl
              << " --nworkers N? default N=1" << std::endl
              << " --type : farm_h, farm_v, parallel_for, seq, threads. default type=parallel_for" << std::endl
              << " --random R: default R=10" << std::endl
              << " --input <file>: if not specified, program runs a random matrix" << std::endl
              << " --output <file>: if not specified" << std::endl
              << " --screen : terminal, empty, opencv(If available). default screen=empty " << std::endl;
    exit(1);
}

int main(int argc, char * argv[]) {


    uint32_t n_workers = 1;
    uint32_t perc = 10;

    bool check = false;
    bool output_set = false, input_set = false;

    string s;
    string input_file;
    string type("parallel_for");

    ofstream output;
    ifstream input;


    int indexarg = 1;
    int c;
    int digit_optind = 0;

    while (true) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"screen",    required_argument, 0,  's' },
                {"nworkers",  required_argument, 0,  'n' },
                {"input",  required_argument, 0,  'i' },
                {"random", required_argument,  0,  'r' },
                {"output",  required_argument, 0, 'o'},
                {"type",  required_argument, 0, 't'},
        };

        c = getopt_long(argc, argv, "s:n:i:r:o:t:",
                        long_options, &option_index);
        if (c == -1)
            break;

        // only because all the parameters has an argument.
        indexarg += 2;

        switch (c) {

            case 'i':
                input.open(optarg);
                input_set = true;
                if (input.fail()){
                    //fixme add full path.
                    cerr << "input file " << optarg << " doesn't exist" << endl;
                    exit(1);
                }
                break;
            case 'o':
                output.open(optarg);
                output_set = true;
                if (output.fail()){
                    cerr << "error in opening ouput file " << optarg << endl;
                    exit(1);
                }
                break;
            case 's':
                s = string(optarg);
                break;
            case 't':
                type = string(optarg);
                break;
            case 'n':
                n_workers = atoi(optarg);
                break;
            case 'r':
                perc = atoi(optarg);
                if (perc < 1 || perc > 99){
                    cerr << "random argument must be in (0, 100) interval" << endl;
                    print_error_message();
                    exit(1);
                }
                break;

            default:
                break;
        }
    }


    if (argc - indexarg < 3){
        print_error_message();
        exit(1);
    }

    uint32_t n = atoi(argv[indexarg++]);
    uint32_t m = atoi(argv[indexarg++]);
    uint32_t n_iter = atoi(argv[indexarg++]);

    if (n == 0 || m == 0 || n_iter == 0){
        print_error_message();
        exit(1);
    }


    if (s == "terminal"){
        gol<terminal_visualizer> g(n , m );
        execute_gol( g, n_workers, n_iter,  perc, type, input_set, input, output_set, output);
#ifdef OPENCV_SCREEN
    } else if (s == "opencv"){
        gol<opencv_visualizer> g(n, m);
        execute_gol(  g, n_workers, n_iter,  perc, type, input_set, input, output_set, output);
#endif
    } else {
        gol<empty_visualizer> g(n, m);
        execute_gol(  g, n_workers, n_iter,  perc, type, input_set, input, output_set, output);
    }

    return 0;
}
