// profiling.c
#include "profiling.h"
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "utils.h"

void init_profiler(ProfilerData* profiler) {
    gettimeofday(&profiler->start_time, NULL);
    getrusage(RUSAGE_SELF, &profiler->last_usage);
    clock_gettime(CLOCK_MONOTONIC, &profiler->wall_start);
    
    profiler->cpu_usage = 0.0;
    profiler->memory_usage = 0;
    profiler->sample_count = 0;
    profiler->wall_elapsed = 0.0;
    
    // Initialize hotspot tracking
    for (int i = 0; i < MAX_HOTSPOTS; i++) {
        profiler->hotspots[i].count = 0;
        profiler->hotspots[i].total_time = 0.0;
        memset(profiler->hotspots[i].name, 0, MAX_NAME_LENGTH);
    }
    
    pthread_mutex_init(&profiler->profile_mutex, NULL);
}

void start_hotspot(ProfilerData* profiler, const char* name) {
    pthread_mutex_lock(&profiler->profile_mutex);
    
    // Find or create hotspot
    int index = -1;
    for (int i = 0; i < MAX_HOTSPOTS; i++) {
        if (strcmp(profiler->hotspots[i].name, name) == 0) {
            index = i;
            break;
        }
        if (profiler->hotspots[i].name[0] == '\0') {
            index = i;
            strncpy(profiler->hotspots[i].name, name, MAX_NAME_LENGTH - 1);
            break;
        }
    }
    
    if (index != -1) {
        clock_gettime(CLOCK_MONOTONIC, &profiler->hotspots[index].start);
    }
    
    pthread_mutex_unlock(&profiler->profile_mutex);
}

void end_hotspot(ProfilerData* profiler, const char* name) {
    pthread_mutex_lock(&profiler->profile_mutex);
    
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    for (int i = 0; i < MAX_HOTSPOTS; i++) {
        if (strcmp(profiler->hotspots[i].name, name) == 0) {
            double elapsed = (end.tv_sec - profiler->hotspots[i].start.tv_sec) + 
                             (end.tv_nsec - profiler->hotspots[i].start.tv_nsec) / 1000000000.0;
            
            profiler->hotspots[i].total_time += elapsed;
            profiler->hotspots[i].count++;
            break;
        }
    }
    
    pthread_mutex_unlock(&profiler->profile_mutex);
}

void calculate_metrics(ProfilerData* profiler) {
    struct rusage current_usage;
    struct timeval current_time;
    struct timespec wall_end;
    
    getrusage(RUSAGE_SELF, &current_usage);
    gettimeofday(&current_time, NULL);
    clock_gettime(CLOCK_MONOTONIC, &wall_end);

    // Calculate CPU usage
    double user_time = (current_usage.ru_utime.tv_sec - profiler->last_usage.ru_utime.tv_sec) +
                      (current_usage.ru_utime.tv_usec - profiler->last_usage.ru_utime.tv_usec) / 1000000.0;
    double sys_time = (current_usage.ru_stime.tv_sec - profiler->last_usage.ru_stime.tv_sec) +
                     (current_usage.ru_stime.tv_usec - profiler->last_usage.ru_stime.tv_usec) / 1000000.0;
    double elapsed = (current_time.tv_sec - profiler->start_time.tv_sec) +
                    (current_time.tv_usec - profiler->start_time.tv_usec) / 1000000.0;

    // Calculate wall clock time
    profiler->wall_elapsed = (wall_end.tv_sec - profiler->wall_start.tv_sec) + 
                             (wall_end.tv_nsec - profiler->wall_start.tv_nsec) / 1000000000.0;

    pthread_mutex_lock(&profiler->profile_mutex);
    profiler->cpu_usage = ((user_time + sys_time) / elapsed) * 100.0;
    profiler->memory_usage = current_usage.ru_maxrss;
    profiler->sample_count++;
    pthread_mutex_unlock(&profiler->profile_mutex);

    profiler->last_usage = current_usage;
}

void log_profile_data(ProfilerData* profiler) {
    pthread_mutex_lock(&profiler->profile_mutex);
    FILE* log_file = fopen("performance_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "Sample %d:\n", profiler->sample_count);
        fprintf(log_file, "CPU Usage: %.2f%%\n", profiler->cpu_usage);
        fprintf(log_file, "Memory Usage: %zu KB\n", profiler->memory_usage);
        fprintf(log_file, "Wall Clock Time: %.6f seconds\n", profiler->wall_elapsed);
        
        // Log hotspots
        fprintf(log_file, "Hotspots:\n");
        for (int i = 0; i < MAX_HOTSPOTS; i++) {
            if (profiler->hotspots[i].name[0] != '\0') {
                fprintf(log_file, "  %s: Total Time=%.6f, Calls=%d, Avg Time=%.6f\n", 
                    profiler->hotspots[i].name, 
                    profiler->hotspots[i].total_time,
                    profiler->hotspots[i].count,
                    profiler->hotspots[i].total_time / (profiler->hotspots[i].count ? profiler->hotspots[i].count : 1)
                );
            }
        }
        fprintf(log_file, "------------------------\n");
        fclose(log_file);
    }
    pthread_mutex_unlock(&profiler->profile_mutex);
}

void* profiling_thread(void* arg) {
    SharedBuffer* buffer = (SharedBuffer*)arg;
    ProfilerData* profiler = &buffer->profiler;  // Use the shared profiler directly

    while (!buffer->all_data_processed) {
        calculate_metrics(profiler);
        log_profile_data(profiler);
        usleep(10000000); // Sample every 10 seconds
    }

    // Final metrics
    calculate_metrics(profiler);
    log_profile_data(profiler);

    pthread_mutex_destroy(&profiler->profile_mutex);
    return NULL;
}