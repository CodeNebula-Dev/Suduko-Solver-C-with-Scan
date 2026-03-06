#ifndef SOLVER_H
#define SOLVER_H

#include <stdbool.h>
#include <stdio.h>

#define SIZE 9

// Print the Sudoku grid
void printMatrix(int matrix[SIZE][SIZE]);

// Check if placing num at (row, col) is valid
bool isValid(int matrix[SIZE][SIZE], int row, int col, int num);

// Find the next empty cell (value == 0)
bool findEmptyCell(int matrix[SIZE][SIZE], int *row, int *col);

// Solve the Sudoku using backtracking
bool solveSudoku(int matrix[SIZE][SIZE]);

// Get Sudoku grid from manual user input (scanf)
void manualInput(int matrix[SIZE][SIZE]);

#endif // SOLVER_H
