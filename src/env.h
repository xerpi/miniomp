// Type declaration for Internal Control Variables (ICV) structure
typedef struct {
  int nthreads_var;
  // Add other ICVs if needed
} miniomp_icv_t;

// Global variable storing the ICV (internal control variables) supported in our implementation
extern miniomp_icv_t miniomp_icv;

// Functions implemented in this module
void parse_env(void);
