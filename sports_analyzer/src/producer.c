#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../include/producer.h"
#include "../include/utils.h"

#define MAX_PATH 1024


/*
    Function to process a CSV file and add its data to the shared buffer
    The data is read line by line and added to the buffer as long the buffer is not full
    and there are still active consumers
*/
void process_csv_file(const char* file_path, SharedBuffer* buffer) {

    start_hotspot(&buffer->profiler, "csv_file_processing");

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_path);
        return;
    }

    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Add data to buffer
        pthread_mutex_lock(&buffer->mutex);
        
        // Check if there are still active consumers
        if (buffer->active_consumers == 0) {
            pthread_mutex_unlock(&buffer->mutex);
            fclose(file);
            return;
        }
        
        while (buffer->count == buffer->size) { 
            pthread_cond_wait(&buffer->not_full, &buffer->mutex);
            // Check again after waking up
            if (buffer->active_consumers == 0) {
                pthread_mutex_unlock(&buffer->mutex);
                fclose(file);
                return;
            }
        }
        
        strcpy(buffer->entries[buffer->in].data, line);
        strcpy(buffer->entries[buffer->in].filename, file_path);

        buffer->in = (buffer->in + 1) % buffer->size;
        buffer->count++;
        
        pthread_cond_signal(&buffer->not_empty);
        pthread_mutex_unlock(&buffer->mutex);
    }

    printf("Finished reading file: %s\n", file_path);
    fclose(file);

    end_hotspot(&buffer->profiler, "csv_file_processing");
}

/*
    The folders contain a lot of csv files and I decided to use a recursive function to search for them
*/
void search_csv_files(const char* dir_path, SharedBuffer* buffer) {
    DIR* dir;
    struct dirent* entry;
    char path[MAX_PATH];

    if ((dir = opendir(dir_path)) == NULL) {
        perror("opendir() error");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        if(entry->d_type == DT_DIR) {
            search_csv_files(path, buffer);
        } else {
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcmp(entry->d_name + len - 4, ".csv") == 0) {
                
                if(strstr(path, "atp_players") != NULL) { // already processed players in producer
                    continue;
                }

                strcpy(buffer->entries[buffer->in].filename, path);
                
                process_csv_file(path, buffer);
            }
        }
    }

    closedir(dir);
}

/*
    Function to read football players from a CSV file and add them to the shared buffer
*/
void read_football_players_in_shared_buffer(SharedBuffer *buffer)
{
    FILE* file = fopen("data/football/atp_players.csv", "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", "data/football/atp_players.csv");
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Add data to buffer
        pthread_mutex_lock(&buffer->mutex);
        while (buffer->count == buffer->size) { 
            pthread_cond_wait(&buffer->not_full, &buffer->mutex);
        }
        
        char *p=strtok(line, ",");
        int i=0;
        while(p!=NULL)
        {
            switch(i)
            {
                case 0:
                    buffer->players[buffer->player_count].player_id = atoi(p);
                    break;
                case 1:
                    strcpy(buffer->players[buffer->player_count].name_first, p);
                    break;
                case 2:
                    strcpy(buffer->players[buffer->player_count].name_last, p);
                    break;
            }
            p=strtok(NULL, ",");
            i++;
        }
        buffer->player_count++;
        
        pthread_cond_signal(&buffer->not_empty);
        pthread_mutex_unlock(&buffer->mutex);
    }

    printf("Finished adding football players to buffer, size %d\n", buffer->player_count);
}

/*
    Function to read tennis players from a CSV file and add them to the shared buffer
*/
void read_tennis_players_in_shared_buffer(SharedBuffer *buffer)
{
    FILE* file = fopen("data/tennis/atp_players.csv", "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", "data/tennis/atp_players.csv");
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Add data to buffer
        pthread_mutex_lock(&buffer->mutex);
        while (buffer->count == buffer->size) { 
            pthread_cond_wait(&buffer->not_full, &buffer->mutex);
        }
        
        char *p=strtok(line, ",");
        int i=0;
        while(p!=NULL)
        {
            switch(i)
            {
                case 0:
                    buffer->players[buffer->player_count].player_id = atoi(p);
                    break;
                case 1:
                    strcpy(buffer->players[buffer->player_count].name_first, p);
                    break;
                case 2:
                    strcpy(buffer->players[buffer->player_count].name_last, p);
                    break;
            }
            p=strtok(NULL, ",");
            i++;
        }
        buffer->player_count++;
        
        pthread_cond_signal(&buffer->not_empty);
        pthread_mutex_unlock(&buffer->mutex);
    }

    printf("Finished adding tennis players to buffer, size %d\n", buffer->player_count);
}

// TODO: Implement the rest for basketball

/*
    Function to generate a phase report based on the data in the shared buffer
    The report is written to a file and contains the player with the max points and the top 10 PPA players
    Also, a callback function is used to print the top 10 PPA players
*/
void generate_phase_report(SharedBuffer* buffer, ReportCallback callback, const char* filename, bool is_football) {
    FILE* report = fopen(filename, "w");
    if (report == NULL) {
        printf("Error opening report file: %s\n", filename);
        return;
    }

    const char* sport = is_football ? "Football" : "Tennis";
    fprintf(report, "%s Results:\n", sport);
    
    // Print max points player info
    Player* max_points_player = is_football ? 
        &buffer->player_with_max_points_football : 
        &buffer->player_with_max_points_tennis;
    
    fprintf(report, "Player with max points: %s %s, points: %d\n",
            max_points_player->name_first,
            max_points_player->name_last,
            max_points_player->points);
    
    // Print top 10 PPA players using callback
    fprintf(report, "\nTop 10 PPA Players:\n");
    callback(buffer, report, is_football);
    
    fclose(report);
}


/*
    Producer thread function
    The producer reads football and tennis players from CSV files and adds them to the shared buffer
    After adding all players, the producer waits for the consumers to finish processing the data
    Then, it generates a report for each phase and signals the completion of the processing
*/
void* producer_thread(void* arg) {
    ProducerArgs* args = (ProducerArgs*)arg;
    SharedBuffer* buffer = args->buffer;
    //int producer_id = args->producer_id;
    FILE *football_report, *tennis_report;

    // Phase 1: Football processing
    football_report = fopen("football_report.txt", "w");
    if (football_report == NULL) {
        printf("Error opening football report file\n");
        return NULL;
    }

    start_hotspot(&buffer->profiler, "football_phase");
    printf("Starting football phase...\n");
    read_football_players_in_shared_buffer(buffer);
    search_csv_files("data/football", buffer);
    end_hotspot(&buffer->profiler, "football_phase");
    
    // Signal end of football data
    pthread_mutex_lock(&buffer->mutex);
    buffer->phase_data_processed = true;
    pthread_cond_broadcast(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);

    // Wait for consumers to finish football processing
    pthread_mutex_lock(&buffer->completion_mutex);
    while (buffer->active_consumers > 0) {
        pthread_cond_wait(&buffer->all_done, &buffer->completion_mutex);
    }
    pthread_mutex_unlock(&buffer->completion_mutex);

    // Print football results
    generate_phase_report(buffer, print_top_ppa_players, "football_report.txt", true);

    // Phase 2: Tennis processing
    tennis_report = fopen("tennis_report.txt", "w");
    if (tennis_report == NULL) {
        printf("Error opening tennis report file\n");
        return NULL;
    }

    // Switch to tennis phase
    pthread_mutex_lock(&buffer->phase_mutex);
    buffer->current_phase = PHASE_TENNIS;
    buffer->phase_data_processed = false;
    buffer->player_count = 0; // Reset player count for tennis
    memset(buffer->players, 0, sizeof(Player) * 66000); // Reset player data
    pthread_cond_broadcast(&buffer->phase_change);
    pthread_mutex_unlock(&buffer->phase_mutex);

    printf("Starting tennis phase...\n");
    start_hotspot(&buffer->profiler, "tennis_phase");
    read_tennis_players_in_shared_buffer(buffer);
    search_csv_files("data/tennis", buffer);
    end_hotspot(&buffer->profiler, "tennis_phase");

    // Signal end of tennis data
    pthread_mutex_lock(&buffer->mutex);
    buffer->phase_data_processed = true;
    pthread_cond_broadcast(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);

    // Wait for consumers to finish tennis processing
    pthread_mutex_lock(&buffer->completion_mutex);
    while (buffer->active_consumers > 0) {
        pthread_cond_wait(&buffer->all_done, &buffer->completion_mutex);
    }
    pthread_mutex_unlock(&buffer->completion_mutex);

    // Print tennis results
    generate_phase_report(buffer, print_top_ppa_players, "tennis_report.txt", false);

    //TO:DO - for basketball, use a different callback function to use the callback more effectively

    // Signal completion
    pthread_mutex_lock(&buffer->phase_mutex);
    buffer->current_phase = PHASE_DONE;
    pthread_cond_broadcast(&buffer->phase_change);
    pthread_mutex_unlock(&buffer->phase_mutex);

    buffer->all_data_processed = true;
    
    free(arg);
    return NULL;
}
