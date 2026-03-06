/*
 * image_reader.c — Read a Sudoku grid from an image using stb_image + Tesseract
 * OCR
 */

#include "image_reader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Tesseract C API
#include <tesseract/capi.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static unsigned char *toGrayscale(const unsigned char *data, int w, int h,
                                  int channels) {
  unsigned char *gray = (unsigned char *)malloc(w * h);
  if (!gray)
    return NULL;
  for (int i = 0; i < w * h; i++) {
    int idx = i * channels;
    if (channels >= 3) {
      gray[i] = (unsigned char)(0.299 * data[idx] + 0.587 * data[idx + 1] +
                                0.114 * data[idx + 2]);
    } else {
      gray[i] = data[idx];
    }
  }
  return gray;
}

static int otsuThreshold(const unsigned char *data, int size) {
  int hist[256] = {0};
  for (int i = 0; i < size; i++)
    hist[data[i]]++;
  double sumAll = 0;
  for (int t = 0; t < 256; t++)
    sumAll += t * hist[t];
  double sumB = 0;
  int wB = 0;
  double maxVar = 0;
  int bestT = 128;
  for (int t = 0; t < 256; t++) {
    wB += hist[t];
    if (wB == 0)
      continue;
    int wF = size - wB;
    if (wF == 0)
      break;
    sumB += t * hist[t];
    double mB = sumB / wB;
    double mF = (sumAll - sumB) / wF;
    double var = (double)wB * wF * (mB - mF) * (mB - mF);
    if (var > maxVar) {
      maxVar = var;
      bestT = t;
    }
  }
  return bestT;
}

// Bilinear scale of a grayscale image
static unsigned char *scaleBilinear(const unsigned char *src, int srcW,
                                    int srcH, int dstW, int dstH) {
  unsigned char *dst = (unsigned char *)malloc(dstW * dstH);
  if (!dst)
    return NULL;

  float x_ratio = ((float)(srcW - 1)) / dstW;
  float y_ratio = ((float)(srcH - 1)) / dstH;

  for (int i = 0; i < dstH; i++) {
    for (int j = 0; j < dstW; j++) {
      int x = (int)(x_ratio * j);
      int y = (int)(y_ratio * i);
      float x_diff = (x_ratio * j) - x;
      float y_diff = (y_ratio * i) - y;

      int index = y * srcW + x;
      // boundary checks (just in case)
      int pA = src[index];
      int pB = (x + 1 < srcW) ? src[index + 1] : pA;
      int pC = (y + 1 < srcH) ? src[index + srcW] : pA;
      int pD = (x + 1 < srcW && y + 1 < srcH) ? src[index + srcW + 1] : pA;

      float val = pA * (1 - x_diff) * (1 - y_diff) +
                  pB * (x_diff) * (1 - y_diff) + pC * (y_diff) * (1 - x_diff) +
                  pD * (x_diff * y_diff);

      dst[i * dstW + j] = (unsigned char)val;
    }
  }
  return dst;
}

// Bounding box extraction
static unsigned char *extractDigitBBox(const unsigned char *data, int w, int h,
                                       int *bbW, int *bbH) {
  int minX = w, minY = h, maxX = -1, maxY = -1;
  int blackPixels = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if (data[y * w + x] < 128) { // thresholded black
        if (x < minX)
          minX = x;
        if (x > maxX)
          maxX = x;
        if (y < minY)
          minY = y;
        if (y > maxY)
          maxY = y;
        blackPixels++;
      }
    }
  }

  if (blackPixels < 20)
    return NULL; // Noise

  *bbW = maxX - minX + 1;
  *bbH = maxY - minY + 1;

  if (*bbW < 4 || *bbH < 8)
    return NULL; // Too small

  unsigned char *bbox = (unsigned char *)malloc((*bbW) * (*bbH));
  for (int y = 0; y < *bbH; y++) {
    for (int x = 0; x < *bbW; x++) {
      bbox[y * (*bbW) + x] = data[(minY + y) * w + (minX + x)];
    }
  }
  return bbox;
}

// Center in white canvas
static unsigned char *padToCanvas(const unsigned char *bbox, int bbW, int bbH,
                                  int canvasSize) {
  unsigned char *canvas = (unsigned char *)malloc(canvasSize * canvasSize);
  memset(canvas, 255, canvasSize * canvasSize);

  int offsetX = (canvasSize - bbW) / 2;
  int offsetY = (canvasSize - bbH) / 2;

  for (int y = 0; y < bbH; y++) {
    for (int x = 0; x < bbW; x++) {
      canvas[(offsetY + y) * canvasSize + (offsetX + x)] = bbox[y * bbW + x];
    }
  }

  // Return canvas directly without dilation, as dilation can merge holes in '8'
  // and '6' for thick fonts.
  return canvas;
}

// Optional Dilation: Thicken the strokes of the digits
// useful for thin fonts where Tesseract fails to detect the number.
static unsigned char *dilateCanvas(const unsigned char *canvas,
                                   int canvasSize) {
  unsigned char *dilated = (unsigned char *)malloc(canvasSize * canvasSize);
  memset(dilated, 255, canvasSize * canvasSize);
  for (int y = 1; y < canvasSize - 1; y++) {
    for (int x = 1; x < canvasSize - 1; x++) {
      if (canvas[y * canvasSize + x] == 0 ||
          canvas[(y - 1) * canvasSize + x] == 0 ||
          canvas[(y + 1) * canvasSize + x] == 0 ||
          canvas[y * canvasSize + (x - 1)] == 0 ||
          canvas[y * canvasSize + (x + 1)] == 0) {
        dilated[y * canvasSize + x] = 0;
      }
    }
  }
  return dilated;
}

// Auto-crop white borders from the image
static void autoCropGrid(unsigned char **gray_ptr, int *imgW, int *imgH) {
  unsigned char *gray = *gray_ptr;
  int w = *imgW;
  int h = *imgH;
  int minX = w, minY = h, maxX = 0, maxY = 0;

  // Find limits of dark pixels (grid lines)
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if (gray[y * w + x] < 200) {
        if (x < minX)
          minX = x;
        if (x > maxX)
          maxX = x;
        if (y < minY)
          minY = y;
        if (y > maxY)
          maxY = y;
      }
    }
  }

  if (minX > maxX || minY > maxY)
    return; // Completely empty image

  // Add a 1px buffer if possible to not slice the exact line
  if (minX > 0)
    minX--;
  if (minY > 0)
    minY--;
  if (maxX < w - 1)
    maxX++;
  if (maxY < h - 1)
    maxY++;

  int newW = maxX - minX + 1;
  int newH = maxY - minY + 1;

  // If cropped is same as original, do nothing
  if (newW == w && newH == h)
    return;

  unsigned char *cropped = (unsigned char *)malloc(newW * newH);
  if (!cropped)
    return;

  for (int y = 0; y < newH; y++) {
    for (int x = 0; x < newW; x++) {
      cropped[y * newW + x] = gray[(minY + y) * w + (minX + x)];
    }
  }

  free(gray);
  *gray_ptr = cropped;
  *imgW = newW;
  *imgH = newH;
  printf("  ✂️  Auto-cropped borders: new size %d × %d px\n", newW, newH);
}

// ---------------------------------------------------------------------------
// Main API
// ---------------------------------------------------------------------------

int readSudokuFromImage(const char *imagePath, int matrix[SIZE][SIZE]) {
  printf("\n  📷 Loading image: %s\n", imagePath);

  int imgW, imgH, channels;
  unsigned char *imgData = stbi_load(imagePath, &imgW, &imgH, &channels, 0);
  if (!imgData) {
    fprintf(stderr, "  ❌ Error: Could not load image '%s'\n", imagePath);
    return -1;
  }
  printf("  ✅ Image loaded: %d × %d px, %d channel(s)\n", imgW, imgH,
         channels);

  unsigned char *gray = toGrayscale(imgData, imgW, imgH, channels);
  stbi_image_free(imgData);
  if (!gray)
    return -1;

  // Crop out any white borders so the grid lines align exactly with edges
  autoCropGrid(&gray, &imgW, &imgH);

  double cellW_float = (double)imgW / SIZE;
  double cellH_float = (double)imgH / SIZE;
  printf("  📐 Cell size: %.2f × %.2f px\n", cellW_float, cellH_float);

  TessBaseAPI *tess = TessBaseAPICreate();
  if (TessBaseAPIInit3(tess, NULL, "eng") != 0) {
    fprintf(stderr, "  ❌ Error: Could not initialize Tesseract\n");
    free(gray);
    TessBaseAPIDelete(tess);
    return -1;
  }
  TessBaseAPISetVariable(tess, "tessedit_char_whitelist", "123456789");
  TessBaseAPISetPageSegMode(tess, PSM_SINGLE_CHAR);

  printf("\n  🔍 Reading cells...\n\n");

  int emptyCount = 0, filledCount = 0;

  for (int row = 0; row < SIZE; row++) {
    printf("  Row %d: ", row + 1);
    fflush(stdout);

    for (int col = 0; col < SIZE; col++) {
      int cellX = (int)(col * cellW_float);
      int cellY = (int)(row * cellH_float);
      int cellW = (int)((col + 1) * cellW_float) - cellX;
      int cellH = (int)((row + 1) * cellH_float) - cellY;

      // 15% inner crop margin
      int marginX = (int)(cellW * 0.15);
      int marginY = (int)(cellH * 0.15);
      int innerW = cellW - 2 * marginX;
      int innerH = cellH - 2 * marginY;

      unsigned char *innerBuf = (unsigned char *)malloc(innerW * innerH);
      if (!innerBuf) {
        fprintf(stderr, "  ❌ Error: Memory allocation failed for innerBuf\n");
        TessBaseAPIEnd(tess);
        TessBaseAPIDelete(tess);
        free(gray);
        return -1;
      }
      int x0 = cellX + marginX;
      int y0 = cellY + marginY;

      // Extract small inner grayscale
      for (int y = 0; y < innerH; y++)
        for (int x = 0; x < innerW; x++)
          innerBuf[y * innerW + x] = gray[(y0 + y) * imgW + (x0 + x)];

      // Scale to 60x60 BEFORE thresholding using Bilinear Interpolation
      int scaledW = 60, scaledH = 60;
      unsigned char *scaled =
          scaleBilinear(innerBuf, innerW, innerH, scaledW, scaledH);

      // Otsu threshold the smooth scaled image
      int thresh = otsuThreshold(scaled, scaledW * scaledH);
      for (int i = 0; i < scaledW * scaledH; i++) {
        scaled[i] = (scaled[i] > thresh * 0.9) ? 255 : 0;
      }

      // Extract BBox
      int bbW, bbH;
      unsigned char *bbox =
          extractDigitBBox(scaled, scaledW, scaledH, &bbW, &bbH);
      free(scaled);

      if (!bbox) {
        matrix[row][col] = 0;
        printf(". ");
        emptyCount++;
        continue;
      }

      // Pad to perfectly centered 80x80 canvas
      int canvasSize = 80;
      unsigned char *canvas = padToCanvas(bbox, bbW, bbH, canvasSize);
      free(bbox);

      // OCR
      TessBaseAPISetImage(tess, canvas, canvasSize, canvasSize, 1, canvasSize);
      char *text = TessBaseAPIGetUTF8Text(tess);

      int digit = 0;
      if (text) {
        for (int k = 0; text[k]; k++) {
          if (text[k] >= '1' && text[k] <= '9') {
            digit = text[k] - '0';
            break;
          }
        }
        TessDeleteText(text);
      }
      TessBaseAPIClear(tess);

      if (digit == 0) {
        // Fallback for thin fonts: try with dilation
        unsigned char *dilatedCanvas = dilateCanvas(canvas, canvasSize);
        TessBaseAPISetImage(tess, dilatedCanvas, canvasSize, canvasSize, 1,
                            canvasSize);
        char *fallbackText = TessBaseAPIGetUTF8Text(tess);

        if (fallbackText) {
          int conf = TessBaseAPIMeanTextConf(tess);
          if (conf >= 55) {
            for (int k = 0; fallbackText[k]; k++) {
              if (fallbackText[k] >= '1' && fallbackText[k] <= '9') {
                digit = fallbackText[k] - '0';
                break;
              }
            }
          }
          TessDeleteText(fallbackText);
        }
        TessBaseAPIClear(tess);

        if (digit == 0) {
          // Both failed, save the original canvas for debugging
          char debugPath[256];
          system("mkdir -p /tmp/failed_ocr");
          snprintf(debugPath, sizeof(debugPath),
                   "/tmp/failed_ocr/fail_r%d_c%d.png", row, col);
          stbi_write_png(debugPath, canvasSize, canvasSize, 1, canvas,
                         canvasSize);
        }
        free(dilatedCanvas);
      }

      free(canvas);

      matrix[row][col] = digit;
      if (digit > 0) {
        printf("%d ", digit);
        filledCount++;
      } else {
        printf("? ");
        emptyCount++;
      }
      free(innerBuf);
    }
    printf("\n");
  }

  TessBaseAPIEnd(tess);
  TessBaseAPIDelete(tess);
  free(gray);

  printf("\n  📊 Recognition complete: %d filled cells, %d empty cells\n\n",
         filledCount, emptyCount);

  return 0;
}
