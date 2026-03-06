#include "solver.h"

//-----------------------------------------------------
// Function to print the Sudoku grid
//-----------------------------------------------------

void printMatrix(int matrix[SIZE][SIZE]) {
  printf("\n  ╔═══════╦═══════╦═══════╗\n");
  for (int i = 0; i < SIZE; i++) {
    printf("  ║ ");
    for (int j = 0; j < SIZE; j++) {
      if (matrix[i][j] == 0) {
        printf(". ");
      } else {
        printf("%d ", matrix[i][j]);
      }
      if (j == 2 || j == 5) {
        printf("║ ");
      }
    }
    printf("║\n");
    if (i == 2 || i == 5) {
      printf("  ╠═══════╬═══════╬═══════╣\n");
    }
  }
  printf("  ╚═══════╩═══════╩═══════╝\n\n");
}

//--------------------------------------
// Function to check if it's valid to place num in the given row, column
//--------------------------------------

bool isValid(int matrix[SIZE][SIZE], int row, int col, int num) {
  // Check the row
  for (int j = 0; j < SIZE; j++) {
    if (matrix[row][j] == num) {
      return false;
    }
  }

  // Check the column
  for (int i = 0; i < SIZE; i++) {
    if (matrix[i][col] == num) {
      return false;
    }
  }

  // Check the 3x3 subgrid
  int startRow = row - row % 3;
  int startCol = col - col % 3;
  for (int i = startRow; i < startRow + 3; i++) {
    for (int j = startCol; j < startCol + 3; j++) {
      if (matrix[i][j] == num) {
        return false;
      }
    }
  }

  return true;
}

//==============================
// Function to find the next empty cell in the Sudoku grid (represented by 0)
//==============================

bool findEmptyCell(int matrix[SIZE][SIZE], int *row, int *col) {
  for (*row = 0; *row < SIZE; (*row)++) {
    for (*col = 0; *col < SIZE; (*col)++) {
      if (matrix[*row][*col] == 0) {
        return true; // Empty cell found
      }
    }
  }
  return false; // No empty cells left
}

//==================================================
// Backtracking function to solve the Sudoku
//==================================================

bool solveSudoku(int matrix[SIZE][SIZE]) {
  int row, col;

  // If there are no empty cells, the puzzle is solved
  if (!findEmptyCell(matrix, &row, &col)) {
    return true;
  }

  // Try numbers 1 to 9 in the empty cell
  for (int num = 1; num <= 9; num++) {
    if (isValid(matrix, row, col, num)) {
      matrix[row][col] = num; // Place the number

      // Recursively solve the rest of the grid
      if (solveSudoku(matrix)) {
        return true;
      }

      // If the number doesn't work, backtrack
      matrix[row][col] = 0;
    }
  }

  // If no number works, return false (backtrack)
  return false;
}

//==============================
// Get Sudoku grid from manual user input
//==============================

void manualInput(int matrix[SIZE][SIZE]) {
  printf("Enter the Sudoku puzzle (0 for empty cells):\n\n");

  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      printf("  Cell (%d,%d): ", i + 1, j + 1);
      scanf("%d", &matrix[i][j]);
      if (matrix[i][j] < 0 || matrix[i][j] > 9) {
        printf("  ⚠ Invalid! Enter a number between 0 and 9.\n");
        j--; // Retry this cell
      }
    }
  }
}
