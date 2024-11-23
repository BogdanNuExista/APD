#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/consumer.h"
#include "../include/utils.h"
#include <pthread.h> 

/*
    Consumer thread function
    The consumer reads data from the shared buffer and processes it according to the current phase
*/
void* consumer_thread(void* arg) {
    ConsumerArgs* args = (ConsumerArgs*)arg;
    SharedBuffer* buffer = args->buffer;
    int consumer_id = args->consumer_id;
    ProcessingPhase current_phase = PHASE_FOOTBALL;

    pthread_mutex_lock(&buffer->completion_mutex);
    buffer->active_consumers++;
    pthread_mutex_unlock(&buffer->completion_mutex);

    while (current_phase != PHASE_DONE) {
        char data[1024];
        char filename[1024];
        int should_process = 0;
        
        pthread_mutex_lock(&buffer->mutex);
        
        while (buffer->count == 0 && !buffer->phase_data_processed) {
            pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
        }
        
        // Check if current phase is done
        if (buffer->count == 0 && buffer->phase_data_processed) {
            pthread_mutex_unlock(&buffer->mutex);
            
            // Signal phase completion
            pthread_mutex_lock(&buffer->completion_mutex);
            buffer->active_consumers--;
            if (buffer->active_consumers == 0) {
                pthread_cond_signal(&buffer->all_done);
            }
            pthread_mutex_unlock(&buffer->completion_mutex);
            
            // Wait for next phase
            pthread_mutex_lock(&buffer->phase_mutex);
            while (current_phase == buffer->current_phase && 
                   buffer->current_phase != PHASE_DONE) {
                pthread_cond_wait(&buffer->phase_change, &buffer->phase_mutex);
            }
            current_phase = buffer->current_phase;
            pthread_mutex_unlock(&buffer->phase_mutex);
            
            if (current_phase != PHASE_DONE) {
                pthread_mutex_lock(&buffer->completion_mutex);
                buffer->active_consumers++;
                pthread_mutex_unlock(&buffer->completion_mutex);
            }
            continue;
        }

        // Check if this consumer should process this data
        if (current_phase == PHASE_FOOTBALL) {
            if (strstr(buffer->entries[buffer->out].filename, "data/football") != NULL) {
                // Consumer 0 handles PPA, Consumer 1 handles max points
                should_process = (consumer_id == 0 && strstr(buffer->entries[buffer->out].filename, "atp_rankings") == NULL) ||
                               (consumer_id == 1 && strstr(buffer->entries[buffer->out].filename, "atp_rankings") != NULL);
            }
        } else if (current_phase == PHASE_TENNIS) {
            if (strstr(buffer->entries[buffer->out].filename, "data/tennis") != NULL) {
                // Consumer 0 handles PPA, Consumer 1 handles max points
                should_process = (consumer_id == 0 && strstr(buffer->entries[buffer->out].filename, "atp_rankings") == NULL) ||
                               (consumer_id == 1 && strstr(buffer->entries[buffer->out].filename, "atp_rankings") != NULL);
            }
        }

        // Copy data if this consumer should process it
        if (should_process) {
            strcpy(data, buffer->entries[buffer->out].data);
            strcpy(filename, buffer->entries[buffer->out].filename);
            buffer->out = (buffer->out + 1) % buffer->size;
            buffer->count--;
            
            pthread_cond_signal(&buffer->not_full);
            pthread_mutex_unlock(&buffer->mutex);

            // Process according to current phase and consumer role
            if (current_phase == PHASE_FOOTBALL) {
                if (consumer_id == 0) {
                    calculate_ppa_for_football(buffer, filename, data);
                } else {
                    calculate_max_points_for_football(buffer, filename, data);
                }
            } else if (current_phase == PHASE_TENNIS) {
                if (consumer_id == 0) {
                    calculate_ppa_for_tennis(buffer, filename, data);
                } else {
                    calculate_max_points_for_tennis(buffer, filename, data);
                }
            }
        } else {
            // If this consumer shouldn't process this data, just unlock and continue
            pthread_mutex_unlock(&buffer->mutex);
        }
    }

    printf("COUNT DEBUG: %d\n", buffer->debug_count);

    free(arg);
    return NULL;
}
















//////////////////////////////////////////////////////////// HELPER FUNCTIONS ////////////////////////////////////////////////////////////

void print_buffer_players(SharedBuffer *buffer) {
    for (int i = 5000; i < 6000; i++) {
        printf("Player: %s %s on index %d, PPA: %f\n", buffer->players[i].name_first, buffer->players[i].name_last, i, buffer->players[i].ppa);
    }
}


int find_player_by_name(Player *players, int player_count, char* name) {
    for (int i = 0; i < player_count; i++) {
        char full_name[200];
        sprintf(full_name, "%s %s", players[i].name_first, players[i].name_last);
        if (strcmp(players[i].name_first, name) == 0) {
            return i;
        }
    }
    return -1;
}

int find_player_by_id(Player *players, int player_count, int id) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].player_id == id) {
            return i;
        }
    }
    return -1;
}


// tourney_id,tourney_name,surface,draw_size,tourney_level,tourney_date,match_num,winner_id,winner_seed,winner_entry,
// winner_name,winner_hand,winner_ht,winner_ioc,winner_age,loser_id,loser_seed,loser_entry,loser_name,loser_hand,loser_ht,
// loser_ioc,loser_age,score,best_of,round,minutes,w_ace,w_df,w_svpt,w_1stIn,w_1stWon,w_2ndWon,w_SvGms,w_bpSaved,w_bpFaced,
// l_ace,l_df,l_svpt,l_1stIn,l_1stWon,l_2ndWon,l_SvGms,l_bpSaved,l_bpFaced,winner_rank,winner_rank_points,loser_rank,loser_rank_points

/*
    Function to calculate the PPA for a football match
    The PPA is calculated as the sum of aces, double faults, first serve points won and second serve points won divided 
    by the total serve points. This is calculated for both the winner and loser of the match and the difference is added 
    to the player's PPA.

    We do the same for tennis; 
*/
void calculate_ppa_for_football(SharedBuffer *buffer, char* filename, char* data) {
    if(strstr(filename, "atp_rankings") != NULL) {
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
        // Find the next comma or end of string
        end = strchr(rest, ',');
        if (end == NULL) {
            end = rest + strlen(rest);
        }

        // Create a temporary null-terminated token
        char temp = *end;
        *end = '\0';
        token = rest;

        // Process the token
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

        // Restore the original character and move to the next field
        *end = temp;
        if (*end == '\0') {
            break;
        }
        rest = end + 1;
    }

    // Remove trailing newline characters from names
    winner_name[strcspn(winner_name, "\r\n")] = 0;
    loser_name[strcspn(loser_name, "\r\n")] = 0;

    if(w_svpt == 0 || l_svpt == 0) {
        return;
    }

    //printf("Winner: %s, Loser: %s - w_ace: %d, w_df: %d, w_svpt: %d, w_1stWon: %d, w_2ndWon: %d, l_ace: %d, l_df: %d, l_svpt: %d, l_1stWon: %d, l_2ndWon: %d\n", winner_name, loser_name, w_ace, w_df, w_svpt, w_1stWon, w_2ndWon, l_ace, l_df, l_svpt, l_1stWon, l_2ndWon);

    double wPPA = (double)(w_ace + w_df + w_1stWon + w_2ndWon) / w_svpt;
    double lPPA = (double)(l_ace + l_df + l_1stWon + l_2ndWon) / l_svpt;

    pthread_mutex_lock(&buffer->mutex);

    int find_winner = find_player_by_id(buffer->players, buffer->player_count, winner_id);
    int find_loser = find_player_by_id(buffer->players, buffer->player_count, loser_id);

    if (find_winner != -1 && find_loser != -1) {
        buffer->players[find_winner].ppa += wPPA;
        buffer->players[find_loser].ppa += lPPA;

        //printf("Max ppa for football: %f\n", buffer->players[find_winner].ppa);
        //printf("OMG PLAYER FOUND IN BUFFER\n");
    } else {
        printf("Warning: Player not found. Winner: %s, Loser: %s\n", winner_name, loser_name);
    }

    pthread_mutex_unlock(&buffer->mutex);

    // can use fprintf to write the ppa for each player to a file
    end_hotspot(&buffer->profiler, "football_ppa_calculation");
}

void calculate_ppa_for_tennis(SharedBuffer *buffer, char* filename, char* data) {

    if(strstr(filename, "atp_rankings") != NULL) {
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
        // Find the next comma or end of string
        end = strchr(rest, ',');
        if (end == NULL) {
            end = rest + strlen(rest);
        }

        // Create a temporary null-terminated token
        char temp = *end;
        *end = '\0';
        token = rest;

        // Process the token
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

        // Restore the original character and move to the next field
        *end = temp;
        if (*end == '\0') {
            break;
        }
        rest = end + 1;
    }

    // Remove trailing newline characters from names
    winner_name[strcspn(winner_name, "\r\n")] = 0;
    loser_name[strcspn(loser_name, "\r\n")] = 0;

    //printf("Winner: %s, Loser: %s, File_name %s, data %s", winner_name, loser_name, filename, data);

    if(w_svpt == 0 || l_svpt == 0) {
        return;
    }

    double wPPA = (double)(w_ace - w_df + w_1stWon + w_2ndWon) / w_svpt;
    double lPPA = (double)(l_ace - l_df + l_1stWon + l_2ndWon) / l_svpt;

    pthread_mutex_lock(&buffer->mutex);

    int find_winner = find_player_by_id(buffer->players, buffer->player_count, winner_id);
    int find_loser = find_player_by_id(buffer->players, buffer->player_count, loser_id);

    if (find_winner != -1 && find_loser != -1) {
        buffer->players[find_winner].ppa += wPPA - lPPA;
        buffer->players[find_loser].ppa += lPPA - wPPA;
        //printf("OMG PLAYER FOUND IN BUFFER\n");
        //printf("PPA: %f\n", buffer->players[find_winner].ppa);
    } else {
        printf("Warning: Player not found. Winner: %s, Loser: %s\n", winner_name, loser_name);
    }

    pthread_mutex_unlock(&buffer->mutex);

    end_hotspot(&buffer->profiler, "tennis_ppa_calculation");
}

void calculate_ppa_for_basket()
{
    //TODO
}

/*
    Function to calculate the max points for a football player
    The max points are calculated as the total points divided by the number of tournaments played

    The average points per tournament are calculated for each player and compared to find the player 
    with the highest average
*/
void calculate_max_points_for_football(SharedBuffer *buffer, char *filename, char *data)
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

    pthread_mutex_lock(&buffer->mutex);
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
    pthread_mutex_unlock(&buffer->mutex);

    end_hotspot(&buffer->profiler, "football_points_calculation");
}

void calculate_max_points_for_tennis(SharedBuffer *buffer, char *filename, char *data)
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

    pthread_mutex_lock(&buffer->mutex);
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
    pthread_mutex_unlock(&buffer->mutex);

    end_hotspot(&buffer->profiler, "tennis_points_calculation");
}



//////////////////////////////////////////////////////////// HELPER FUNCTIONS ////////////////////////////////////////////////////////////

