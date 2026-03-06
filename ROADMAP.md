# 🧩 Sudoku Solver — Project Roadmap

> **From manual input to image recognition — the complete evolution of our Sudoku solving approach.**

---

## 📖 Table of Contents

1. [Phase 1 — The Raw Approach (Manual Input)](#phase-1--the-raw-approach-manual-input)
2. [Phase 2 — The Solving Algorithm (Backtracking)](#phase-2--the-solving-algorithm-backtracking)
3. [Phase 3 — The Automation Idea (Image Recognition)](#phase-3--the-automation-idea-image-recognition)
4. [Phase 4 — Integration & Performance Timing](#phase-4--integration--performance-timing)
5. [Phase 5 — Future Ideas](#phase-5--future-ideas)

---

## Phase 1 — The Raw Approach (Manual Input)

### 🎯 The Starting Point

We began with the simplest possible way to feed a Sudoku puzzle into a computer program: **ask the user to type every single cell value**.

### How It Works

The program uses a nested loop to iterate through all 81 cells of the 9×9 grid. For each cell, it prompts the user:

```
Enter value for cell (1,1): 5
Enter value for cell (1,2): 3
Enter value for cell (1,3): 0    ← 0 means empty
...
```

**The convention:** Enter the digit (1–9) if the cell has a number, or **0** if the cell is empty.

### The Code (from `Algorithm.c`)

```c
for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
        printf("Enter value for cell (%d,%d): ", i + 1, j + 1);
        scanf("%d", &matrix[i][j]);
        if (matrix[i][j] < 0 || matrix[i][j] > 9) {
            printf("Invalid! Enter numbers between 0 and 9.\n");
            j--;  // Retry this cell
        }
    }
}
```

### 📊 Input validation

The code validates each entry to be between 0 and 9. If the user enters an invalid number, it asks them to re-enter that cell.

### 😤 The Pain Point

Entering 81 values manually is **tedious and error-prone**:
- A typical Sudoku has ~25–35 given cells and ~46–56 empty cells
- You still have to enter `0` for every empty cell
- One typo can lead to "No solution exists!"
- It takes 2–3 minutes just to enter the puzzle

> **This frustration is what motivated Phase 3 — automating input via image recognition.**

---

## Phase 2 — The Solving Algorithm (Backtracking)

### 🧠 The Core Idea

We use a **backtracking** algorithm — a brute-force strategy that systematically tries all possibilities and "backtracks" when it hits a dead end.

### Step-by-Step Logic

#### 1. Find an Empty Cell

Scan the grid left-to-right, top-to-bottom for the first cell containing `0`:

```c
bool findEmptyCell(int matrix[SIZE][SIZE], int *row, int *col) {
    for (*row = 0; *row < SIZE; (*row)++) {
        for (*col = 0; *col < SIZE; (*col)++) {
            if (matrix[*row][*col] == 0) {
                return true;  // Found an empty cell
            }
        }
    }
    return false;  // No empty cells → puzzle is solved!
}
```

#### 2. Check if a Number is Valid

Before placing a number (1–9) in an empty cell, we must verify it doesn't violate Sudoku rules:

| Check        | What it does                                      |
|-------------|---------------------------------------------------|
| **Row**      | No duplicate in the same row (all 9 columns)      |
| **Column**   | No duplicate in the same column (all 9 rows)      |
| **3×3 Box**  | No duplicate in the 3×3 subgrid the cell belongs to |

```c
bool isValid(int matrix[SIZE][SIZE], int row, int col, int num) {
    // Check row
    for (int j = 0; j < SIZE; j++)
        if (matrix[row][j] == num) return false;

    // Check column
    for (int i = 0; i < SIZE; i++)
        if (matrix[i][col] == num) return false;

    // Check 3×3 box
    int startRow = row - row % 3;
    int startCol = col - col % 3;
    for (int i = startRow; i < startRow + 3; i++)
        for (int j = startCol; j < startCol + 3; j++)
            if (matrix[i][j] == num) return false;

    return true;  // Safe to place
}
```

#### 3. Recursive Backtracking

The heart of the algorithm:

```
┌──────────────────────────────────────────────┐
│  Find empty cell                             │
│  ├─ No empty cell? → SOLVED ✅                │
│  └─ Found (row, col)?                        │
│       ├─ Try num = 1, 2, ..., 9              │
│       │   ├─ isValid? → place it, recurse    │
│       │   │   ├─ Recursion succeeds? → DONE  │
│       │   │   └─ Recursion fails? → undo,    │
│       │   │      try next number              │
│       │   └─ Not valid? → skip               │
│       └─ All numbers fail → BACKTRACK ↩️      │
└──────────────────────────────────────────────┘
```

```c
bool solveSudoku(int matrix[SIZE][SIZE]) {
    int row, col;
    if (!findEmptyCell(matrix, &row, &col))
        return true;  // Solved!

    for (int num = 1; num <= 9; num++) {
        if (isValid(matrix, row, col, num)) {
            matrix[row][col] = num;       // Try this number
            if (solveSudoku(matrix))
                return true;              // It worked!
            matrix[row][col] = 0;         // Undo (backtrack)
        }
    }
    return false;  // Dead end, go back
}
```

### ⏱ Time Complexity

- **Worst case:** O(9^(n)) where n = number of empty cells
- **In practice:** The constraint checking prunes most branches early
- **Typical solve time:** < 1 millisecond for standard puzzles

---

## Phase 3 — The Automation Idea (Image Recognition)

### 💡 The Thought Process

> "What if instead of typing 81 numbers, I could just give the program a **picture** of the Sudoku puzzle?"

This is where image recognition comes in. The idea:

```
 📷 Photo of Sudoku → 🔍 Read digits → 🧩 Solve → ✅ Answer
```

### 🛠 Technology Choices

| Component          | Library         | Why?                                          |
|-------------------|-----------------|-----------------------------------------------|
| Image loading      | `stb_image.h`   | Single-header C library, no dependencies      |
| Image writing      | `stb_image_write.h` | For saving cell images for debugging      |
| Digit recognition  | Tesseract OCR   | Industry-standard OCR engine, has C API       |

### 📐 The Grid Division Logic

Since the input image is a **perfectly cropped** Sudoku grid, we use a simple uniform division:

```
Image Width  = W pixels
Image Height = H pixels

Cell Width   = W / 9
Cell Height  = H / 9

Cell (row, col) starts at pixel:
  x = col × cellWidth
  y = row × cellHeight
```

**Visual representation:**

```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│(0,0)│(0,1)│(0,2)│(0,3)│(0,4)│(0,5)│(0,6)│(0,7)│(0,8)│
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│(1,0)│(1,1)│ ... │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ ... │     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │     │
│     │     │     │     │     │     │     │     │     │
│(8,0)│     │     │     │     │     │     │     │(8,8)│
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

### 🔍 Cell Processing Pipeline

For each of the 81 cells, the software follows this pipeline:

```
┌────────────────┐     ┌────────────┐     ┌──────────────┐
│ Extract cell   │────▶│ Convert to │────▶│   Binary     │
│ sub-image      │     │ grayscale  │     │ thresholding │
└────────────────┘     └────────────┘     └──────────────┘
                                                  │
                                                  ▼
                                          ┌──────────────┐
                                          │ Count dark   │
                                          │ pixels       │
                                          └──────────────┘
                                                  │
                                    ┌─────────────┴──────────────┐
                                    ▼                            ▼
                           ┌──────────────┐            ┌──────────────┐
                           │ Very few     │            │ Many dark    │
                           │ dark pixels  │            │ pixels found │
                           │  → EMPTY (0) │            │  → HAS DIGIT │
                           └──────────────┘            └──────────────┘
                                                              │
                                                              ▼
                                                     ┌──────────────┐
                                                     │ Add padding  │
                                                     │ around cell  │
                                                     └──────────────┘
                                                              │
                                                              ▼
                                                     ┌──────────────┐
                                                     │ Tesseract    │
                                                     │ OCR → digit  │
                                                     │ (1–9)        │
                                                     └──────────────┘
```

### Step-by-Step Breakdown

#### Step 1: Load the Image

```c
unsigned char *imgData = stbi_load(imagePath, &imgW, &imgH, &channels, 0);
```

This loads any PNG, JPG, or BMP file into a raw pixel array.

#### Step 2: Convert to Grayscale

```c
gray[i] = 0.299 * R + 0.587 * G + 0.114 * B;
```

The luminosity formula gives us a single brightness value per pixel.

#### Step 3: Binary Thresholding

```c
pixel = (pixel > 128) ? 255 : 0;    // White or Black
```

This simplifies the image to pure black and white, making digits stand out.

#### Step 4: Empty Cell Detection

```c
double darkRatio = darkPixels / totalInnerPixels;
if (darkRatio < 0.03) {
    matrix[row][col] = 0;  // Cell is empty
}
```

We apply a **10% margin** inward from each cell edge (to avoid counting grid lines as dark pixels), then check: if less than 3% of the inner area is dark, the cell is empty.

#### Step 5: OCR with Tesseract

```c
TessBaseAPISetVariable(tess, "tessedit_char_whitelist", "123456789");
TessBaseAPISetPageSegMode(tess, PSM_SINGLE_CHAR);
TessBaseAPISetImage(tess, cellImage, width, height, 1, width);
char *text = TessBaseAPIGetUTF8Text(tess);
```

Key configurations:
- **Whitelist** `"123456789"` — only recognize these characters (no letters, no 0)
- **Single character mode** — tell Tesseract each image contains exactly one character
- **Padding** — we add white space around the digit for better recognition accuracy

---

## Phase 4 — Integration & Performance Timing

### 🔗 Unified Interface

Both input modes feed into the same solver. The program decides based on command-line arguments:

```
./sudoku              → Manual input (scanf)
./sudoku --image FILE → Image recognition
```

### Architecture

```
┌─────────────────────────────────────────────────────┐
│                    main.c                            │
│                                                     │
│  ┌─────────────┐        ┌────────────────────────┐  │
│  │ manualInput()│        │ readSudokuFromImage()  │  │
│  │  (solver.c)  │        │   (image_reader.c)     │  │
│  └──────┬───────┘        └───────────┬────────────┘  │
│         │                            │               │
│         └──────────┬─────────────────┘               │
│                    ▼                                 │
│            ┌──────────────┐                          │
│            │ int matrix   │                          │
│            │  [9][9]      │                          │
│            └──────┬───────┘                          │
│                   ▼                                  │
│            ┌──────────────┐                          │
│            │ solveSudoku()│                          │
│            │  (solver.c)  │                          │
│            └──────┬───────┘                          │
│                   ▼                                  │
│            ┌──────────────┐                          │
│            │ printMatrix()│                          │
│            │ + timing     │                          │
│            └──────────────┘                          │
└─────────────────────────────────────────────────────┘
```

### ⏱ Performance Timing

We use C's `clock()` to measure precisely how long the backtracking solver takes:

```c
clock_t start = clock();
bool solved = solveSudoku(matrix);
clock_t end = clock();

double elapsed_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
```

The output automatically scales units:
- Under 1 ms → displayed in **microseconds** (µs)
- Under 1 second → displayed in **milliseconds** (ms)
- Over 1 second → displayed in **seconds** (s)

### 📁 File Structure

```
Sudoku Solver C with Scan/
├── Algorithm.c          ← Original single-file version (kept for reference)
├── main.c               ← New unified entry point
├── solver.h / solver.c  ← Extracted solver functions
├── image_reader.h/.c    ← Image recognition pipeline
├── stb_image.h          ← Image loading library (bundled)
├── stb_image_write.h    ← Image writing library (bundled)
├── Makefile             ← Build system
└── ROADMAP.md           ← This document
```

### 🏗 Building

```bash
# Install dependencies (macOS)
brew install tesseract

# Build the project
make

# Run in manual mode
./sudoku

# Run with an image
./sudoku --image puzzle.png

# Clean build artifacts
make clean
```

---

## Phase 5 — Future Ideas

### 🚀 Where We Could Take This Next

| Idea                        | Description                                                     |
|----------------------------|-----------------------------------------------------------------|
| **Perspective correction** | Use Hough line detection to handle angled/skewed photos         |
| **Camera input**           | Capture directly from webcam instead of a saved file            |
| **GUI overlay**            | Display the solution overlaid on the original image             |
| **Multiple puzzles**       | Batch-process a folder of Sudoku images                         |
| **Difficulty rating**      | Analyze puzzles before solving to estimate difficulty           |
| **Solution animation**     | Step-by-step visualization of the backtracking process          |
| **Web version**            | Port to WebAssembly for a browser-based solver                  |
| **Training data**          | Use misrecognized digits to fine-tune a custom OCR model        |

---

> **Built with ❤️ — From 81 keystrokes to zero, one pixel at a time.**
