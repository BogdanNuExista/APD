#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define EPSILON 0.000001 // this is the maximum difference between the reference and the result matrices, like in omp_matrix_mult.c
#define MAX_N 3000
#define MIN_N 1000
#define DEFAULT_BLOCK_SIZE 32

// Matrices declared as global variables for simplicity
double **a, **b, **c_reference, **c_result;

// Function prototypes for all multiplication variants
typedef void (*mult_func)(int n);
typedef void (*parallel_mult_func)(int n, int nthreads);

// Helper functions
void allocate_matrices(int n) {
    a = malloc(n * sizeof(*a));
    b = malloc(n * sizeof(*b));
    c_reference = malloc(n * sizeof(*c_reference));
    c_result = malloc(n * sizeof(*c_result));
    for(int i = 0; i < n; i++) {
        a[i] = malloc(n * sizeof(double));
        b[i] = malloc(n * sizeof(double));
        c_reference[i] = malloc(n * sizeof(double));
        c_result[i] = malloc(n * sizeof(double));
    }
}

void free_matrices(int n) {
    for(int i = 0; i < n; i++) {
        free(a[i]);
        free(b[i]);
        free(c_reference[i]);
        free(c_result[i]);
    }
    free(a);
    free(b);
    free(c_reference);
    free(c_result);
}

// the reason I want the values of the matrices to be between 0 and 1 is
// to make sure the values don't get too big and the program doesn't take too long to run

void init_matrices(int n) {
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            a[i][j] = rand() / (double)RAND_MAX; 
            b[i][j] = rand() / (double)RAND_MAX; 
            c_reference[i][j] = 0.0;
            c_result[i][j] = 0.0;
        }
    }
}

int validate_result(int n) { // this function is the same as in omp_matrix_mult.c
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            if(fabs(c_reference[i][j] - c_result[i][j]) > EPSILON) { 
                return 0;
            }
        }
    }
    return 1;
}

// Reference implementation (i-j-k) for correctness checking
void multiply_ijk_reference(int n) {
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            double sum = 0.0;
            for(int k = 0; k < n; k++) {
                sum += a[i][k] * b[k][j];
            }
            c_reference[i][j] = sum;
        }
    }
}

///////// Serial implementations of all 6 permutations

void multiply_ijk(int n) {
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            double sum = 0.0;
            for(int k = 0; k < n; k++) {
                sum += a[i][k] * b[k][j];
            }
            c_result[i][j] = sum;
        }
    }
}

void multiply_ikj(int n) {
    for(int i = 0; i < n; i++) {
        for(int k = 0; k < n; k++) {
            double aik = a[i][k]; // cache a[i][k] for faster access
            for(int j = 0; j < n; j++) {
                c_result[i][j] += aik * b[k][j];
            }
        }
    }
}

void multiply_jik(int n) {
    for(int j = 0; j < n; j++) {
        for(int i = 0; i < n; i++) {
            double sum = 0.0;
            for(int k = 0; k < n; k++) {
                sum += a[i][k] * b[k][j];
            }
            c_result[i][j] = sum;
        }
    }
}

void multiply_jki(int n) {
    for(int j = 0; j < n; j++) {
        for(int k = 0; k < n; k++) {
            double bkj = b[k][j];
            for(int i = 0; i < n; i++) {
                c_result[i][j] += a[i][k] * bkj;
            }
        }
    }
}

void multiply_kij(int n) {
    for(int k = 0; k < n; k++) {
        for(int i = 0; i < n; i++) {
            double aik = a[i][k];
            for(int j = 0; j < n; j++) {
                c_result[i][j] += aik * b[k][j];
            }
        }
    }
}

void multiply_kji(int n) {
    for(int k = 0; k < n; k++) {
        for(int j = 0; j < n; j++) {
            double bkj = b[k][j];
            for(int i = 0; i < n; i++) {
                c_result[i][j] += a[i][k] * bkj;
            }
        }
    }
}

//////////////////// Parallel implementations
void parallel_multiply_ijk(int n, int nthreads) {
    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                double sum = 0.0;
                for(int k = 0; k < n; k++) {
                    sum += a[i][k] * b[k][j];
                }
                c_result[i][j] = sum;
            }
        }
    }
}

void parallel_multiply_ikj(int n, int nthreads) {
    // Initialize result matrix to zero
    #pragma omp parallel for collapse(2) num_threads(nthreads)
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c_result[i][j] = 0.0;
        }
    }

    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int i = 0; i < n; i++) {
            for(int k = 0; k < n; k++) {
                const double aik = a[i][k];
                for(int j = 0; j < n; j++) {
                    c_result[i][j] += aik * b[k][j];
                }
            }
        }
    }
}

void parallel_multiply_jik(int n, int nthreads) {
    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int j = 0; j < n; j++) {
            for(int i = 0; i < n; i++) {
                double sum = 0.0;
                for(int k = 0; k < n; k++) {
                    sum += a[i][k] * b[k][j];
                }
                c_result[i][j] = sum;
            }
        }
    }
}

void parallel_multiply_jki(int n, int nthreads) {
    // Initialize result matrix to zero
    #pragma omp parallel for collapse(2) num_threads(nthreads)
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c_result[i][j] = 0.0;
        }
    }

    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int j = 0; j < n; j++) {
            for(int k = 0; k < n; k++) {
                const double bkj = b[k][j];
                for(int i = 0; i < n; i++) {
                    c_result[i][j] += a[i][k] * bkj;
                }
            }
        }
    }
}

void parallel_multiply_kij(int n, int nthreads) {
    // Initialize result matrix to zero 
    #pragma omp parallel for collapse(2) num_threads(nthreads)
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c_result[i][j] = 0.0;
        }
    }

    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int k = 0; k < n; k++) {
            for(int i = 0; i < n; i++) {
                const double aik = a[i][k];
                for(int j = 0; j < n; j++) {
                    #pragma omp atomic // this fixed the problem, because multiple threads can update c_result simultaneously 
                    c_result[i][j] += aik * b[k][j];
                }
            }
        }
    }
}

void parallel_multiply_kji(int n, int nthreads) {
    // Initialize result matrix to zero
    #pragma omp parallel for collapse(2) num_threads(nthreads)
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            c_result[i][j] = 0.0;
        }
    }

    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int k = 0; k < n; k++) {
            for(int j = 0; j < n; j++) {
                const double bkj = b[k][j];
                for(int i = 0; i < n; i++) {
                    #pragma omp atomic // this fixed the problem, because multiple threads can update c_result simultaneously
                    c_result[i][j] += a[i][k] * bkj;
                }
            }
        }
    }
}


//////////////////////////////////////// Blocked matrix multiplication
/*
    Handle case where n is not evenly divisible by block_size
        This is achieved by calculating the end indices 
        (i_end, j_end, k_end) for each block, ensuring that 
        the loops do not exceed the matrix boundaries.
        This ensures that all elements of the matrix are processed correctly, 
        even if the matrix size is not a multiple of the block size.
*/
void blocked_multiply_serial(int n, int block_size) {
    // Handle case where n is not evenly divisible by block_size
    // int n_blocks = (n + block_size - 1) / block_size;
    
    for(int ii = 0; ii < n; ii += block_size) {
        for(int jj = 0; jj < n; jj += block_size) {
            for(int kk = 0; kk < n; kk += block_size) {
                // Compute boundary of current block
                int i_end = (ii + block_size < n) ? ii + block_size : n;
                int j_end = (jj + block_size < n) ? jj + block_size : n;
                int k_end = (kk + block_size < n) ? kk + block_size : n;
                
                for(int i = ii; i < i_end; i++) {
                    for(int j = jj; j < j_end; j++) {
                        double sum = c_result[i][j];
                        for(int k = kk; k < k_end; k++) {
                            sum += a[i][k] * b[k][j];
                        }
                        c_result[i][j] = sum;
                    }
                }
            }
        }
    }
}

void blocked_multiply_parallel(int n, int block_size, int nthreads) {
    // int n_blocks = (n + block_size - 1) / block_size;
    
    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp for collapse(2)
        for(int ii = 0; ii < n; ii += block_size) {
            for(int jj = 0; jj < n; jj += block_size) {
                for(int kk = 0; kk < n; kk += block_size) {
                    int i_end = (ii + block_size < n) ? ii + block_size : n;
                    int j_end = (jj + block_size < n) ? jj + block_size : n;
                    int k_end = (kk + block_size < n) ? kk + block_size : n;
                    
                    for(int i = ii; i < i_end; i++) {
                        for(int j = jj; j < j_end; j++) {
                            double sum = c_result[i][j];
                            for(int k = kk; k < k_end; k++) {
                                sum += a[i][k] * b[k][j];
                            }
                            c_result[i][j] = sum;
                        }
                    }
                }
            }
        }
    }
}

/////////////////////// Test harness
void test_algorithm(const char* name, mult_func func, int n) {
    init_matrices(n);
    
    // Generate reference result
    multiply_ijk_reference(n);
    
    // Test algorithm
    double start = omp_get_wtime();
    func(n);
    double end = omp_get_wtime();
    
    // Validate result
    int valid = validate_result(n);
    
    printf("%s (N=%d): Time=%.3f seconds, Valid=%s\n", 
           name, n, end - start, valid ? "yes" : "no");
}

void test_parallel_algorithm(const char* name, parallel_mult_func func, 
                           int n, int nthreads) {
    init_matrices(n);
    
    // Generate reference result
    multiply_ijk_reference(n);
    
    // Test algorithm
    double start = omp_get_wtime();
    func(n, nthreads); 
    double end = omp_get_wtime();
    
    // Validate result
    int valid = validate_result(n);
    
    printf("%s (N=%d, threads=%d): Time=%.3f seconds, Valid=%s\n", 
           name, n, nthreads, end - start, valid ? "yes" : "no");
}

int main() {
    int sizes[] = {500, 1000, 1500};
    int n_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int nthreads = omp_get_max_threads(); // get the number of threads available aka 8
    
    printf("Testing matrix multiplication algorithms\n");
    printf("Number of threads available: %d\n\n", nthreads);
    
    for(int i = 0; i < n_sizes; i++) {
        int n = sizes[i];
        allocate_matrices(n);
        
        printf("\nMatrix size N=%d\n", n);
        printf("=================\n");
        
        // Test serial algorithms
        test_algorithm("Serial IJK", multiply_ijk, n);
        test_algorithm("Serial IKJ", multiply_ikj, n);
        test_algorithm("Serial JIK", multiply_jik, n);
        test_algorithm("Serial JKI", multiply_jki, n);
        test_algorithm("Serial KIJ", multiply_kij, n);
        test_algorithm("Serial KJI", multiply_kji, n);
        
        // Test all parallel algorithms
        test_parallel_algorithm("Parallel IJK", parallel_multiply_ijk, n, nthreads);
        test_parallel_algorithm("Parallel IKJ", parallel_multiply_ikj, n, nthreads);
        test_parallel_algorithm("Parallel JIK", parallel_multiply_jik, n, nthreads);
        test_parallel_algorithm("Parallel JKI", parallel_multiply_jki, n, nthreads);
        test_parallel_algorithm("Parallel KIJ", parallel_multiply_kij, n, nthreads);
        test_parallel_algorithm("Parallel KJI", parallel_multiply_kji, n, nthreads);
        
        // Test blocked algorithms
        int block_sizes[] = {16, 32, 64, 128};
        for(int bs = 0; bs < sizeof(block_sizes)/sizeof(block_sizes[0]); bs++) {
            char name[100];
            sprintf(name, "Blocked Serial (bs=%d)", block_sizes[bs]);
            init_matrices(n);
            double start = omp_get_wtime();
            blocked_multiply_serial(n, block_sizes[bs]);
            double end = omp_get_wtime();
            printf("%s: Time=%.3f seconds\n", name, end - start);
            
            // Test parallel blocked version
            sprintf(name, "Blocked Parallel (bs=%d)", block_sizes[bs]);
            init_matrices(n);
            start = omp_get_wtime();
            blocked_multiply_parallel(n, block_sizes[bs], nthreads);
            end = omp_get_wtime();
            printf("%s: Time=%.3f seconds\n", name, end - start);
        }
        
        free_matrices(n);
    }
    
    return 0;
}