

## GAME OF LIFE SIMULATION

This project aims to simulate game of life ([Wikipedia](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)) evaluating different strategies 
of parallelization using fastflow library ([Official web](http://calvados.di.unipi.it/)).


## Compile

In order to compile the project it is important to set FF_ROOT with the path of fastflow library.

```bash
git clone dir
cd fastflow_gameoflife
mkdir build
cd build
cmake -DFF_ROOT=<your fastflow install directory> ..
make
```

## Execute

```bash
./gol <N_ROWS> <N_COLUMNS> <N_ITER> 
--type [farm_h | farm_V | parallel_for | seq | threads]
--screen [empty | terminal | opencv(if avaliable)]
--nworkers <N> 
--random <R> (percentage of live cells at the beginning)
--input <FILE>
--output <FILE>
```








