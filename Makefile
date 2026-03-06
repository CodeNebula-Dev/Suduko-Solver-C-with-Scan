# Sudoku Solver — Makefile
# Builds the unified solver with manual + image input modes

CC      = gcc
CFLAGS  = -Wall -O2 $(shell pkg-config --cflags tesseract lept)
LDFLAGS = $(shell pkg-config --libs tesseract lept) -lm

SRCS    = main.c solver.c image_reader.c
TARGET  = sudoku

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) solver.h image_reader.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

# ─── Quick test targets ───

test-manual:
	@echo "5 3 0 0 7 0 0 0 0 6 0 0 1 9 5 0 0 0 0 9 8 0 0 0 0 6 0 8 0 0 0 6 0 0 0 3 4 0 0 8 0 3 0 0 1 7 0 0 0 2 0 0 0 6 0 6 0 0 0 0 2 8 0 0 0 0 4 1 9 0 0 5 0 0 0 0 8 0 0 7 9" | ./$(TARGET)

test-image:
	./$(TARGET) --image sample_sudoku.png
