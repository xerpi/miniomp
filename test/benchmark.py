#!/usr/bin/python

import time
import os
import sys
import subprocess

if len(sys.argv) < 2:
	print('Usage:\n\t', sys.argv[0], ' name')
	sys.exit()

env = os.environ.copy()

name = sys.argv[1]
name_omp = "./" + name + "-omp"
name_gomp = "./" + name + "-gomp"
N = 5

subprocess.call(["make", "-C", "../src"])
subprocess.call(["make", name_omp])
subprocess.call(["make", name_gomp])

total_omp = 0.0
for i in range(N):
	env["OMP_NUM_THREADS"] = "7"
	start = time.perf_counter()
	subprocess.call([name_omp], env=env)
	end = time.perf_counter()
	total_omp += end - start
mean_omp = total_omp / N

total_gomp = 0.0
for i in range(N):
	start = time.perf_counter()
	subprocess.call([name_gomp])
	end = time.perf_counter()
	total_gomp += end - start
mean_gomp = total_gomp / N

print('Mean omp: ', mean_omp)
print('Mean gomp: ', mean_gomp)
