// profiling.h
#ifndef PROFILING_H
#define PROFILING_H

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#define MAX_HOTSPOTS 10
#define MAX_NAME_LENGTH 50

typedef struct {
    char name[MAX_NAME_LENGTH];
    struct timespec start;
    double total_time;
    int count;
} HotspotData;

typedef struct {
    struct timeval start_time;
    struct timespec wall_start;
    struct rusage last_usage;
    double cpu_usage;
    size_t memory_usage;
    int sample_count;
    double wall_elapsed;
    
    // Hotspot tracking
    HotspotData hotspots[MAX_HOTSPOTS];
    
    pthread_mutex_t profile_mutex;
} ProfilerData;

// Existing function declarations
void init_profiler(ProfilerData* profiler);
void* profiling_thread(void* arg);
void log_profile_data(ProfilerData* profiler);
void calculate_metrics(ProfilerData* profiler);

// New hotspot tracking functions
void start_hotspot(ProfilerData* profiler, const char* name);
void end_hotspot(ProfilerData* profiler, const char* name);

#endif