# Sudoku Solver (C with OpenCV & Tesseract OCR)

A high-performance Sudoku Solver written in C that can solve puzzles natively using a backtracking algorithm and read puzzles automatically from images via Tesseract OCR and custom C-based OpenCV-like routines.

## Features
- **Dual Input Modes**: Solve via manual terminal entry or directly from a photo of a Sudoku grid.
- **Custom Image Processing Algorithm**: Natively implemented Otsu's thresholding, Grayscale conversion, Bilinear Scaling, and Auto-cropping in C without needing massive external computer-vision libraries.
- **Fast Backtracking Solver**: Instantly resolves difficult boards in microseconds.

## Project Structure
```text
├── image_reader.c   # OCR pipeline: Image processing, thresholding, Tesseract extraction
├── image_reader.h   # Header for the OCR pipeline
├── main.c           # Entry point: Handles CLI arguments and times the solving execution 
├── solver.c         # Heart of the algorithm: Validation and backtracking solver 
├── solver.h         # Header for solver logic
├── Makefile / gcc   # Compilation commands
└── *.jpeg           # Test images used for validation
```

## How It Works: The Execution Pipeline

### Phase 1: Image Processing (`image_reader.c`)
If you provide an image, the OCR routine is triggered:
1. **Grayscale Conversion**: The RGB image is converted into a single-channel grayscale matrix.
2. **Auto-Cropping**: Outer white borders are automatically cropped so that the Sudoku grid lines align to the bounds.
3. **Grid Sub-segmentation**: The image is mapped into an exact `9x9` array grid based on pixel bounds.
4. **Bilinear Scaling & Thresholding**: Each individual cell is cropped out with a 15% inner margin offset, scaled to 60x60 using a Bilinear algorithm, and converted into pure black & white via **Otsu's Method**.
5. **Bounding Box**: White noise is ignored and the true black text is centered dynamically onto an 80x80 canvas.
6. **Tesseract OCR Engine**: The engine runs in `PSM_SINGLE_CHAR` (Page Segmentation Mode 10) mapped to read digits from `1-9`, detecting the text from the canvas.

### Phase 2: The Solver (`solver.c`)
1. **Matrix Generation**: The detected digits (or manual entries) fill a standard `9x9` matrix. Unknown digits are left as `0`.
2. **Validation**: The solver algorithm scans each row, column, and 3x3 subgrid to verify the Sudoku rules are unbroken.
3. **Backtracking**: Finding an empty space, it natively tests integers 1-9. It recurses down the matrix; if an integer choice fails downstream, it logically backtracks and re-evaluates.
4. **Completion**: Prints the completed, true puzzle in terminal output.

## Prerequisites
To compile the image processing code, you must install the **Tesseract OCR C-API** library. 
On macOS (using Homebrew):
```bash
brew install tesseract
```

## Compilation

Build the program by linking the correct libraries. From the root directory:

```bash
# Example for macOS with Homebrew paths
gcc -o sudoku main.c solver.c image_reader.c -ltesseract -lm -I/opt/homebrew/include -L/opt/homebrew/lib
```

## Usage
Run the generated `./sudoku` executable.

**1. Manual Input Mode:**
Input the grid rows manually when prompted. Use `0` for empty squares.
```bash
./sudoku
```

**2. Image Recognition Mode:**
Pass an image path as an argument using `-i` or `--image`.
```bash
./sudoku -i suduko.jpeg
```

Example Output:
```text
  📸 Mode: Image Recognition

  📷 Loading image: suduko.jpeg
  ✅ Image loaded: 260 × 256 px, 3 channel(s)
  🔍 Reading cells...
  ...
  📊 Recognition complete: 14 filled cells, 67 empty cells

  ─── Initial Sudoku Puzzle ───
  ╔═══════╦═══════╦═══════╗
  ║ . . . ║ 8 . 1 ║ . . . ║
  ║ . . . ║ . . . ║ 4 3 . ║
  ...

  ⏳ Solving...

  ─── Solved Sudoku Puzzle ───
  ╔═══════╦═══════╦═══════╗
  ║ 2 3 4 ║ 8 5 1 ║ 7 6 9 ║
  ...
  ⏱  Solve time: 60.0 µs
```
