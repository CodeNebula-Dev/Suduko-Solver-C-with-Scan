#ifndef IMAGE_READER_H
#define IMAGE_READER_H

#define SIZE 9

// Read a Sudoku grid from an image file.
// The image should be a cleanly cropped Sudoku grid.
// Returns 0 on success, -1 on failure.
int readSudokuFromImage(const char *imagePath, int matrix[SIZE][SIZE]);

#endif // IMAGE_READER_H
