#include <stdlib.h>
#include "../include/utils.h"
#include <string.h>
#include <stdio.h>


void init_buffer(SharedBuffer* buffer, int size) {
    
    buffer->entries = (BufferEntry*)malloc(size * sizeof(BufferEntry));

    buffer->players = (Player*)malloc(66000 * sizeof(Player));

    memset(&buffer->player_with_max_points_tennis, 0, sizeof(Player));
    memset(&buffer->player_with_max_points_football, 0, sizeof(Player));

    buffer->size = size;
    buffer->in = 0;
    buffer->out = 0;
    buffer->count = 0;
    buffer->player_count = 0; 
    buffer->all_data_processed = false;
    buffer->active_consumers = 0;
    buffer->debug_count = 0;

    buffer->current_phase = PHASE_FOOTBALL;
    buffer->phase_data_processed = false;
    pthread_mutex_init(&buffer->phase_mutex, NULL);
    pthread_cond_init(&buffer->phase_change, NULL);
    pthread_mutex_destroy(&buffer->phase_mutex);
    pthread_cond_destroy(&buffer->phase_change);

    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_mutex_init(&buffer->completion_mutex, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);

    pthread_cond_init(&buffer->all_done, NULL);

}

void destroy_buffer(SharedBuffer* buffer) {
    
    free(buffer->entries);
    free(buffer->players);
    pthread_mutex_destroy(&buffer->mutex);
    pthread_mutex_destroy(&buffer->completion_mutex);
    pthread_cond_destroy(&buffer->not_full);
    pthread_cond_destroy(&buffer->not_empty);
    
    pthread_cond_destroy(&buffer->all_done);
}



void print_top_ppa_players(SharedBuffer* buffer, FILE* file, bool is_football) {
    (void)is_football;
    // Create temporary array of player pointers for sorting
    Player** players = malloc(buffer->player_count * sizeof(Player*));
    int valid_count = 0;
    
    for (int i = 0; i < buffer->player_count; i++) {
        if (buffer->players[i].ppa != 0 && buffer->players[i].tourneyz_count > 0) {
            // Calculate average PPA per tournament
            buffer->players[i].ppa /= buffer->players[i].tourneyz_count;
            players[valid_count] = &buffer->players[i];
            valid_count++;
        }
    }
    
    // Sort players by average PPA
    for (int i = 0; i < valid_count - 1; i++) {
        for (int j = 0; j < valid_count - i - 1; j++) {
            if (players[j]->ppa < players[j + 1]->ppa) {
                Player* temp = players[j];
                players[j] = players[j + 1];
                players[j + 1] = temp;
            }
        }
    }
    
    // Print top 10 (or less if fewer players)
    int limit = valid_count < 10 ? valid_count : 10;
    for (int i = 0; i < limit; i++) {
        fprintf(file, "%d. %s %s - Average PPA: %.4f (across %d tournaments)\n", 
                i + 1, 
                players[i]->name_first,
                players[i]->name_last,
                players[i]->ppa,
                players[i]->tourneyz_count);
    }
    
    free(players);
}