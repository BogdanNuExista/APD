// jones_family.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "barrier.h"

#define NUM_FAMILY_MEMBERS 6
#define NUM_DAYS 5

// Barriers for different synchronization points
my_barrier_t breakfast_barrier;
my_barrier_t kids_math_barrier;
my_barrier_t kids_football_barrier;
my_barrier_t dinner_barrier;

// Structure to hold family member information
typedef struct {
    const char *name;
    int is_parent;
    int is_math_student;
    int is_football_player;
} family_member_t;

// Family member definitions
family_member_t family_members[NUM_FAMILY_MEMBERS] = {
    {"Mama Jones", 1, 0, 0},
    {"Papa Jones", 1, 0, 0},
    {"Alice", 0, 1, 0},
    {"Bob", 0, 1, 0},
    {"Chris", 0, 0, 1},
    {"Dave", 0, 0, 1}
};

void simulate_activity(const char *name, const char *activity, int duration) {
    printf("%s %s\n", name, activity);
    sleep(duration);  // Simulate activity duration
}

void* family_member_routine(void *arg) {
    family_member_t *member = (family_member_t*)arg;
    
    for (int day = 1; day <= NUM_DAYS; day++) {
        printf("\n%s starting day %d\n", member->name, day);
        
        // Morning routine
        simulate_activity(member->name, "wakes up", 1);
        printf("%s is ready for breakfast\n", member->name);
        my_barrier_wait(&breakfast_barrier);
        simulate_activity(member->name, "has breakfast with family", 2);
        
        // Day activities
        if (member->is_parent) {
            simulate_activity(member->name, "goes to work", 3);
            simulate_activity(member->name, "works", 5);
        } else {
            simulate_activity(member->name, "goes to school", 2);
            simulate_activity(member->name, "studies in class", 4);
            simulate_activity(member->name, "walks home", 2);
            
            // After-school activities
            if (member->is_math_student) {
                printf("%s is ready to learn math\n", member->name);
                my_barrier_wait(&kids_math_barrier);
                simulate_activity(member->name, "studies math with sibling", 3);
            }
            
            if (member->is_football_player) {
                printf("%s is ready to play football\n", member->name);
                my_barrier_wait(&kids_football_barrier);
                simulate_activity(member->name, "plays football with sibling", 3);
            }
        }
        
        // Evening routine
        printf("%s is ready for dinner\n", member->name);
        my_barrier_wait(&dinner_barrier);
        simulate_activity(member->name, "has dinner with family", 2);
        simulate_activity(member->name, "goes to sleep", 1);
    }
    
    return NULL;
}

int main() {
    pthread_t threads[NUM_FAMILY_MEMBERS];
    
    // Initialize barriers
    my_barrier_init(&breakfast_barrier, NUM_FAMILY_MEMBERS);
    my_barrier_init(&kids_math_barrier, 2);  // Alice and Bob
    my_barrier_init(&kids_football_barrier, 2);  // Chris and Dave
    my_barrier_init(&dinner_barrier, NUM_FAMILY_MEMBERS);
    
    // Create threads for each family member
    for (int i = 0; i < NUM_FAMILY_MEMBERS; i++) {
        pthread_create(&threads[i], NULL, family_member_routine, &family_members[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_FAMILY_MEMBERS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Cleanup
    my_barrier_destroy(&breakfast_barrier);
    my_barrier_destroy(&kids_math_barrier);
    my_barrier_destroy(&kids_football_barrier);
    my_barrier_destroy(&dinner_barrier);
    
    return 0;
}