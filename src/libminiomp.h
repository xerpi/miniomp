#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "intrinsic.h"
#include "env.h"
#include "parallel.h"
#include "synchronization.h"
#include "loop.h"
#include "single.h"
#include "task.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

#define errprintf(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
#define dbgprintf(...) errprintf(__VA_ARGS__)
#else
#define dbgprintf(...) (void)0
#endif

typedef struct miniomp_specific_t {
	int id;
	miniomp_task_t *current_task;
} miniomp_specific_t;

miniomp_specific_t *miniomp_get_specific(void);
int miniomp_set_specific(const void *value);
