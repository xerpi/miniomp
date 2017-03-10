// Type declaration for loop worksharing descriptor
typedef struct {
  long start;           // loop bounds and increment 
  long end;
  long incr;

  int schedule;         // schedule kind for loop
  long chunk_size;
  // complete the definition of worksharing descriptor
} miniomp_loop_t;

#define ws_STATIC 	0
#define ws_STATICCHUNK 	1
#define ws_DYNAMIC 	2
#define ws_GUIDED 	3
#define ws_RUNTIME 	4
#define ws_AUTO 	5

extern miniomp_loop_t miniomp_loop;
