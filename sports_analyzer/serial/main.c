// This will be the serial implementation of the sports analyzer project
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include "../include/utils.h"
#include "../include/profiling.h"

#define BUFFER_SIZE 1000
#define MAX_PATH 1024

void read_tennis_players_in_buffer(SharedBuffer *buffer)
{
    FILE *file = fopen("../data/tennis/atp_players.csv", "r");
    if (file == NULL)
    {
        printf("Error opening file: %s\n", "data/tennis/atp_players.csv");
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while(fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;

        char *p = strtok(line, ",");
        int i = 0;
        while (p != NULL)
        {
            switch (i)
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
            p = strtok(NULL, ",");
            i++;
        }
        buffer->player_count++;
    }
}

void read_football_players_in_buffer(SharedBuffer *buffer)
{
    FILE *file = fopen("../data/football/atp_players.csv", "r");
    if (file == NULL)
    {
        printf("Error opening file: %s\n", "data/football/atp_players.csv");
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while(fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;

        char *p = strtok(line, ",");
        int i = 0;
        while (p != NULL)
        {
            switch (i)
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
            p = strtok(NULL, ",");
            i++;
        }
        buffer->player_count++;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int find_player_by_id(Player *players, int player_count, int id) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].player_id == id) {
            return i;
        }
    }
    return -1;
}

void generate_phase_report(SharedBuffer* buffer, const char* filename, bool is_football) {
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

    print_top_ppa_players(buffer, report, is_football);
    
    fclose(report);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void calculate_ppa_for_foorball(SharedBuffer *buffer, const char* filename, char *data)
{
    if(strstr(filename, "atp_players") != NULL) { // already processed players in producer
        return;
    }
    if(strstr(filename, "rankings") != NULL) { // this is for max points, not PPA
        return;
    }
    if(strstr(filename, "football") == NULL) { // accept football data only first
        return;
    }

    start_hotspot(&buffer->profiler, "football_ppa_calculation");

    int field_count = 0;
    int w_ace = 0, w_df = 0, w_svpt = 0, w_1stWon = 0, w_2ndWon = 0;
    int l_ace = 0, l_df = 0, l_svpt = 0, l_1stWon = 0, l_2ndWon = 0;
    char winner_name[100] = {0};
    char loser_name[100] = {0};
    int winner_id = 0, loser_id = 0;

    char* token;
    char* rest = data;
    char* end;

    while (1) {
        end = strchr(rest, ',');
        if (end == NULL) {
            end = rest + strlen(rest);
        }

        char temp = *end;
        *end = '\0';
        token = rest;

        switch (field_count) {
            case 7: winner_id = atoi(token); break;
            case 10: strncpy(winner_name, token, sizeof(winner_name) - 1); break;
            case 15: loser_id = atoi(token); break;
            case 18: strncpy(loser_name, token, sizeof(loser_name) - 1); break;
            case 27: w_ace = atoi(token); break;
            case 28: w_df = atoi(token); break;
            case 29: w_svpt = atoi(token); break;
            case 31: w_1stWon = atoi(token); break;
            case 32: w_2ndWon = atoi(token); break;
            case 33: l_ace = atoi(token); break;
            case 34: l_df = atoi(token); break;
            case 35: l_svpt = atoi(token); break;
            case 37: l_1stWon = atoi(token); break;
            case 38: l_2ndWon = atoi(token); break;
        }

        field_count++;

        *end = temp;
        if (*end == '\0') {
            break;
        }
        rest = end + 1;
    }

    winner_name[strcspn(winner_name, "\r\n")] = 0;
    loser_name[strcspn(loser_name, "\r\n")] = 0;

    if(w_svpt == 0 || l_svpt == 0) {
        return;
    }

    double wPPA = (double)(w_ace + w_df + w_1stWon + w_2ndWon) / w_svpt;
    double lPPA = (double)(l_ace + l_df + l_1stWon + l_2ndWon) / l_svpt;

    int find_winner = find_player_by_id(buffer->players, buffer->player_count, winner_id);
    int find_loser = find_player_by_id(buffer->players, buffer->player_count, loser_id);

    if (find_winner != -1 && find_loser != -1) {
        buffer->players[find_winner].ppa += wPPA;
        buffer->players[find_loser].ppa += lPPA;
    } else {
        printf("Warning: Player not found. Winner: %s, Loser: %s\n", winner_name, loser_name);
    }

    end_hotspot(&buffer->profiler, "football_ppa_calculation");

}

void calculate_ppa_for_tennis(SharedBuffer *buffer, const char* filename, char *data)
{
    if(strstr(filename, "atp_players") != NULL) { // already processed players in producer
        return;
    }
    if(strstr(filename, "rankings") != NULL) { // this is for max points, not PPA
        return;
    }
    if(strstr(filename, "tennis") == NULL) { // accept tennis data only first
        return;
    }

    start_hotspot(&buffer->profiler, "tennis_ppa_calculation");

    int field_count = 0;
    int w_ace = 0, w_df = 0, w_svpt = 0, w_1stWon = 0, w_2ndWon = 0;
    int l_ace = 0, l_df = 0, l_svpt = 0, l_1stWon = 0, l_2ndWon = 0;
    char winner_name[100] = {0};
    char loser_name[100] = {0};
    int winner_id = 0, loser_id = 0;

    char* token;
    char* rest = data;
    char* end;

    while (1) {
        end = strchr(rest, ',');
        if (end == NULL) {
            end = rest + strlen(rest);
        }

        char temp = *end;
        *end = '\0';
        token = rest;

        switch (field_count) {
            case 7: winner_id = atoi(token); break;
            case 10: strncpy(winner_name, token, sizeof(winner_name) - 1); break;
            case 15: loser_id = atoi(token); break;
            case 18: strncpy(loser_name, token, sizeof(loser_name) - 1); break;
            case 27: w_ace = atoi(token); break;
            case 28: w_df = atoi(token); break;
            case 29: w_svpt = atoi(token); break;
            case 31: w_1stWon = atoi(token); break;
            case 32: w_2ndWon = atoi(token); break;
            case 33: l_ace = atoi(token); break;
            case 34: l_df = atoi(token); break;
            case 35: l_svpt = atoi(token); break;
            case 37: l_1stWon = atoi(token); break;
            case 38: l_2ndWon = atoi(token); break;
        }

        field_count++;

        *end = temp;
        if (*end == '\0') {
            break;
        }
        rest = end + 1;
    }

    winner_name[strcspn(winner_name, "\r\n")] = 0;
    loser_name[strcspn(loser_name, "\r\n")] = 0;


    if(w_svpt == 0 || l_svpt == 0) {
        return;
    }

    double wPPA = (double)(w_ace - w_df + w_1stWon + w_2ndWon) / w_svpt;
    double lPPA = (double)(l_ace - l_df + l_1stWon + l_2ndWon) / l_svpt;

    int find_winner = find_player_by_id(buffer->players, buffer->player_count, winner_id);
    int find_loser = find_player_by_id(buffer->players, buffer->player_count, loser_id);

    if (find_winner != -1 && find_loser != -1) {
        buffer->players[find_winner].ppa += wPPA - lPPA;
        buffer->players[find_loser].ppa += lPPA - wPPA;
    } else {
        printf("Warning: Player not found. Winner: %s, Loser: %s\n", winner_name, loser_name);
    }

    end_hotspot(&buffer->profiler, "tennis_ppa_calculation");

}

void calculate_max_points_for_football(SharedBuffer *buffer,const char *filename, char *data)
{
    if(strstr(filename, "atp_rankings") == NULL) {
        return;
    }

    start_hotspot(&buffer->profiler, "football_points_calculation");

    char *p = strtok(data, ",");
    int i = 0;
    int p_id = 0;
    int p_points = 0;
    char tourney_id[11];  // Added to store tournament ID
    while(p)
    {
        switch(i)
        {
            case 0: strncpy(tourney_id, p, sizeof(tourney_id) - 1); tourney_id[sizeof(tourney_id)-1] = '\0'; break;
            case 2: p_id = atoi(p); break;
            case 3: p_points = atoi(p); break;
        }
        p = strtok(NULL, ",");
        i++;
    }

    buffer->debug_count++;
    int find_player = find_player_by_id(buffer->players, buffer->player_count, p_id);
    if (find_player != -1) {
        // Check if tournament is already counted for this player
        bool tourney_exists = false;
        for (int t = 0; t < buffer->players[find_player].tourneyz_count; t++) {
            if (strcmp(buffer->players[find_player].tourneyz[t], tourney_id) == 0) {
                tourney_exists = true;
                break;
            }
        }
        
        // Add new tournament if not exists
        if (!tourney_exists) {
            strcpy(buffer->players[find_player].tourneyz[buffer->players[find_player].tourneyz_count], tourney_id);
            buffer->players[find_player].tourneyz_count++;
        }

        buffer->players[find_player].points += p_points;
        
        // Calculate average points per tournament
        double avg_points = (double)buffer->players[find_player].points / 
                          buffer->players[find_player].tourneyz_count;
        
        // Compare using average points
        if (avg_points > ((double)buffer->player_with_max_points_football.points / 
                         (buffer->player_with_max_points_football.tourneyz_count > 0 ? 
                          buffer->player_with_max_points_football.tourneyz_count : 1))) {
            buffer->player_with_max_points_football = buffer->players[find_player];
        }
    } else {
        printf("Warning: Player not found when calc max points for football. Player ID: %d\n", p_id);
    }

    end_hotspot(&buffer->profiler, "football_points_calculation");

}

void calculate_max_points_for_tennis(SharedBuffer *buffer,const char *filename, char *data)
{
    if(strstr(filename, "atp_rankings") == NULL) {
        return;
    }

    start_hotspot(&buffer->profiler, "tennis_points_calculation");

    char *p = strtok(data, ",");
    int i = 0;
    int p_id = 0;
    int p_points = 0;
    char tourney_id[11];  // Added to store tournament ID
    while(p)
    {
        switch(i)
        {
            case 0: strncpy(tourney_id, p, sizeof(tourney_id) - 1); tourney_id[sizeof(tourney_id)-1] = '\0'; break;
            case 2: p_id = atoi(p); break;
            case 3: p_points = atoi(p); break;
        }
        p = strtok(NULL, ",");
        i++;
    }

    int find_player = find_player_by_id(buffer->players, buffer->player_count, p_id);
    if (find_player != -1) {
        // Check if tournament is already counted for this player
        bool tourney_exists = false;
        for (int t = 0; t < buffer->players[find_player].tourneyz_count; t++) {
            if (strcmp(buffer->players[find_player].tourneyz[t], tourney_id) == 0) {
                tourney_exists = true;
                break;
            }
        }
        
        // Add new tournament if not exists
        if (!tourney_exists) {
            strcpy(buffer->players[find_player].tourneyz[buffer->players[find_player].tourneyz_count], tourney_id);
            buffer->players[find_player].tourneyz_count++;
        }

        buffer->players[find_player].points += p_points;
        
        // Calculate average points per tournament
        double avg_points = (double)buffer->players[find_player].points / 
                          buffer->players[find_player].tourneyz_count;
        
        // Compare using average points
        if (avg_points > ((double)buffer->player_with_max_points_tennis.points / 
                         (buffer->player_with_max_points_tennis.tourneyz_count > 0 ? 
                          buffer->player_with_max_points_tennis.tourneyz_count : 1))) {
            buffer->player_with_max_points_tennis = buffer->players[find_player];
        }
    } else {
        printf("Warning: Player not found when calc max points for tennis. Player ID: %d\n", p_id);
    }

    end_hotspot(&buffer->profiler, "tennis_points_calculation");

}


void process_csv_file(const char* file_path, SharedBuffer* buffer) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_path);
        return;
    }

    bool is_football=false, is_tennis=false;
    if(strstr(file_path, "football") != NULL) {
        is_football = true;
    } else if(strstr(file_path, "tennis") != NULL) {
        is_tennis = true;
    }

    char line[1024];
    fgets(line, sizeof(line), file); // Skip header
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
    
        if(is_football) {
            if(strstr(file_path, "atp_rankings") != NULL) {
                calculate_max_points_for_football(buffer, file_path, line);
            } else {
                calculate_ppa_for_foorball(buffer, file_path, line);
            }
        } else if(is_tennis) {
            if(strstr(file_path, "atp_rankings") != NULL) {
                calculate_max_points_for_tennis(buffer, file_path, line);
            } else {
            calculate_ppa_for_tennis(buffer, file_path, line);
            }
        }
        
    }
    printf("Finished reading file: %s\n", file_path);
    fclose(file);
}

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

void solve()
{
    struct timespec start, finish;
    double elapsed;

    printf("\nMeasuring Wall-clock time \n");
    printf("Start...\n");
    
    // Start the timer
    clock_gettime(CLOCK_MONOTONIC, &start);

    SharedBuffer buffer;
    init_buffer(&buffer, BUFFER_SIZE);
    init_profiler(&buffer.profiler);

    read_football_players_in_buffer(&buffer);
    printf("Finished adding football players to buffer, size %d\n", buffer.player_count);
    search_csv_files("../data/football", &buffer);

    generate_phase_report(&buffer, "football_report.txt", true);

    memset(buffer.players, 0, sizeof(Player) * 66000);
    buffer.player_count = 0;

    read_tennis_players_in_buffer(&buffer);
    printf("Finished adding tennis players to buffer, size %d\n", buffer.player_count);
    search_csv_files("../data/tennis", &buffer);

    generate_phase_report(&buffer, "tennis_report.txt", false);

    printf("DEBUG COUNT: %d\n", buffer.debug_count);

    destroy_buffer(&buffer);

    // Stop the timer
    clock_gettime(CLOCK_MONOTONIC, &finish);

    // Calculate the elapsed time
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Serial time = %lf seconds\n", elapsed);

    calculate_metrics(&buffer.profiler);
    log_profile_data(&buffer.profiler); 
}

int main(void)
{
    solve();

    return 0;
}