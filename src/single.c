#include "libminiomp.h"

// Declaratiuon of global variable for single work descriptor
miniomp_single_t miniomp_single;

/* This routine is called when first encountering a SINGLE construct.
   Returns true if this is the thread that should execute the clause.  */

bool GOMP_single_start(void)
{
	/*
	 * Stubbed
	 */
	return (true);
}
