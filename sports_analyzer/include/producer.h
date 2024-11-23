#ifndef PRODUCER_H
#define PRODUCER_H

#include "utils.h"

typedef struct {
    SharedBuffer* buffer;
    int producer_id;
} ProducerArgs;

void* producer_thread(void* arg);

#endif // PRODUCER_H