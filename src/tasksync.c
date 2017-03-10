#include "libminiomp.h"

// Called when encountering taskwait and taskgroup constructs

void
GOMP_taskwait (void)
{
    printf("TBI: Entered in taskwait, there should be no pending tasks, so I proceed\n");
}

void
GOMP_taskgroup_start (void)
{
    printf("TBI: Starting a taskgroup region, at the end of which I should wait for tasks created here\n");
}

void
GOMP_taskgroup_end (void)
{
    printf("TBI: Finsihed a taskgroup region, there should be no pending tasks, so I proceed\n");
}
