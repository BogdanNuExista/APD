#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

#define MAX_CMD_LEN 256
#define MAX_RESULT_LEN 4096
#define TAG_WORK 1
#define TAG_RESULT 2

// Command types
#define CMD_PRIMES 1
#define CMD_PRIMEDIVISORS 2
#define CMD_ANAGRAMS 3
#define CMD_WAIT 4

typedef struct {
    int type;
    char client[64];
    char params[192];
} Command;

// Worker functions
int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

void count_primes(int n, char *result) {
    int count = 0;
    for (int i = 2; i <= n; i++) {
        if (is_prime(i)) count++;
    }
    sprintf(result, "Number of primes up to %d: %d", n, count);
}

void count_prime_divisors(int n, char *result) {
    int original = n;
    int count = 0;
    int divisors[100] = {0};
    int div_count = 0;
    
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0 && is_prime(i)) {
            divisors[div_count++] = i;
            while (n % i == 0) {
                n /= i;
            }
        }
    }
    
    if (n > 1 && is_prime(n)) {
        divisors[div_count++] = n;
    }
    
    sprintf(result, "Number %d has %d prime divisors: ", original, div_count);
    for (int i = 0; i < div_count; i++) {
        char temp[32];
        sprintf(temp, "%d ", divisors[i]);
        strcat(result, temp);
    }
}

void generate_anagrams(char *str, int start, int end, char *result, int *count) {
    if (*count >= 100) return; // Limit number of anagrams to prevent buffer overflow
    
    if (start == end) {
        if (strlen(result) + strlen(str) + 2 < MAX_RESULT_LEN) {
            strcat(result, str);
            strcat(result, " ");
            (*count)++;
        }
    } else {
        for (int i = start; i <= end && *count < 100; i++) {
            // Swap
            char temp = str[start];
            str[start] = str[i];
            str[i] = temp;
            
            generate_anagrams(str, start + 1, end, result, count);
            
            // Restore
            temp = str[start];
            str[start] = str[i];
            str[i] = temp;
        }
    }
}

// Server functions
void write_to_log(FILE *log_file, const char *client, const char *action, const char *details) {
    time_t now;
    time(&now);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    fprintf(log_file, "[%s] %s: %s - %s\n", timestamp, client, action, details);
    fflush(log_file);
}

void write_result_to_file(const char *client, const char *result) {
    char filename[128];
    snprintf(filename, sizeof(filename), "%s_output.txt", client);
    FILE *f = fopen(filename, "a");
    if (f) {
        fprintf(f, "%s\n", result);
        fclose(f);
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////

// For matrix operations
#define CMD_MATRIXADD 5
#define CMD_MATRIXMULT 6
#define MATRIX_SIZE_THRESHOLD 100 // Matrices larger than 100x100 will be processed in parallel
#define MATRIX_CHUNK_SIZE 50 // Size of submatrix chunks for parallel processing

typedef struct {
    int size;        // Matrix size
    int start_row;   // Starting row for this chunk
    int end_row;     // Ending row for this chunk
    int num_workers; // Total number of workers
    int worker_rank; // This worker's rank
    double *matrix1; // Full first matrix for multiplication
    double *matrix2; // Full second matrix
    double *result;  // Partial result matrix
} ParallelMatrixTask;

// Function to read matrix from file
int read_matrix(const char* filename, int size, double* matrix) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (fscanf(file, "%lf", &matrix[i * size + j]) != 1) {
                fclose(file);
                return 0;
            }
        }
    }
    fclose(file);
    return 1;
}

// Function to write matrix to file
void write_matrix(const char* filename, int size, double* matrix) {
    FILE* file = fopen(filename, "w");
    if (!file) return;
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            fprintf(file, "%.2f ", matrix[i * size + j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Improved matrix multiplication function
void parallel_matrix_mult(ParallelMatrixTask *task) {
    int size = task->size;
    int start = task->start_row;
    int end = task->end_row;
    
    // Each worker computes its assigned rows
    for (int i = start; i < end; i++) {
        for (int j = 0; j < size; j++) {
            double sum = 0;
            for (int k = 0; k < size; k++) {
                sum += task->matrix1[i * size + k] * task->matrix2[k * size + j];
            }
            task->result[i * size + j] = sum;
        }
    }
}

// Improved matrix addition function
void parallel_matrix_add(ParallelMatrixTask *task) {
    int size = task->size;
    int start = task->start_row;
    int end = task->end_row;
    
    for (int i = start; i < end; i++) {
        for (int j = 0; j < size; j++) {
            task->result[i * size + j] = 
                task->matrix1[i * size + j] + task->matrix2[i * size + j];
        }
    }
}

void check_completed_work(int *worker_status, int size, FILE *log_file) {
    MPI_Status status;
    char result[MAX_RESULT_LEN];
    int flag;

    for (int i = 1; i < size; i++) {
        if (worker_status[i] == 1) {
            MPI_Iprobe(i, TAG_RESULT, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                MPI_Recv(result, MAX_RESULT_LEN, MPI_CHAR, i, TAG_RESULT, 
                        MPI_COMM_WORLD, &status);
                worker_status[i] = 0;
                
                if (strlen(result) > 0) {
                    char client_id[64];
                    char *result_start = strstr(result, "RESULT:");
                    if (result_start) {
                        int client_len = result_start - result - 7; // subtract "CLIENT:" length
                        strncpy(client_id, result + 7, client_len);
                        client_id[client_len] = '\0';
                        char *actual_result = result_start + 7;
                        
                        write_to_log(log_file, client_id, "COMPLETED", actual_result);
                        write_result_to_file(client_id, actual_result);
                    }
                }
            }
        }
    }
}

void handle_matrix_operation(Command* cmd, int* worker_status, int size, FILE* log_file) {
    char size_str[32], file1[256], file2[256];
    sscanf(cmd->params, "%s %s %s", size_str, file1, file2);
    int matrix_size = atoi(size_str);
    
    // Allocate memory for matrices
    double* matrix1 = malloc(matrix_size * matrix_size * sizeof(double));
    double* matrix2 = malloc(matrix_size * matrix_size * sizeof(double));
    double* result = calloc(matrix_size * matrix_size, sizeof(double));
    
    if (!matrix1 || !matrix2 || !result) {
        write_to_log(log_file, cmd->client, "ERROR", "Failed to allocate memory");
        free(matrix1);
        free(matrix2);
        free(result);
        return;
    }
    
    // Read input matrices
    if (!read_matrix(file1, matrix_size, matrix1)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to read file: %s", file1);
        write_to_log(log_file, cmd->client, "ERROR", error_msg);
        free(matrix1);
        free(matrix2);
        free(result);
        return;
    }
    
    if (!read_matrix(file2, matrix_size, matrix2)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to read file: %s", file2);
        write_to_log(log_file, cmd->client, "ERROR", error_msg);
        free(matrix1);
        free(matrix2);
        free(result);
        return;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Processing %dx%d matrices", matrix_size, matrix_size);
    write_to_log(log_file, cmd->client, "STARTED", log_msg);
    
    if (matrix_size <= MATRIX_SIZE_THRESHOLD) {
        // Sequential processing for small matrices
        ParallelMatrixTask task = {
            .size = matrix_size,
            .start_row = 0,
            .end_row = matrix_size,
            .matrix1 = matrix1,
            .matrix2 = matrix2,
            .result = result
        };
        
        if (cmd->type == CMD_MATRIXADD) {
            parallel_matrix_add(&task);
        } else {
            parallel_matrix_mult(&task);
        }
        
        // Write result directly for sequential processing
        char result_file[512];
        snprintf(result_file, sizeof(result_file), "%s_result.txt", cmd->client);
        write_matrix(result_file, matrix_size, result);
        
        snprintf(log_msg, sizeof(log_msg), 
                "Sequential matrix operation completed. Result written to %s", result_file);
        write_to_log(log_file, cmd->client, "COMPLETED", log_msg);
    } else {
        // Wait for all workers to be free before starting parallel operation
        int all_workers_free = 0;
        while (!all_workers_free) {
            all_workers_free = 1;
            for (int i = 1; i < size; i++) {
                if (worker_status[i] == 1) {
                    all_workers_free = 0;
                    break;
                }
            }
            if (!all_workers_free) {
                check_completed_work(worker_status, size, log_file);
                usleep(10000);
            }
        }
        
        // Parallel processing for large matrices
        int num_workers = size - 1;
        int rows_per_worker = matrix_size / num_workers;
        int extra_rows = matrix_size % num_workers;
        
        // Mark all workers as busy
        for (int i = 1; i < size; i++) {
            worker_status[i] = 1;
        }
        
        // Send matrices and work distribution to workers
        for (int i = 1; i < size; i++) {
            // Calculate rows for this worker
            int worker_rows = rows_per_worker + (extra_rows > 0 ? 1 : 0);
            extra_rows--;
            
            int start_row = (i - 1) * rows_per_worker + 
                           (extra_rows > 0 ? extra_rows : 0);
            int end_row = start_row + worker_rows;
            
            // Send operation parameters
            MPI_Send(&matrix_size, 1, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
            MPI_Send(&cmd->type, 1, MPI_INT, i, TAG_WORK + 1, MPI_COMM_WORLD);
            
            // Send matrices
            MPI_Send(matrix1, matrix_size * matrix_size, MPI_DOUBLE, i, 
                    TAG_WORK + 2, MPI_COMM_WORLD);
            MPI_Send(matrix2, matrix_size * matrix_size, MPI_DOUBLE, i, 
                    TAG_WORK + 3, MPI_COMM_WORLD);
            
            // Send work range
            int work_info[2] = {start_row, end_row};
            MPI_Send(work_info, 2, MPI_INT, i, TAG_WORK + 4, MPI_COMM_WORLD);
        }
        
        // Wait for all workers to complete
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            int worker_rows[2];
            MPI_Recv(worker_rows, 2, MPI_INT, i, TAG_RESULT, MPI_COMM_WORLD, &status);
            
            int start_row = worker_rows[0];
            int end_row = worker_rows[1];
            int result_size = (end_row - start_row) * matrix_size;
            
            // Receive partial results
            MPI_Recv(&result[start_row * matrix_size], result_size, MPI_DOUBLE, 
                    i, TAG_RESULT + 1, MPI_COMM_WORLD, &status);
            
            // Mark worker as free
            worker_status[i] = 0;
        }
        
        // Write final result
        char result_file[512];
        snprintf(result_file, sizeof(result_file), "%s_result.txt", cmd->client);
        write_matrix(result_file, matrix_size, result);
        
        snprintf(log_msg, sizeof(log_msg), 
                "Parallel matrix operation completed. Result written to %s", result_file);
        write_to_log(log_file, cmd->client, "COMPLETED", log_msg);
    }
    
    // Cleanup
    free(matrix1);
    free(matrix2);
    free(result);
}

// Modified worker function to handle improved matrix operations
void handle_parallel_matrix_operation(int rank) {
    MPI_Status status;
    
    // Receive matrix size
    int matrix_size;
    MPI_Recv(&matrix_size, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    
    // Receive operation type
    int operation;
    MPI_Recv(&operation, 1, MPI_INT, 0, TAG_WORK + 1, MPI_COMM_WORLD, &status);
    
    // Allocate and receive matrices
    double* matrix1 = malloc(matrix_size * matrix_size * sizeof(double));
    double* matrix2 = malloc(matrix_size * matrix_size * sizeof(double));
    
    MPI_Recv(matrix1, matrix_size * matrix_size, MPI_DOUBLE, 0, 
            TAG_WORK + 2, MPI_COMM_WORLD, &status);
    MPI_Recv(matrix2, matrix_size * matrix_size, MPI_DOUBLE, 0, 
            TAG_WORK + 3, MPI_COMM_WORLD, &status);
    
    // Receive work chunk information
    int work_info[2];
    MPI_Recv(work_info, 2, MPI_INT, 0, TAG_WORK + 4, MPI_COMM_WORLD, &status);
    
    int start_row = work_info[0];
    int end_row = work_info[1];
    int result_size = (end_row - start_row) * matrix_size;
    
    // Allocate result array
    double* partial_result = calloc(matrix_size * matrix_size, sizeof(double));
    
    // Create task structure
    ParallelMatrixTask task = {
        .size = matrix_size,
        .start_row = start_row,
        .end_row = end_row,
        .matrix1 = matrix1,
        .matrix2 = matrix2,
        .result = partial_result
    };
    
    // Perform operation
    if (operation == CMD_MATRIXADD) {
        parallel_matrix_add(&task);
    } else {
        parallel_matrix_mult(&task);
    }
    
    // Send back row information
    MPI_Send(work_info, 2, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
    
    // Send partial result
    MPI_Send(&partial_result[start_row * matrix_size], result_size, MPI_DOUBLE, 
            0, TAG_RESULT + 1, MPI_COMM_WORLD);
    
    // Cleanup
    free(matrix1);
    free(matrix2);
    free(partial_result);
}

///////////////////////////////////////////////////////////////////////////////////////////////



int parse_command(const char *line, Command *cmd) {
    char cmd_type[32];
    if (strncmp(line, "WAIT", 4) == 0) {
        cmd->type = CMD_WAIT;
        sscanf(line, "WAIT %s", cmd->params);
        strcpy(cmd->client, "SYSTEM");
        return 1;
    }
    
    sscanf(line, "%s %s %s", cmd->client, cmd_type, cmd->params);
    
    if (strcmp(cmd_type, "PRIMES") == 0) {
        cmd->type = CMD_PRIMES;
    } else if (strcmp(cmd_type, "PRIMEDIVISORS") == 0) {
        cmd->type = CMD_PRIMEDIVISORS;
    } else if (strcmp(cmd_type, "ANAGRAMS") == 0) {
        cmd->type = CMD_ANAGRAMS;
    }
    else if(strcmp(cmd_type, "MATRIXADD") == 0) {
        cmd->type = CMD_MATRIXADD;
        char param1[32], param2[32];
        sscanf(line, "%s %s %s %s %s", cmd->client, cmd_type, cmd->params, param1, param2);
        // add param1 and param2 to cmd->params
        strcat(cmd->params, " ");
        strcat(cmd->params, param1);
        strcat(cmd->params, " ");
        strcat(cmd->params, param2);
        printf("CMDDDDDDDDDDDDDDDDDDDDDDD PARAMS %s\n", cmd->params);
    } else if(strcmp(cmd_type, "MATRIXMULT") == 0) {
        cmd->type = CMD_MATRIXMULT;
        char param1[32], param2[32];
        sscanf(line, "%s %s %s %s %s", cmd->client, cmd_type, cmd->params, param1, param2);
        // add param1 and param2 to cmd->params
        strcat(cmd->params, " ");
        strcat(cmd->params, param1);
        strcat(cmd->params, " ");
        strcat(cmd->params, param2);
        printf("CMDDDDDDDDDDDDDDDDDDDDDDD PARAMS %s\n", cmd->params);
    } else {
        return 0;
    }
    
    return 1;
}

void run_worker(int rank) {
    printf("Worker %d started\n", rank);
    Command cmd;
    char result[MAX_RESULT_LEN];
    char final_result[MAX_RESULT_LEN];
    MPI_Status status;

    while (1) {
        MPI_Recv(&cmd, sizeof(Command), MPI_CHAR, 0, TAG_WORK, MPI_COMM_WORLD, &status);

        if (cmd.type == -1) {
            printf("Worker %d received termination signal\n", rank);
            break;
        }

        memset(result, 0, MAX_RESULT_LEN);

        printf("CMD TYPE %d and PARAMS %s\n", cmd.type, cmd.params);  
        
        switch (cmd.type) {
            case CMD_PRIMES:
                printf("Worker %d processing PRIMES command\n", rank);
                count_primes(atoi(cmd.params), result);
                break;
                
            case CMD_PRIMEDIVISORS:
                printf("Worker %d processing PRIMEDIVISORS command\n", rank);
                count_prime_divisors(atoi(cmd.params), result);
                break;
                
            case CMD_ANAGRAMS:
                printf("Worker %d processing ANAGRAMS command\n", rank);
                char word[192];
                strncpy(word, cmd.params, sizeof(word) - 1);
                word[sizeof(word) - 1] = '\0';
                strcpy(result, "Anagrams: ");
                int count = 0;
                generate_anagrams(word, 0, strlen(word) - 1, result, &count);
                break;

            case CMD_MATRIXADD:
            case CMD_MATRIXMULT:
                handle_parallel_matrix_operation(rank);
                break;
            
        }

        // Format final result with client ID
        snprintf(final_result, MAX_RESULT_LEN, "CLIENT:%s RESULT:%s", cmd.client, result);
        
        MPI_Send(final_result, MAX_RESULT_LEN, MPI_CHAR, 0, TAG_RESULT, MPI_COMM_WORLD);
        printf("Worker %d sent result\n", rank);
    }

    printf("Worker %d finished\n", rank);
}

void run_server(int size, const char *cmd_filename) {
    FILE *cmd_file = fopen(cmd_filename, "r");
    FILE *log_file = fopen("dispatcher.log", "w");
    
    if (!cmd_file || !log_file) {
        printf("Error opening files\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return;
    }

    printf("Main server started with %d workers\n", size - 1);
    fprintf(log_file, "Server started with %d workers\n", size - 1);
    fflush(log_file);

    int *worker_status = calloc(size, sizeof(int));
    char line[MAX_CMD_LEN];
    Command cmd;

    while (fgets(line, sizeof(line), cmd_file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (parse_command(line, &cmd)) {
            if (cmd.type == CMD_WAIT) {
                int wait_time = atoi(cmd.params);
                printf("Waiting for %d seconds...\n", wait_time);
                
                time_t start_time = time(NULL);
                while (time(NULL) - start_time < wait_time) {
                    check_completed_work(worker_status, size, log_file);
                    usleep(10000);
                }
                continue;
            }

            write_to_log(log_file, cmd.client, "RECEIVED", line);
            printf("Received command: %s\n", line);

            // Handle matrix operations separately
            if (cmd.type == CMD_MATRIXADD || cmd.type == CMD_MATRIXMULT) {
                handle_matrix_operation(&cmd, worker_status, size, log_file);
            } else {
                // Handle non-matrix operations
                int worker = -1;
                while (worker == -1) {
                    check_completed_work(worker_status, size, log_file);
                    
                    for (int i = 1; i < size; i++) {
                        if (worker_status[i] == 0) {
                            worker = i;
                            worker_status[i] = 1;
                            break;
                        }
                    }
                    
                    if (worker == -1) {
                        usleep(10000);
                    }
                }

                char dispatch_msg[256];
                sprintf(dispatch_msg, "Dispatched to worker %d", worker);
                write_to_log(log_file, cmd.client, "DISPATCHED", dispatch_msg);
                printf("Dispatching to worker %d\n", worker);

                MPI_Send(&cmd, sizeof(Command), MPI_CHAR, worker, TAG_WORK, MPI_COMM_WORLD);
            }
        }
    }

    // Wait for all workers to complete their tasks
    int all_complete = 0;
    while (!all_complete) {
        check_completed_work(worker_status, size, log_file);
        
        all_complete = 1;
        for (int i = 1; i < size; i++) {
            if (worker_status[i] == 1) {
                all_complete = 0;
                break;
            }
        }
        usleep(10000);
    }

    // Send termination signal to all workers
    printf("Sending termination signal to workers...\n");
    cmd.type = -1;
    for (int i = 1; i < size; i++) {
        MPI_Send(&cmd, sizeof(Command), MPI_CHAR, i, TAG_WORK, MPI_COMM_WORLD);
    }

    free(worker_status);
    fclose(cmd_file);
    fclose(log_file);
    printf("Main server finished\n");
}

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) { 
        if (argc != 2) { 
            printf("Usage: %s <command_file>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        run_server(size, argv[1]);
    } else {
        run_worker(rank);
    }

    MPI_Finalize();
    return 0;
}