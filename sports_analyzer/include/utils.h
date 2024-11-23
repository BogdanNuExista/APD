#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include "profiling.h"

#define MAX_TOURNEYZ 2000

typedef struct {
    char data[1024];
    char filename[1024];
} BufferEntry; 

typedef enum {
    PHASE_FOOTBALL,
    PHASE_TENNIS,
    PHASE_DONE
} ProcessingPhase;

typedef struct {
    int player_id;
    char name_first[100];
    char name_last[100];
    int points; // max points este calculat ca numarul total de puncte impartit la numarul de turnee
    int w_ace, w_df, w_svpt, w_1stWon, w_2ndWon; // for wPPA = (w_ace - w_df + w_1stWon + w_2ndWon) / w_svpt
    int l_ace, l_df, l_svpt, l_1stWon, l_2ndWon; // for lPPA = (l_ace - l_df + l_1stWon + l_2ndWon) / l_svpt
    double ppa; // PPA = wPPA - lPPA -> ppa formula
    char tourneyz[MAX_TOURNEYZ][20];
    int tourneyz_count;
} Player;

typedef struct {
    BufferEntry* entries;
    int size;
    int in;
    int out;
    int count;
    char filename[1024];
    Player *players;
    Player player_with_max_points_tennis;
    Player player_with_max_points_football;
    ProfilerData profiler;
    int player_count;

    int debug_count;

    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    pthread_cond_t done_reading;

    ProcessingPhase current_phase;
    bool phase_data_processed;
    pthread_mutex_t phase_mutex;
    pthread_cond_t phase_change;

    bool all_data_processed;
    int active_consumers;
    pthread_mutex_t completion_mutex;
    pthread_cond_t all_done;
} SharedBuffer;

typedef void (*ReportCallback)(SharedBuffer*, FILE*, bool);

typedef struct {
    SharedBuffer* buffer;
    ReportCallback callback;
    FILE* file;
    bool is_football; 
} ReportContext;

void init_buffer(SharedBuffer* buffer, int size);
void destroy_buffer(SharedBuffer* buffer);
void print_top_ppa_players(SharedBuffer* buffer, FILE* file, bool is_football);

#endif // UTILS_H