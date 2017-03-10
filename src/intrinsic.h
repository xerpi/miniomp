// OpenMP intrinsics

// Sets the number of threads to be used in the following parallel regions
void omp_set_num_threads (int n);

// Returns the number of threads in the current parallel region
int omp_get_num_threads (void);

// Returns the identifier of the invoking thread within the team
int omp_get_thread_num (void);
