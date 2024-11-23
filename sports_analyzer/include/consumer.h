#ifndef CONSUMER_H
#define CONSUMER_H

#include "utils.h"

typedef struct {
    SharedBuffer* buffer;
    int consumer_id;
} ConsumerArgs;

void* consumer_thread(void* arg);
void calculate_ppa_for_tennis(SharedBuffer *buffer, char* filename, char* data);
void calculate_ppa_for_football(SharedBuffer *buffer, char* filename, char* data);
void calculate_max_points_for_football(SharedBuffer *buffer, char *filename, char *data);
void calculate_max_points_for_tennis(SharedBuffer *buffer, char *filename, char *data);

#endif // CONSUMER_H