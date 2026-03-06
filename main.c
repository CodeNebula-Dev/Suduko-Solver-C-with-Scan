/*
 * main.c — Sudoku Solver with dual input modes
 *
 * Usage:
 *   ./sudoku              → Manual input mode (type values via terminal)
 *   ./sudoku --image FILE → Image recognition mode (read from Sudoku grid
 * image)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "image_reader.h"
#include "solver.h"

// ---------------------------------------------------------------------------
// Banner
// ---------------------------------------------------------------------------

static void printBanner(void) {
  printf("\n");
  printf("  ╔══════════════════════════════════════════╗\n");
  printf("  ║         🧩 SUDOKU SOLVER v2.0 🧩         ║\n");
  printf("  ║    Manual Input  •  Image Recognition    ║\n");
  printf("  ╚══════════════════════════════════════════╝\n\n");
}

static void printUsage(const char *prog) {
  printf("  Usage:\n");
  printf("    %s              Manual input mode\n", prog);
  printf("    %s --image FILE Image recognition mode\n\n", prog);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  int matrix[SIZE][SIZE];
  int useImage = 0;
  const char *imagePath = NULL;

  // Parse arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--image") == 0 || strcmp(argv[i], "-i") == 0) {
      if (i + 1 < argc) {
        useImage = 1;
        imagePath = argv[++i];
      } else {
        fprintf(stderr,
                "  ❌ Error: --image requires a file path argument\n\n");
        printUsage(argv[0]);
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printBanner();
      printUsage(argv[0]);
      return 0;
    }
  }

  printBanner();

  // ---------- Input ----------
  if (useImage) {
    printf("  📸 Mode: Image Recognition\n");
    if (readSudokuFromImage(imagePath, matrix) != 0) {
      fprintf(stderr, "  ❌ Failed to read Sudoku from image. Exiting.\n\n");
      return 1;
    }
  } else {
    printf("  ⌨️  Mode: Manual Input\n\n");
    manualInput(matrix);
  }

  // ---------- Display initial puzzle ----------
  printf("  ─── Initial Sudoku Puzzle ───\n");
  printMatrix(matrix);

  // ---------- Solve with timing ----------
  printf("  ⏳ Solving...\n");

  clock_t start = clock();
  int solved = solveSudoku(matrix);
  clock_t end = clock();

  double elapsed_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
  double elapsed_us = (double)(end - start) / CLOCKS_PER_SEC * 1000000.0;

  if (solved) {
    printf("\n  ─── Solved Sudoku Puzzle ───\n");
    printMatrix(matrix);

    printf("  ⏱  Solve time: ");
    if (elapsed_ms < 1.0) {
      printf("%.1f µs", elapsed_us);
    } else if (elapsed_ms < 1000.0) {
      printf("%.3f ms", elapsed_ms);
    } else {
      printf("%.3f s", elapsed_ms / 1000.0);
    }
    printf("\n\n");
  } else {
    printf("\n  ❌ No solution exists for this puzzle!\n\n");
  }

  return 0;
}
