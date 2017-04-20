#!/usr/bin/python3

import time
import sys
import subprocess

if len(sys.argv) < 2:
	print('Usage:\n\t', sys.argv[0], ' name')
	sys.exit()

name = sys.argv[1]
name_omp = "./" + name + "-omp"
name_gomp = "./" + name + "-gomp"
N = 5
threads_range = {1, 2, 4, 8, 16}

subprocess.call(["make", "-C", "../src"])
subprocess.call(["make", name_omp])
subprocess.call(["make", name_gomp])

results_omp = {}
results_gomp = {}

def run_benchmark(program, times, num_threads):
	total = 0.0
	for i in range(times):
		start = time.perf_counter()

		subprocess.call([program, "50000000", str(num_threads)])

		end = time.perf_counter()

		total += end - start
	return total / times

def print_results(results):
	print('{', end='')
	for key, value in results.items():
		print('(' + str(key) + ', ' + str(value) + ')', end='')
	print('}')

for i in threads_range:
	results_omp[i] = run_benchmark(name_omp, N, i)
	results_gomp[i] = run_benchmark(name_gomp, N, i)

print_results(results_omp)
print_results(results_gomp)
