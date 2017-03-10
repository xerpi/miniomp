#!/bin/bash

USAGE="\n USAGE: ./run-extrae.sh prog num_threads\n
        prog        -> omp program name\n
        num_threads -> number of threads\n"

if (test $# -lt 2 || test $# -gt 2)
then
        echo -e $USAGE
        exit 0
fi

export LD_PRELOAD=$EXTRAE_HOME/lib/libomptrace.so

OMP_NUM_THREADS=$2 ./$1
