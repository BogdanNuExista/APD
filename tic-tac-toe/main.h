#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <omp.h>

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

// Function declarations
GameBoard* createBoard(int size);
void freeBoard(GameBoard* board);
void printBoard(GameBoard* board);
bool isMovesLeft(GameBoard* board);
bool isValidMove(GameBoard* board, int row, int col);
bool checkWin(GameBoard* board, char player);
Move getBestMove(GameBoard* board, int depth, bool isParallel, int numThreads);
int minimax(GameBoard* board, int depth, bool isMax, int alpha, int beta, bool isParallel);
void playerMove(GameBoard* board);
void computerMove(GameBoard* board, int depth, bool isParallel, int numThreads, double* moveTime);
void computerVsComputer(int size, int depth, bool isParallel, int numThreads);
void playerVsComputer(int size, int depth, bool isParallel, int numThreads);


typedef struct {
    double* moveTimes;        // Array to store individual move times
    int moveCount;            // Number of moves recorded
    int capacity;             // Capacity of moveTimes array
    double totalTime;         // Total time for all moves
    double averageTime;       // Average move time
    double speedup;           // Speedup compared to serial version (for parallel runs)
} PerformanceStats;

// Function declarations
PerformanceStats* createStats(int initialCapacity);
void freeStats(PerformanceStats* stats);
void recordMoveTime(PerformanceStats* stats, double moveTime);
void calculateAverageTime(PerformanceStats* stats);
void printPerformanceReport(PerformanceStats* stats, int size, int depth, int threads);
void runPerformanceTests(void);


#endif