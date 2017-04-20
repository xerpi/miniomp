#!/bin/bash
### Directives pel gestor de cues
# following option makes sure the job will run in the current directory
#$ -cwd
# following option makes sure the job has the same environmnent variables as the submission shell
#$ -V
# Canviar el nom del job
#$ -N lab1-omp
# Per canviar de shell
#$ -S /bin/bash

./benchmark_sieve2.py sieve2_taskloop
