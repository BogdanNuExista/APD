#ifndef SERIAL_H
#define SERIAL_H

#define MAX_BOARD_SIZE 20
#define MIN_BOARD_SIZE 3
#define MAX_DEPTH 10

typedef struct {
    char** board;
    int size;
} GameBoard;

typedef struct {
    int row;
    int col;
    int score;
} Move;

typedef struct {
    double* moveTimes;        // Array to store individual move times
    int moveCount;            // Number of moves recorded
    int capacity;             // Capacity of moveTimes array
    double totalTime;         // Total time for all moves
    double averageTime;       // Average move time
    double speedup;           // Speedup compared to serial version (for parallel runs)
} PerformanceStats;

#endif // SERIAL_H