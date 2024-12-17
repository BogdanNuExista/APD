#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

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
    } else {
        return 0;
    }
    
    return 1;
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