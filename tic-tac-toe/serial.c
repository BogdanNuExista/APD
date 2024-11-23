#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "serial.h"

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
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

void freeStats(PerformanceStats* stats) {
    free(stats->moveTimes);
    free(stats);
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

int minimax(GameBoard* board, int depth, bool isMax, int alpha, int beta) {
    if (checkWin(board, 'X')) return -10;
    if (checkWin(board, 'O')) return 10;
    if (!isMovesLeft(board) || depth == 0) return 0;

    if (isMax) {
        int best = -1000;
        for (int i = 0; i < board->size; i++) {
            for (int j = 0; j < board->size; j++) {
                if (board->board[i][j] == ' ') {
                    board->board[i][j] = 'O';
                    int val = minimax(board, depth - 1, !isMax, alpha, beta);
                    best = max(best, val);
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
        for (int i = 0; i < board->size; i++) {
            for (int j = 0; j < board->size; j++) {
                if (board->board[i][j] == ' ') {
                    board->board[i][j] = 'X';
                    int val = minimax(board, depth - 1, !isMax, alpha, beta);
                    best = min(best, val);
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

Move getBestMoveX(GameBoard* board, int depth) {
    Move bestMove;
    bestMove.row = -1;
    bestMove.col = -1;
    int bestVal = 1000;

    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] == ' ') {
                board->board[i][j] = 'X';
                int moveVal = minimax(board, depth, true, -1000, 1000);
                board->board[i][j] = ' ';

                if (moveVal < bestVal) {
                    bestMove.row = i;
                    bestMove.col = j;
                    bestVal = moveVal;
                }
            }
        }
    }
    return bestMove;
}

Move getBestMoveO(GameBoard* board, int depth) {
    Move bestMove;
    bestMove.row = -1;
    bestMove.col = -1;
    int bestVal = -1000;

    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->board[i][j] == ' ') {
                board->board[i][j] = 'O';
                int moveVal = minimax(board, depth, false, -1000, 1000);
                board->board[i][j] = ' ';

                if (moveVal > bestVal) {
                    bestMove.row = i;
                    bestMove.col = j;
                    bestVal = moveVal;
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

void computerMoveX(GameBoard* board, int depth, double* moveTime) {
    clock_t start = clock();
    Move bestMove = getBestMoveX(board, depth);
    clock_t end = clock();
    *moveTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    board->board[bestMove.row][bestMove.col] = 'X';
}

void computerMoveO(GameBoard* board, int depth, double* moveTime) {
    clock_t start = clock();
    Move bestMove = getBestMoveO(board, depth);
    clock_t end = clock();
    *moveTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    board->board[bestMove.row][bestMove.col] = 'O';
}

void playerVsComputer(int size, int depth) {
    GameBoard* board = createBoard(size);
    PerformanceStats* stats = createStats(50);
    double moveTime;
    
    printf("\nGame starts! You are X, Computer is O\n");
    
    while (true) {
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
        computerMoveO(board, depth, &moveTime);
        recordMoveTime(stats, moveTime);
        printf("Computer's move took: %.4f seconds\n", moveTime);
        
        if (checkWin(board, 'O')) {
            printBoard(board);
            printf("Computer wins!\n");
            break;
        }
        
        if (!isMovesLeft(board)) {
            printBoard(board);
            printf("Draw!\n");
            break;
        }
    }
    
    calculateAverageTime(stats);
    printPerformanceReport(stats, size, depth, 1);
    
    freeStats(stats);
    freeBoard(board);
}

void computerVsComputer(int size, int depth, bool debugMode) {
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
        computerMoveO(board, depth, &moveTime);
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
        computerMoveX(board, depth, &moveTime);
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
    
    calculateAverageTime(stats);
    printf("\nPerformance Statistics:\n");
    printf("------------------------\n");
    printf("Board Size: %dx%d\n", size, size);
    printf("Search Depth: %d\n", depth);
    printf("Total Moves: %d\n", moves);
    printf("Total Computer Moves: %d\n", stats->moveCount);
    printf("Average Move Time: %.4f seconds\n", stats->averageTime);
    printf("Total Game Time: %.4f seconds\n", stats->totalTime);
    
    freeStats(stats);
    freeBoard(board);
}

int main() {
    int choice, size, depth;
    bool debugMode;

    printf("Generalized Tic-tac-toe Game (Serial Version)\n");
    printf("1. Player vs Computer\n");
    printf("2. Computer vs Computer\n");
    printf("Enter choice (1-2): ");
    scanf("%d", &choice);

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

    if (choice == 1) {
        playerVsComputer(size, depth);
    } else if (choice == 2) {
        printf("Debug mode? (0/1): ");
        scanf("%d", (int*)&debugMode);
        computerVsComputer(size, depth, debugMode);
    }

    return 0;
}