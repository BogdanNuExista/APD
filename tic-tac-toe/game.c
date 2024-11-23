#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include "main.h"

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

GameBoard* createBoard(int size) {
    GameBoard* board = (GameBoard*)malloc(sizeof(GameBoard));
    board->size = size;
    board->board = (char**)malloc(size * sizeof(char*));
    for (int i = 0; i < size; i++) {
        board->board[i] = (char*)malloc(size * sizeof(char));
        for (int j = 0; j < size; j++) {
            board->board[i][j] = ' ';
        }
    }
    return board;
}

void freeBoard(GameBoard* board) {
    for (int i = 0; i < board->size; i++) {
        free(board->board[i]);
    }
    free(board->board);
    free(board);
}

void printBoard(GameBoard* board) {
    printf("\n");
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            printf(" %c ", board->board[i][j]);
            if (j < board->size - 1) printf("|");
        }
        printf("\n");
        if (i < board->size - 1) {
            for (int j = 0; j < board->size; j++) {
                printf("---");
                if (j < board->size - 1) printf("+");
            }
            printf("\n");
        }
    }
    printf("\n");
}

bool isMovesLeft(GameBoard* board) {
    for (int i = 0; i < board->size; i++)
        for (int j = 0; j < board->size; j++)
            if (board->board[i][j] == ' ')
                return true;
    return false;
}

bool isValidMove(GameBoard* board, int row, int col) {
    return (row >= 0 && row < board->size &&
            col >= 0 && col < board->size &&
            board->board[row][col] == ' ');
}

bool checkWin(GameBoard* board, char player) {
    // Check rows
    for (int i = 0; i < board->size; i++) {
        bool win = true;
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] != player) {
                win = false;
                break;
            }
        }
        if (win) return true;
    }

    // Check columns
    for (int j = 0; j < board->size; j++) {
        bool win = true;
        for (int i = 0; i < board->size; i++) {
            if (board->board[i][j] != player) {
                win = false;
                break;
            }
        }
        if (win) return true;
    }

    // Check diagonals
    bool win = true;
    for (int i = 0; i < board->size; i++) {
        if (board->board[i][i] != player) {
            win = false;
            break;
        }
    }
    if (win) return true;

    win = true;
    for (int i = 0; i < board->size; i++) {
        if (board->board[i][board->size - 1 - i] != player) {
            win = false;
            break;
        }
    }
    return win;
}

int minimax(GameBoard* board, int depth, bool isMax, int alpha, int beta, bool isParallel) {
    if (checkWin(board, 'X')) return -10;
    if (checkWin(board, 'O')) return 10;
    if (!isMovesLeft(board) || depth == 0) return 0;

    if (isMax) {
        int best = -1000;
        #pragma omp parallel for if(isParallel) reduction(max:best)
        for (int i = 0; i < board->size; i++) {
            for (int j = 0; j < board->size; j++) {
                if (board->board[i][j] == ' ') {
                    board->board[i][j] = 'O';
                    best = max(best, minimax(board, depth - 1, !isMax, alpha, beta, isParallel));
                    board->board[i][j] = ' ';
                    alpha = max(alpha, best);
                    if (beta <= alpha)
                        break;
                }
            }
        }
        return best;
    } else {
        int best = 1000;
        #pragma omp parallel for if(isParallel) reduction(min:best)
        for (int i = 0; i < board->size; i++) {
            for (int j = 0; j < board->size; j++) {
                if (board->board[i][j] == ' ') {
                    board->board[i][j] = 'X';
                    best = min(best, minimax(board, depth - 1, !isMax, alpha, beta, isParallel));
                    board->board[i][j] = ' ';
                    beta = min(beta, best);
                    if (beta <= alpha)
                        break;
                }
            }
        }
        return best;
    }
}

Move getBestMove(GameBoard* board, int depth, bool isParallel, int numThreads) {
    Move bestMove;
    bestMove.row = -1;
    bestMove.col = -1;
    int bestVal = -1000;

    if (isParallel) {
        omp_set_num_threads(numThreads);
    }

    #pragma omp parallel for collapse(2) if(isParallel)
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] == ' ') {
                board->board[i][j] = 'O';
                int moveVal = minimax(board, depth, false, -1000, 1000, isParallel);
                board->board[i][j] = ' ';

                #pragma omp critical
                {
                    if (moveVal > bestVal) {
                        bestMove.row = i;
                        bestMove.col = j;
                        bestVal = moveVal;
                    }
                }
            }
        }
    }
    return bestMove;
}

void playerMove(GameBoard* board) {
    int row, col;
    do {
        printf("Enter row (0-%d) and column (0-%d): ", board->size - 1, board->size - 1);
        scanf("%d %d", &row, &col);
    } while (!isValidMove(board, row, col));
    board->board[row][col] = 'X';
}

void computerMove(GameBoard* board, int depth, bool isParallel, int numThreads, double* moveTime) {
    double startTime = omp_get_wtime();
    Move bestMove = getBestMove(board, depth, isParallel, numThreads);
    double endTime = omp_get_wtime();
    *moveTime = endTime - startTime;
    
    board->board[bestMove.row][bestMove.col] = 'O';
}

void playerVsComputer(int size, int depth, bool isParallel, int numThreads) {
    GameBoard* board = createBoard(size);
    PerformanceStats* stats = createStats(50);
    double moveTime;
    
    printf("\nGame starts! You are X, Computer is O\n");
    
    while (true) {
        // Always show board before player's move
        printBoard(board);
        printf("Your turn!\n");
        playerMove(board);
        
        if (checkWin(board, 'X')) {
            printBoard(board);
            printf("Player wins!\n");
            break;
        }
        
        if (!isMovesLeft(board)) {
            printBoard(board);
            printf("Draw!\n");
            break;
        }
        
        printf("\nComputer's turn...\n");
        computerMove(board, depth, isParallel, numThreads, &moveTime);
        recordMoveTime(stats, moveTime);
        printf("Computer's move took: %.4f seconds\n", moveTime);
        printBoard(board);
        
        if (checkWin(board, 'O')) {
            printf("Computer wins!\n");
            break;
        }
        
        if (!isMovesLeft(board)) {
            printf("Draw!\n");
            break;
        }
    }
    
    // Calculate and display performance statistics
    calculateAverageTime(stats);
    printf("\nPerformance Statistics:\n");
    printf("------------------------\n");
    printf("Board Size: %dx%d\n", size, size);
    printf("Search Depth: %d\n", depth);
    printf("Number of Threads: %d\n", numThreads);
    printf("Parallelization: %s\n", isParallel ? "Enabled" : "Disabled");
    printf("Total Moves by Computer: %d\n", stats->moveCount);
    printf("Average Move Time: %.4f seconds\n", stats->averageTime);
    printf("Total Computer Time: %.4f seconds\n", stats->totalTime);
    
    freeStats(stats);
    freeBoard(board);
}

// For Computer X's moves
Move getBestMoveX(GameBoard* board, int depth, bool isParallel, int numThreads) {
    Move bestMove;
    bestMove.row = -1;
    bestMove.col = -1;
    int bestVal = 1000;  // Start with a high value since X is minimizing

    if (isParallel) {
        omp_set_num_threads(numThreads);
    }

    #pragma omp parallel for collapse(2) if(isParallel)
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] == ' ') {
                board->board[i][j] = 'X';
                int moveVal = minimax(board, depth, true, -1000, 1000, isParallel);
                board->board[i][j] = ' ';

                #pragma omp critical
                {
                    if (moveVal < bestVal) {  // Looking for minimum value
                        bestMove.row = i;
                        bestMove.col = j;
                        bestVal = moveVal;
                    }
                }
            }
        }
    }
    return bestMove;
}

// Renamed original getBestMove to getBestMoveO for clarity
Move getBestMoveO(GameBoard* board, int depth, bool isParallel, int numThreads) {
    Move bestMove;
    bestMove.row = -1;
    bestMove.col = -1;
    int bestVal = -1000;

    if (isParallel) {
        omp_set_num_threads(numThreads);
    }

    #pragma omp parallel for collapse(2) if(isParallel)
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] == ' ') {
                board->board[i][j] = 'O';
                int moveVal = minimax(board, depth, false, -1000, 1000, isParallel);
                board->board[i][j] = ' ';

                #pragma omp critical
                {
                    if (moveVal > bestVal) {
                        bestMove.row = i;
                        bestMove.col = j;
                        bestVal = moveVal;
                    }
                }
            }
        }
    }
    return bestMove;
}

void computerMoveX(GameBoard* board, int depth, bool isParallel, int numThreads, double* moveTime) {
    double startTime = omp_get_wtime();
    Move bestMove = getBestMoveX(board, depth, isParallel, numThreads);
    double endTime = omp_get_wtime();
    *moveTime = endTime - startTime;
    
    board->board[bestMove.row][bestMove.col] = 'X';
}

void computerMoveO(GameBoard* board, int depth, bool isParallel, int numThreads, double* moveTime) {
    double startTime = omp_get_wtime();
    Move bestMove = getBestMoveO(board, depth, isParallel, numThreads);
    double endTime = omp_get_wtime();
    *moveTime = endTime - startTime;
    
    board->board[bestMove.row][bestMove.col] = 'O';
}

void computerVsComputer(int size, int depth, bool isParallel, int numThreads) {
    GameBoard* board = createBoard(size);
    PerformanceStats* stats = createStats(50);
    int moves = 0;
    
    printf("\nComputer vs Computer game starts!\n");
    printf("Computer X starts in the middle\n");
    
    // First computer always starts in the middle
    int mid = size / 2;
    board->board[mid][mid] = 'X';
    moves++;
    
    // Always show initial board state
    printBoard(board);
    
    while (true) {
        printf("\nComputer O's turn...\n");
        double moveTime;
        computerMoveO(board, depth, isParallel, numThreads, &moveTime);
        recordMoveTime(stats, moveTime);
        moves++;
        
        printf("Move took: %.4f seconds\n", moveTime);
        printBoard(board);
        
        if (checkWin(board, 'O')) {
            printf("Computer O wins!\n");
            break;
        }
        
        if (!isMovesLeft(board)) {
            printf("Draw!\n");
            break;
        }
        
        printf("\nComputer X's turn...\n");
        computerMoveX(board, depth, isParallel, numThreads, &moveTime);
        recordMoveTime(stats, moveTime);
        moves++;
        
        printf("Move took: %.4f seconds\n", moveTime);
        printBoard(board);
        
        if (checkWin(board, 'X')) {
            printf("Computer X wins!\n");
            break;
        }
        
        if (!isMovesLeft(board)) {
            printf("Draw!\n");
            break;
        }
    }
    
    // Performance statistics code remains the same...
    calculateAverageTime(stats);
    printf("\nPerformance Statistics:\n");
    printf("------------------------\n");
    printf("Board Size: %dx%d\n", size, size);
    printf("Search Depth: %d\n", depth);
    printf("Number of Threads: %d\n", numThreads);
    printf("Parallelization: %s\n", isParallel ? "Enabled" : "Disabled");
    printf("Total Moves: %d\n", moves);
    printf("Total Computer Moves: %d\n", stats->moveCount);
    printf("Average Move Time: %.4f seconds\n", stats->averageTime);
    printf("Total Game Time: %.4f seconds\n", stats->totalTime);
    
    // if (isParallel) {
    //     printf("\nRunning quick serial test for speedup comparison...\n");
    //     PerformanceStats* serialStats = createStats(10);
    //     GameBoard* serialBoard = createBoard(size);
        
    //     for (int i = 0; i < 3 && isMovesLeft(serialBoard); i++) {
    //         double serialMoveTime;
    //         computerMoveO(serialBoard, depth, false, 1, &serialMoveTime);
    //         recordMoveTime(serialStats, serialMoveTime);
    //     }
        
    //     calculateAverageTime(serialStats);
    //     double speedup = serialStats->averageTime / stats->averageTime;
    //     printf("Speedup achieved with %d threads: %.2fx\n", numThreads, speedup);
        
    //     freeStats(serialStats);
    //     freeBoard(serialBoard);
    // }
    
    freeStats(stats);
    freeBoard(board);
}

PerformanceStats* createStats(int initialCapacity) {
    PerformanceStats* stats = (PerformanceStats*)malloc(sizeof(PerformanceStats));
    stats->moveTimes = (double*)malloc(initialCapacity * sizeof(double));
    stats->moveCount = 0;
    stats->capacity = initialCapacity;
    stats->totalTime = 0.0;
    stats->averageTime = 0.0;
    stats->speedup = 0.0;
    return stats;
}

void freeStats(PerformanceStats* stats) {
    free(stats->moveTimes);
    free(stats);
}

void recordMoveTime(PerformanceStats* stats, double moveTime) {
    if (stats->moveCount >= stats->capacity) {
        stats->capacity *= 2;
        stats->moveTimes = (double*)realloc(stats->moveTimes, stats->capacity * sizeof(double));
    }
    stats->moveTimes[stats->moveCount++] = moveTime;
    stats->totalTime += moveTime;
}

void calculateAverageTime(PerformanceStats* stats) {
    if (stats->moveCount > 0) {
        stats->averageTime = stats->totalTime / stats->moveCount;
    }
}

void printPerformanceReport(PerformanceStats* stats, int size, int depth, int threads) {
    printf("\nPerformance Report:\n");
    printf("Board Size: %dx%d\n", size, size);
    printf("Search Depth: %d\n", depth);
    printf("Threads: %d\n", threads);
    printf("Total Moves: %d\n", stats->moveCount);
    printf("Total Time: %.4f seconds\n", stats->totalTime);
    printf("Average Move Time: %.4f seconds\n", stats->averageTime);
    if (stats->speedup > 0) {
        printf("Speedup: %.2fx\n", stats->speedup);
    }
    printf("\nIndividual Move Times:\n");
    for (int i = 0; i < stats->moveCount; i++) {
        printf("Move %d: %.4f seconds\n", i + 1, stats->moveTimes[i]);
    }
    printf("\n");
}

void runPerformanceTests() {
    int sizes[] = {5, 6, 7};  // Board sizes to test
    int depths[] = {4, 6, 8}; // Search depths to test
    int threads[] = {1, 2, 4, 8}; // Number of threads to test
    
    printf("\nRunning Performance Tests...\n");
    printf("----------------------------\n");
    
    for (int s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        for (int d = 0; d < sizeof(depths)/sizeof(depths[0]); d++) {
            PerformanceStats* serialStats = NULL;
            
            for (int t = 0; t < sizeof(threads)/sizeof(threads[0]); t++) {
                bool isParallel = threads[t] > 1;
                PerformanceStats* stats = createStats(50);
                
                // Run Computer vs Computer game
                GameBoard* board = createBoard(sizes[s]);
                double totalTime = 0;
                int moves = 0;
                
                // First move in the middle
                int mid = sizes[s] / 2;
                board->board[mid][mid] = 'X';
                moves++;
                
                while (true) {
                    double moveTime;
                    computerMove(board, depths[d], isParallel, threads[t], &moveTime);
                    recordMoveTime(stats, moveTime);
                    moves++;
                    
                    if (checkWin(board, 'O') || !isMovesLeft(board)) break;
                    
                    computerMove(board, depths[d], isParallel, threads[t], &moveTime);
                    recordMoveTime(stats, moveTime);
                    moves++;
                    
                    if (checkWin(board, 'X') || !isMovesLeft(board)) break;
                }
                
                calculateAverageTime(stats);
                
                // Calculate speedup for parallel runs
                if (isParallel && serialStats != NULL) {
                    stats->speedup = serialStats->averageTime / stats->averageTime;
                } else if (!isParallel) {
                    serialStats = stats;
                }
                
                printf("\nTest Configuration:\n");
                printf("Board Size: %dx%d\n", sizes[s], sizes[s]);
                printf("Search Depth: %d\n", depths[d]);
                printf("Threads: %d\n", threads[t]);
                printf("Mode: %s\n", isParallel ? "Parallel" : "Serial");
                printf("Average Move Time: %.4f seconds\n", stats->averageTime);
                if (stats->speedup > 0) {
                    printf("Speedup: %.2fx\n", stats->speedup);
                }
                
                freeBoard(board);
                if (isParallel) {
                    freeStats(stats);
                }
            }
            
            if (serialStats != NULL) {
                freeStats(serialStats);
            }
        }
    }
}
