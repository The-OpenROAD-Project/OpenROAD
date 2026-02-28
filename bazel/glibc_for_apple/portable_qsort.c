/*
 * Portable qsort() implementation matching glibc's msort algorithm.
 *
 * glibc's qsort uses a merge sort (which is stable) with a temporary
 * buffer, falling back to insertion sort for small subarrays. Apple's
 * libc uses a different algorithm that produces different orderings for
 * equal elements. This causes non-determinism in ABC's logic synthesis.
 *
 * This implements the same algorithm as glibc's stdlib/msort.c to ensure
 * identical behavior across platforms.
 *
 * Source reference: glibc/stdlib/msort.c (LGPL 2.1)
 * Simplified implementation of the merge sort algorithm used by glibc.
 */

#include <stdlib.h>
#include <string.h>

/* Threshold below which we use insertion sort */
#define INSERTION_THRESHOLD 4

static void insertion_sort(void *base, size_t nmemb, size_t size,
                           int (*compar)(const void *, const void *)) {
  char *b = (char *)base;
  char *tmp = (char *)malloc(size);
  if (!tmp)
    return;

  for (size_t i = 1; i < nmemb; i++) {
    char *key = b + i * size;
    size_t j = i;
    while (j > 0 && compar(b + (j - 1) * size, key) > 0) {
      j--;
    }
    if (j != i) {
      memcpy(tmp, key, size);
      memmove(b + (j + 1) * size, b + j * size, (i - j) * size);
      memcpy(b + j * size, tmp, size);
    }
  }
  free(tmp);
}

static void merge(char *dst, const char *src1, size_t n1, const char *src2,
                  size_t n2, size_t size,
                  int (*compar)(const void *, const void *)) {
  while (n1 > 0 && n2 > 0) {
    if (compar(src1, src2) <= 0) {
      memcpy(dst, src1, size);
      src1 += size;
      n1--;
    } else {
      memcpy(dst, src2, size);
      src2 += size;
      n2--;
    }
    dst += size;
  }
  if (n1 > 0)
    memcpy(dst, src1, n1 * size);
  if (n2 > 0)
    memcpy(dst, src2, n2 * size);
}

static void msort_with_tmp(void *base, size_t nmemb, size_t size,
                           int (*compar)(const void *, const void *),
                           char *tmp) {
  char *b = (char *)base;

  if (nmemb <= INSERTION_THRESHOLD) {
    insertion_sort(base, nmemb, size, compar);
    return;
  }

  size_t n1 = nmemb / 2;
  size_t n2 = nmemb - n1;

  msort_with_tmp(b, n1, size, compar, tmp);
  msort_with_tmp(b + n1 * size, n2, size, compar, tmp);

  /* Merge into tmp, then copy back */
  merge(tmp, b, n1, b + n1 * size, n2, size, compar);
  memcpy(b, tmp, nmemb * size);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  if (nmemb < 2)
    return;

  char *tmp = (char *)malloc(nmemb * size);
  if (!tmp) {
    /* Fallback to insertion sort if malloc fails */
    insertion_sort(base, nmemb, size, compar);
    return;
  }

  msort_with_tmp(base, nmemb, size, compar, tmp);
  free(tmp);
}

/* Also provide qsort_r for completeness */
#ifdef __APPLE__
/* macOS qsort_r has a different signature from glibc:
 * macOS: qsort_r(base, nmemb, size, thunk, compar(thunk, a, b))
 * glibc: qsort_r(base, nmemb, size, compar(a, b, thunk), thunk)
 * We implement the macOS signature since we're overriding on macOS.
 */
static void insertion_sort_r(void *base, size_t nmemb, size_t size, void *thunk,
                             int (*compar)(void *, const void *,
                                           const void *)) {
  char *b = (char *)base;
  char *tmp = (char *)malloc(size);
  if (!tmp)
    return;

  for (size_t i = 1; i < nmemb; i++) {
    char *key = b + i * size;
    size_t j = i;
    while (j > 0 && compar(thunk, b + (j - 1) * size, key) > 0) {
      j--;
    }
    if (j != i) {
      memcpy(tmp, key, size);
      memmove(b + (j + 1) * size, b + j * size, (i - j) * size);
      memcpy(b + j * size, tmp, size);
    }
  }
  free(tmp);
}

static void merge_r(char *dst, const char *src1, size_t n1, const char *src2,
                    size_t n2, size_t size, void *thunk,
                    int (*compar)(void *, const void *, const void *)) {
  while (n1 > 0 && n2 > 0) {
    if (compar(thunk, src1, src2) <= 0) {
      memcpy(dst, src1, size);
      src1 += size;
      n1--;
    } else {
      memcpy(dst, src2, size);
      src2 += size;
      n2--;
    }
    dst += size;
  }
  if (n1 > 0)
    memcpy(dst, src1, n1 * size);
  if (n2 > 0)
    memcpy(dst, src2, n2 * size);
}

static void msort_with_tmp_r(void *base, size_t nmemb, size_t size, void *thunk,
                             int (*compar)(void *, const void *, const void *),
                             char *tmp) {
  char *b = (char *)base;

  if (nmemb <= INSERTION_THRESHOLD) {
    insertion_sort_r(base, nmemb, size, thunk, compar);
    return;
  }

  size_t n1 = nmemb / 2;
  size_t n2 = nmemb - n1;

  msort_with_tmp_r(b, n1, size, thunk, compar, tmp);
  msort_with_tmp_r(b + n1 * size, n2, size, thunk, compar, tmp);

  merge_r(tmp, b, n1, b + n1 * size, n2, size, thunk, compar);
  memcpy(b, tmp, nmemb * size);
}

void qsort_r(void *base, size_t nmemb, size_t size, void *thunk,
             int (*compar)(void *, const void *, const void *)) {
  if (nmemb < 2)
    return;

  char *tmp = (char *)malloc(nmemb * size);
  if (!tmp) {
    insertion_sort_r(base, nmemb, size, thunk, compar);
    return;
  }

  msort_with_tmp_r(base, nmemb, size, thunk, compar, tmp);
  free(tmp);
}
#endif
