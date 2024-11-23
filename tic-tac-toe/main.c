#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include "main.h"

int main() {
    int choice, size, depth, numThreads;
    bool isParallel, debugMode;

    printf("Generalized Tic-tac-toe Game\n");
    printf("1. Player vs Computer\n");
    printf("2. Computer vs Computer\n");
    printf("3. Run Performance Tests\n");
    printf("Enter choice (1-3): ");
    scanf("%d", &choice);

    if (choice == 3) {
        runPerformanceTests();
        return 0;
    }

    printf("Enter board size (3-%d): ", MAX_BOARD_SIZE);
    scanf("%d", &size);
    if (size < MIN_BOARD_SIZE || size > MAX_BOARD_SIZE) {
        printf("Invalid board size!\n");
        return 1;
    }

    printf("Enter search depth (1-%d): ", MAX_DEPTH);
    scanf("%d", &depth);
    if (depth < 1 || depth > MAX_DEPTH) {
        printf("Invalid depth!\n");
        return 1;
    }

    printf("Use parallelization? (0/1): ");
    int isPar;
    scanf("%d", &isPar);
    isParallel = isPar == 1;

    if (isParallel) {
        printf("Enter number of threads (2,4,8): ");
        scanf("%d", &numThreads);
        if (numThreads != 2 && numThreads != 4 && numThreads != 8) {
            printf("Invalid number of threads!\n");
            return 1;
        }
    } else {
        numThreads = 1;
    }

    if (choice == 1) {
        playerVsComputer(size, depth, isParallel, numThreads);
    } else if (choice == 2) {
        computerVsComputer(size, depth, isParallel, numThreads);
    }

    return 0;
}
