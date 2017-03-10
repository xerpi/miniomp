#include <pthread.h>

// Default lock for critical sections
extern pthread_mutex_t miniomp_default_lock;

// Default barrier within a parallel region
#define MYBARRIER 1
#if MYBARRIER
// Here you should define all necessary types and variables to implement your barrier
typedef struct {
    // complete the definition of barrier
} miniomp_barrier_t;

extern miniomp_barrier_t miniomp_barrier;
#else
// However, you can do an initial text using Pthread barriers 
extern pthread_barrier_t miniomp_barrier;
#endif

// Functions implemented in this module
void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_barrier(void);
