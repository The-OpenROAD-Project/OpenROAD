////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string>
#include <algorithm>
#include "stt/flute.h"

namespace stt {
void Tree::printTree(utl::Logger* logger)
{
  for (int i = 0; i < deg; i++)
    logger->report(" {:2d}:  x={:4g}  y={:4g}  e={}",
           i, (float)branch[i].x, (float)branch[i].y, branch[i].n);
  for (int i = deg; i < 2 * deg - 2; i++)
    logger->report("s{:2d}:  x={:4g}  y={:4g}  e={}",
           i, (float)branch[i].x, (float)branch[i].y, branch[i].n);
  logger->report("");
}

namespace flt {

#if FLUTE_D <= 7
#define MGROUP 5040 / 4  // Max. # of groups, 7! = 5040
#define MPOWV 15         // Max. # of POWVs per group
#elif FLUTE_D == 8
#define MGROUP 40320 / 4  // Max. # of groups, 8! = 40320
#define MPOWV 33          // Max. # of POWVs per group
#elif FLUTE_D == 9
#define MGROUP 362880 / 4  // Max. # of groups, 9! = 362880
#define MPOWV 79           // Max. # of POWVs per group
#endif
int numgrp[10] = {0, 0, 0, 0, 6, 30, 180, 1260, 10080, 90720};

struct csoln {
  unsigned char parent;
  unsigned char seg[11];        // Add: 0..i, Sub: j..10; seg[i+1]=seg[j-1]=0
  unsigned char rowcol[FLUTE_D - 2];  // row = rowcol[]/16, col = rowcol[]%16,
  unsigned char neighbor[2 * FLUTE_D - 2];
};

// struct csoln *LUT[FLUTE_D + 1][MGROUP];  // storing 4 .. FLUTE_D
// int numsoln[FLUTE_D + 1][MGROUP];

typedef struct csoln ***LUT_TYPE;
typedef int **NUMSOLN_TYPE;

// Dynamically allocate LUTs.
LUT_TYPE LUT;
NUMSOLN_TYPE numsoln;

struct point {
  DTYPE x, y;
  int o;
};

Tree dmergetree(Tree t1, Tree t2);
Tree hmergetree(Tree t1, Tree t2, int s[]);
Tree vmergetree(Tree t1, Tree t2);
void local_refinement(int deg, Tree *tp, int p);

template <class T> inline T ADIFF(T x, T y) {
  if (x > y) {
    return (x - y);
  } else {
    return (y - x);
  }
}

////////////////////////////////////////////////////////////////

#if LUT_SOURCE==LUT_FILE || LUT_SOURCE==LUT_VAR_CHECK
static void
readLUTfiles(LUT_TYPE LUT,
	     NUMSOLN_TYPE numsoln) {
  unsigned char charnum[256], line[32], *linep, c;
  FILE *fpwv, *fprt;
  struct csoln *p;
  int d, i, j, k, kk, ns, nn;

  for (i = 0; i <= 255; i++) {
    if ('0' <= i && i <= '9')
      charnum[i] = i - '0';
    else if (i >= 'A')
      charnum[i] = i - 'A' + 10;
    else  // if (i=='$' || i=='\n' || ... )
      charnum[i] = 0;
  }

  fpwv = fopen(FLUTE_POWVFILE, "r");
  if (fpwv == NULL) {
    printf("Error in opening %s\n", FLUTE_POWVFILE);
    exit(1);
  }

#if FLUTE_ROUTING == 1
  fprt = fopen(FLUTE_POSTFILE, "r");
  if (fprt == NULL) {
    printf("Error in opening %s\n", FLUTE_POSTFILE);
    exit(1);
  }
#endif

  for (d = 4; d <= FLUTE_D; d++) {
    fscanf(fpwv, "d=%d", &d);
    fgetc(fpwv);    // '/n'
#if FLUTE_ROUTING == 1
    fscanf(fprt, "d=%d", &d);
    fgetc(fprt);    // '/n'
#endif
    for (k = 0; k < numgrp[d]; k++) {
      ns = (int)charnum[fgetc(fpwv)];

      if (ns == 0) {  // same as some previous group
        fscanf(fpwv, "%d", &kk);
        fgetc(fpwv); // '/n'
        numsoln[d][k] = numsoln[d][kk];
        LUT[d][k] = LUT[d][kk];
      } else {
        fgetc(fpwv);  // '\n'
        numsoln[d][k] = ns;
        p = (struct csoln *)malloc(ns * sizeof(struct csoln));
        LUT[d][k] = p;
        for (i = 1; i <= ns; i++) {
          linep = (unsigned char *)fgets((char *)line, 32, fpwv);
          p->parent = charnum[*(linep++)];
          j = 0;
          while ((p->seg[j++] = charnum[*(linep++)]) != 0)
            ;
          j = 10;
          while ((p->seg[j--] = charnum[*(linep++)]) != 0)
            ;
#if FLUTE_ROUTING == 1
          nn = 2 * d - 2;
          fread(line, 1, d - 2, fprt);
          linep = line;
          for (j = d; j < nn; j++) {
            c = charnum[*(linep++)];
            p->rowcol[j - d] = c;
          }
          fread(line, 1, nn / 2 + 1, fprt);
          linep = line;  // last char \n
          for (j = 0; j < nn;) {
            c = *(linep++);
            p->neighbor[j++] = c / 16;
            p->neighbor[j++] = c % 16;
          }
#endif
          p++;
        }
      }
    }
  }
  fclose(fpwv);
#if FLUTE_ROUTING == 1
  fclose(fprt);
#endif
}
#endif

////////////////////////////////////////////////////////////////

static void
makeLUT(LUT_TYPE &LUT,
	NUMSOLN_TYPE &numsoln);
static void
deleteLUT(LUT_TYPE &LUT,
	  NUMSOLN_TYPE &numsoln);
static void
initLUT(int to_d,
        LUT_TYPE LUT,
	NUMSOLN_TYPE numsoln);
static void
ensureLUT(int d);
static std::string
base64_decode(std::string const& encoded_string);
#if LUT_SOURCE==LUT_VAR_CHECK
static void
checkLUT(LUT_TYPE LUT1,
	 NUMSOLN_TYPE numsoln1,
	 LUT_TYPE LUT2,
	 NUMSOLN_TYPE numsoln2);
#endif

// LUTs are initialized to this order at startup.
static constexpr int lut_initial_d = 8;
static int lut_valid_d = 0;

// Use flute LUT file reader.
#define LUT_FILE 1
// Init LUTs from base64 encoded string variables.
#define LUT_VAR 2
// Init LUTs from base64 encoded string variables
// and check against LUTs from file reader.
#define LUT_VAR_CHECK 3

// Set this to LUT_FILE, LUT_VAR, or LUT_VAR_CHECK.
//#define LUT_SOURCE LUT_FILE
//#define LUT_SOURCE LUT_VAR_CHECK
#define LUT_SOURCE LUT_VAR

extern std::string post9;
extern std::string powv9;

void readLUT() {
  makeLUT(LUT, numsoln);

#if LUT_SOURCE==LUT_FILE
  readLUTfiles(LUT, numsoln);
  lut_valid_d = FLUTE_D;

#elif LUT_SOURCE==LUT_VAR
  // Only init to d=8 on startup because d=9 is big and slow.
  initLUT(lut_initial_d, LUT, numsoln);

#elif LUT_SOURCE==LUT_VAR_CHECK
  readLUTfiles(LUT, numsoln);
  // Temporaries to compare to file results.
  LUT_TYPE LUT_;
  NUMSOLN_TYPE numsoln_;
  makeLUT(LUT_, numsoln_);
  initLUT(FLUTE_D, LUT_, numsoln_);
  checkLUT(LUT, numsoln, LUT_, numsoln_);
#endif
}

static void
makeLUT(LUT_TYPE &LUT,
	NUMSOLN_TYPE &numsoln)
{
  LUT = new struct csoln **[FLUTE_D + 1];
  numsoln = new int*[FLUTE_D + 1];
  for (int d = 4; d <= FLUTE_D; d++) {
    LUT[d] = new struct csoln *[MGROUP];
    numsoln[d] = new int[MGROUP];
  }
}

void
deleteLUT()
{
  deleteLUT(LUT, numsoln);
}

static void
deleteLUT(LUT_TYPE &LUT,
	  NUMSOLN_TYPE &numsoln)
{
  for (int d = 4; d <= FLUTE_D; d++) {
    delete [] LUT[d];
    delete [] numsoln[d];
  }
  delete [] numsoln;
  delete [] LUT;
}

static unsigned char
charNum(unsigned char c)
{
  if (isdigit(c))
    return c - '0';
  else if (c >= 'A')
    return c - 'A' + 10;
  else
    return 0;
}

// Init LUTs from base64 encoded string variables.
static void
initLUT(int to_d,
        LUT_TYPE LUT,
	NUMSOLN_TYPE numsoln) {
  std::string pwv_string = base64_decode(powv9);
  const char *pwv = pwv_string.c_str();

#if FLUTE_ROUTING == 1
  std::string prt_string = base64_decode(post9);
  const char *prt = prt_string.c_str();
#endif

  for (int d = 4; d <= to_d; d++) {
    int char_cnt;
    sscanf(pwv, "d=%d%n", &d, &char_cnt);
    pwv += char_cnt + 1;
#if FLUTE_ROUTING == 1
    sscanf(prt, "d=%d%n", &d, &char_cnt);
    prt += char_cnt + 1;
#endif
    for (int k = 0; k < numgrp[d]; k++) {
      int ns = charNum(*pwv++);
      if (ns == 0) {  // same as some previous group
	int kk;
	sscanf(pwv, "%d%n", &kk, &char_cnt);
	pwv += char_cnt + 1;
	numsoln[d][k] = numsoln[d][kk];
	LUT[d][k] = LUT[d][kk];
      } else {
	pwv++;   // '\n'
	numsoln[d][k] = ns;
	struct csoln *p = new struct csoln[ns];
	LUT[d][k] = p;
	for (int i = 1; i <= ns; i++) {
	  p->parent = charNum(*pwv++);

	  int j = 0;
	  unsigned char ch, seg;
	  do {
	    ch = *pwv++;
	    seg = charNum(ch);
	    p->seg[j++] = seg;
	  } while (seg != 0);

	  j = 10;
	  if (ch == '\n')
	    p->seg[j] = 0;
	  else {
	    do {
	      ch = *pwv++;
	      seg = charNum(ch);
	      p->seg[j--] = seg;
	    } while (seg != 0);
	  }

#if FLUTE_ROUTING == 1
	  int nn = 2 * d - 2;
	  for (int j = d; j < nn; j++)
	    p->rowcol[j - d] = charNum(*prt++);

	  for (int j = 0; j < nn;) {
	    unsigned char c = *prt++;
	    p->neighbor[j++] = c / 16;
	    p->neighbor[j++] = c % 16;
	  }
	  prt++;  // \n
#endif
	  p++;
	}
      }
    }
  }
  lut_valid_d = to_d;
}

static void
ensureLUT(int d) {
  if (d > lut_valid_d && d <= FLUTE_D) {
    initLUT(FLUTE_D, LUT, numsoln);
  }
}

#if LUT_SOURCE==LUT_VAR_CHECK
static void
checkLUT(LUT_TYPE LUT1,
	 NUMSOLN_TYPE numsoln1,
	 LUT_TYPE LUT2,
	 NUMSOLN_TYPE numsoln2) {
  for (int d = 4; d <= FLUTE_D; d++) {
    for (int k = 0; k < numgrp[d]; k++) {
      int ns1 = numsoln1[d][k];
      int ns2 = numsoln2[d][k];
      if (ns1 != ns2)
	printf("numsoln[%d][%d] mismatch\n", d, k);
      struct csoln *soln1 = LUT1[d][k];
      struct csoln *soln2 = LUT2[d][k];
      if (soln1->parent != soln2->parent)
	printf("LUT[%d][%d]->parent mismatch\n", d, k);
      for (int j = 0; soln1->seg[j] != 0; j++) {
	if (soln1->seg[j] != soln2->seg[j])
	  printf("LUT[%d][%d]->seg[%d] mismatch\n", d, k, j);
      }
      for (int j = 10; soln1->seg[j] != 0; j--) {
	if (soln1->seg[j] != soln2->seg[j])
	  printf("LUT[%d][%d]->seg[%d] mismatch\n", d, k, j);
      }
      int nn = 2 * d - 2;
      for (int j = d; j < nn; j++) {
	if (soln1->rowcol[j - d] != soln2->rowcol[j - d])
	  printf("LUT[%d][%d]->rowcol[%d] mismatch\n", d, k, j);
      }
      for (int j = 0; j < nn; j++) {
	if (soln1->neighbor[j] != soln2->neighbor[j])
	  printf("LUT[%d][%d]->neighbor[%d] mismatch\n", d, k, j);
      }
    }
  }
}
#endif

/* 
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
   claim that you wrote the original source code. If you use this source code
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

static const std::string base64_chars = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::string
base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

////////////////////////////////////////////////////////////////

DTYPE flute_wl(int d, DTYPE x[], DTYPE y[], int acc) {
  DTYPE minval, l, xu, xl, yu, yl;
  DTYPE *xs, *ys;
  int i, j, minidx;
  int *s;
  struct point **ptp, *tmpp;
  struct point *pt;
        
  /* allocate the dynamic pieces on the heap rather than the stack */
  xs = (DTYPE *)malloc(sizeof(DTYPE) * (d));
  ys = (DTYPE *)malloc(sizeof(DTYPE) * (d));
  s = (int *)malloc(sizeof(int) * (d));
  pt = (struct point *)malloc(sizeof(struct point) * (d+1));
  ptp = (struct point **)malloc(sizeof(struct point *) * (d+1));

  if (d == 2)
    l = ADIFF(x[0], x[1]) + ADIFF(y[0], y[1]);
  else if (d == 3) {
    if (x[0] > x[1]) {
      xu = std::max(x[0], x[2]);
      xl = std::min(x[1], x[2]);
    } else {
      xu = std::max(x[1], x[2]);
      xl = std::min(x[0], x[2]);
    }
    if (y[0] > y[1]) {
      yu = std::max(y[0], y[2]);
      yl = std::min(y[1], y[2]);
    } else {
      yu = std::max(y[1], y[2]);
      yl = std::min(y[0], y[2]);
    }
    l = (xu - xl) + (yu - yl);
  } else {
    ensureLUT(d);
                
    for (i = 0; i < d; i++) {
      pt[i].x = x[i];
      pt[i].y = y[i];
      ptp[i] = &pt[i];
    }

    // sort x
    for (i = 0; i < d - 1; i++) {
      minval = ptp[i]->x;
      minidx = i;
      for (j = i + 1; j < d; j++) {
        if (minval > ptp[j]->x) {
          minval = ptp[j]->x;
          minidx = j;
        }
      }
      tmpp = ptp[i];
      ptp[i] = ptp[minidx];
      ptp[minidx] = tmpp;
    }

#if FLUTE_REMOVE_DUPLICATE_PIN == 1
    ptp[d] = &pt[d];
    ptp[d]->x = ptp[d]->y = -999999;
    j = 0;
    for (i = 0; i < d; i++) {
      for (k = i + 1; ptp[k]->x == ptp[i]->x; k++)
        if (ptp[k]->y == ptp[i]->y)  // pins k and i are the same
          break;
      if (ptp[k]->x != ptp[i]->x)
        ptp[j++] = ptp[i];
    }
    d = j;
#endif

    for (i = 0; i < d; i++) {
      xs[i] = ptp[i]->x;
      ptp[i]->o = i;
    }

    // sort y to find s[]
    for (i = 0; i < d - 1; i++) {
      minval = ptp[i]->y;
      minidx = i;
      for (j = i + 1; j < d; j++) {
        if (minval > ptp[j]->y) {
          minval = ptp[j]->y;
          minidx = j;
        }
      }
      ys[i] = ptp[minidx]->y;
      s[i] = ptp[minidx]->o;
      ptp[minidx] = ptp[i];
    }
    ys[d - 1] = ptp[d - 1]->y;
    s[d - 1] = ptp[d - 1]->o;

    l = flutes_wl(d, xs, ys, s, acc);
  }
  free( xs ) ;
  free( ys ) ;
  free( s ) ;
  free( pt ) ;
  free( ptp ) ;

  return l;
}

// xs[] and ys[] are coords in x and y in sorted order
// s[] is a list of nodes in increasing y direction
//   if nodes are indexed in the order of increasing x coord
//   i.e., s[i] = s_i as defined in paper
// The points are (xs[s[i]], ys[i]) for i=0..d-1
//             or (xs[i], ys[si[i]]) for i=0..d-1

DTYPE flutes_wl_RDP(int d, DTYPE xs[], DTYPE ys[], int s[], int acc) {
  int i, j, ss;

  ensureLUT(d);

  for (i = 0; i < d - 1; i++) {
    if (xs[s[i]] == xs[s[i + 1]] && ys[i] == ys[i + 1]) {
      if (s[i] < s[i + 1])
        ss = s[i + 1];
      else {
        ss = s[i];
        s[i] = s[i + 1];
      }
      for (j = i + 2; j < d; j++) {
        ys[j - 1] = ys[j];
        s[j - 1] = s[j];
      }
      for (j = ss + 1; j < d; j++)
        xs[j - 1] = xs[j];
      for (j = 0; j <= d - 2; j++)
        if (s[j] > ss) s[j]--;
      i--;
      d--;
    }
  }
  return flutes_wl_ALLD(d, xs, ys, s, acc);
}

// For low-degree, i.e., 2 <= d <= FLUTE_D
DTYPE flutes_wl_LD(int d, DTYPE xs[], DTYPE ys[], int s[]) {
  int k, pi, i, j;
  struct csoln *rlist;
  DTYPE dd[2 * FLUTE_D - 2];  // 0..FLUTE_D-2 for v, FLUTE_D-1..2*D-3 for h
  DTYPE minl, sum, l[MPOWV + 1];

  if (d <= 3)
    minl = xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
  else {
    ensureLUT(d);
                               
    k = 0;
    if (s[0] < s[2]) k++;
    if (s[1] < s[2]) k++;

    for (i = 3; i <= d - 1; i++) {  // p0=0 always, skip i=1 for symmetry
      pi = s[i];
      for (j = d - 1; j > i; j--)
        if (s[j] < s[i])
          pi--;
      k = pi + (i + 1) * k;
    }

    if (k < numgrp[d])  // no horizontal flip
      for (i = 1; i <= d - 3; i++) {
        dd[i] = ys[i + 1] - ys[i];
        dd[d - 1 + i] = xs[i + 1] - xs[i];
      }
    else {
      k = 2 * numgrp[d] - 1 - k;
      for (i = 1; i <= d - 3; i++) {
        dd[i] = ys[i + 1] - ys[i];
        dd[d - 1 + i] = xs[d - 1 - i] - xs[d - 2 - i];
      }
    }

    minl = l[0] = xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
    rlist = LUT[d][k];
    for (i = 0; rlist->seg[i] > 0; i++)
      minl += dd[rlist->seg[i]];

    l[1] = minl;
    j = 2;
    while (j <= numsoln[d][k]) {
      rlist++;
      sum = l[rlist->parent];
      for (i = 0; rlist->seg[i] > 0; i++)
        sum += dd[rlist->seg[i]];
      for (i = 10; rlist->seg[i] > 0; i--)
        sum -= dd[rlist->seg[i]];
      minl = std::min(minl, sum);
      l[j++] = sum;
    }
  }

  return minl;
}

// For medium-degree, i.e., FLUTE_D+1 <= d
DTYPE flutes_wl_MD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc) {
  float pnlty, dx, dy;
  float *score, *penalty;
  DTYPE xydiff;
  DTYPE ll, minl;
  DTYPE extral = 0;
  DTYPE *x1, *x2, *y1, *y2;
  DTYPE *distx, *disty;
  int i, r, p, maxbp, nbp, bp, ub, lb, n1, n2, newacc;
  int ms, mins, maxs, minsi, maxsi, degree;
  int return_val;
  int *si, *s1, *s2;
        
  degree = d + 1;
  score = (float *)malloc(sizeof(float) * (2 * degree));
  penalty = (float *)malloc(sizeof(float) * (degree));
        
  x1 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  x2 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  y1 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  y2 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  distx = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  disty = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  si = (int *)malloc(sizeof(int) * (degree));
  s1 = (int *)malloc(sizeof(int) * (degree));
  s2 = (int *)malloc(sizeof(int) * (degree));

  ensureLUT(d);

  if (s[0] < s[d - 1]) {
    ms = std::max(s[0], s[1]);
    for (i = 2; i <= ms; i++)
      ms = std::max(ms, s[i]);
    if (ms <= d - 3) {
      for (i = 0; i <= ms; i++) {
        x1[i] = xs[i];
        y1[i] = ys[i];
        s1[i] = s[i];
      }
      x1[ms + 1] = xs[ms];
      y1[ms + 1] = ys[ms];
      s1[ms + 1] = ms + 1;

      s2[0] = 0;
      for (i = 1; i <= d - 1 - ms; i++)
        s2[i] = s[i + ms] - ms;

      return_val = flutes_wl_LMD(ms + 2, x1, y1, s1, acc) +
        flutes_wl_LMD(d - ms, xs + ms, ys + ms, s2, acc);
      free(score);
      free(penalty);
      free(x1);
      free(x2);
      free(y1);
      free(y2);
      free(distx);
      free(disty);
      free(si);
      free(s1);
      free(s2);
                        
      return return_val;
    }
  } else {  // (s[0] > s[d-1])
    ms = std::min(s[0], s[1]);
    for (i = 2; i <= d - 1 - ms; i++)
      ms = std::min(ms, s[i]);
    if (ms >= 2) {
      x1[0] = xs[ms];
      y1[0] = ys[0];
      s1[0] = s[0] - ms + 1;
      for (i = 1; i <= d - 1 - ms; i++) {
        x1[i] = xs[i + ms - 1];
        y1[i] = ys[i];
        s1[i] = s[i] - ms + 1;
      }
      x1[d - ms] = xs[d - 1];
      y1[d - ms] = ys[d - 1 - ms];
      s1[d - ms] = 0;

      s2[0] = ms;
      for (i = 1; i <= ms; i++)
        s2[i] = s[i + d - 1 - ms];

      return_val = flutes_wl_LMD(d + 1 - ms, x1, y1, s1, acc) +
        flutes_wl_LMD(ms + 1, xs, ys + d - 1 - ms, s2, acc);
      free(score);
      free(penalty);
      free(x1);
      free(x2);
      free(y1);
      free(y2);
      free(distx);
      free(disty);
      free(si);
      free(s1);
      free(s2);
      return return_val;
    }
  }

  // Find inverse si[] of s[]
  for (r = 0; r < d; r++)
    si[s[r]] = r;

  // Determine breaking directions and positions dp[]
  lb = (d - 2 * acc + 2) / 4;
  if (lb < 2) lb = 2;
  ub = d - 1 - lb;

  // Compute scores
#define AAWL 0.6
#define BBWL 0.3
  float CCWL = 7.4 / ((d + 10.) * (d - 3.));
  float DDWL = 4.8 / (d - 1);

  // Compute penalty[]
  dx = CCWL * (xs[d - 2] - xs[1]);
  dy = CCWL * (ys[d - 2] - ys[1]);
  for (r = d / 2, pnlty = 0; r >= 0; r--, pnlty += dx)
    penalty[r] = pnlty, penalty[d - 1 - r] = pnlty;
  for (r = d / 2 - 1, pnlty = dy; r >= 0; r--, pnlty += dy)
    penalty[s[r]] += pnlty, penalty[s[d - 1 - r]] += pnlty;
  //#define CCWL 0.16
  //    for (r=0; r<d; r++)
  //        penalty[r] = abs(d-1-r-r)*dx + abs(d-1-si[r]-si[r])*dy;

  // Compute distx[], disty[]
  xydiff = (xs[d - 1] - xs[0]) - (ys[d - 1] - ys[0]);
  if (s[0] < s[1])
    mins = s[0], maxs = s[1];
  else
    mins = s[1], maxs = s[0];
  if (si[0] < si[1])
    minsi = si[0], maxsi = si[1];
  else
    minsi = si[1], maxsi = si[0];
  for (r = 2; r <= ub; r++) {
    if (s[r] < mins)
      mins = s[r];
    else if (s[r] > maxs)
      maxs = s[r];
    distx[r] = xs[maxs] - xs[mins];
    if (si[r] < minsi)
      minsi = si[r];
    else if (si[r] > maxsi)
      maxsi = si[r];
    disty[r] = ys[maxsi] - ys[minsi] + xydiff;
  }

  if (s[d - 2] < s[d - 1])
    mins = s[d - 2], maxs = s[d - 1];
  else
    mins = s[d - 1], maxs = s[d - 2];
  if (si[d - 2] < si[d - 1])
    minsi = si[d - 2], maxsi = si[d - 1];
  else
    minsi = si[d - 1], maxsi = si[d - 2];
  for (r = d - 3; r >= lb; r--) {
    if (s[r] < mins)
      mins = s[r];
    else if (s[r] > maxs)
      maxs = s[r];
    distx[r] += xs[maxs] - xs[mins];
    if (si[r] < minsi)
      minsi = si[r];
    else if (si[r] > maxsi)
      maxsi = si[r];
    disty[r] += ys[maxsi] - ys[minsi];
  }

  nbp = 0;
  for (r = lb; r <= ub; r++) {
    if (si[r] == 0 || si[r] == d - 1)
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
        - AAWL * (ys[d - 2] - ys[1]) - DDWL * disty[r];
    else score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r]
           - BBWL * (ys[si[r] + 1] - ys[si[r] - 1]) - DDWL * disty[r];
    nbp++;

    if (s[r] == 0 || s[r] == d - 1)
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
        - AAWL * (xs[d - 2] - xs[1]) - DDWL * distx[r];
    else score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]]
           - BBWL * (xs[s[r] + 1] - xs[s[r] - 1]) - DDWL * distx[r];
    nbp++;
  }

  if (acc <= 3)
    newacc = 1;
  else {
    newacc = acc / 2;
    if (acc >= nbp) acc = nbp - 1;
  }

  minl = (DTYPE)INT_MAX;
  for (i = 0; i < acc; i++) {
    maxbp = 0;
    for (bp = 1; bp < nbp; bp++)
      if (score[maxbp] < score[bp]) maxbp = bp;
    score[maxbp] = -9e9;

#define BreakPt(bp) ((bp) / 2 + lb)
#define BreakInX(bp) ((bp) % 2 == 0)
    p = BreakPt(maxbp);
    // Breaking in p
    if (BreakInX(maxbp)) {  // break in x
      n1 = n2 = 0;
      for (r = 0; r < d; r++) {
        if (s[r] < p) {
          s1[n1] = s[r];
          y1[n1] = ys[r];
          n1++;
        } else if (s[r] > p) {
          s2[n2] = s[r] - p;
          y2[n2] = ys[r];
          n2++;
        } else {  // if (s[r] == p)  i.e.,  r = si[p]
          s1[n1] = p;
          s2[n2] = 0;
          if (r == d - 1 || r == d - 2) {
            y1[n1] = y2[n2] = ys[r - 1];
            extral = ys[r] - ys[r - 1];
          } else if (r == 0 || r == 1) {
            y1[n1] = y2[n2] = ys[r + 1];
            extral = ys[r + 1] - ys[r];
          } else {
            y1[n1] = y2[n2] = ys[r];
            extral = 0;
          }
          n1++;
          n2++;
        }
      }
      ll = extral + flutes_wl_LMD(p + 1, xs, y1, s1, newacc) +
        flutes_wl_LMD(d - p, xs + p, y2, s2, newacc);
    } else {  // if (!BreakInX(maxbp))
      n1 = n2 = 0;
      for (r = 0; r < d; r++) {
        if (si[r] < p) {
          s1[si[r]] = n1;
          x1[n1] = xs[r];
          n1++;
        } else if (si[r] > p) {
          s2[si[r] - p] = n2;
          x2[n2] = xs[r];
          n2++;
        } else {  // if (si[r] == p)  i.e.,  r = s[p]
          s1[p] = n1;
          s2[0] = n2;
          if (r == d - 1 || r == d - 2) {
            x1[n1] = x2[n2] = xs[r - 1];
            extral = xs[r] - xs[r - 1];
          } else if (r == 0 || r == 1) {
            x1[n1] = x2[n2] = xs[r + 1];
            extral = xs[r + 1] - xs[r];
          } else {
            x1[n1] = x2[n2] = xs[r];
            extral = 0;
          }
          n1++;
          n2++;
        }
      }
      ll = extral + flutes_wl_LMD(p + 1, x1, ys, s1, newacc) +
        flutes_wl_LMD(d - p, x2, ys + p, s2, newacc);
    }
    if (minl > ll) minl = ll;
  }
  return_val = minl;
        
  free(score);
  free(penalty);
  free(x1);
  free(x2);
  free(y1);
  free(y2);
  free(distx);
  free(disty);
  free(si);
  free(s1);
  free(s2);
  return return_val;
}

static int orderx(const void *a, const void *b) {
  struct point *pa, *pb;

  pa = *(struct point **)a;
  pb = *(struct point **)b;

  if (pa->x < pb->x) return -1;
  if (pa->x > pb->x) return 1;
  return 0;
}

static int ordery(const void *a, const void *b) {
  struct point *pa, *pb;

  pa = *(struct point **)a;
  pb = *(struct point **)b;

  if (pa->y < pb->y) return -1;
  if (pa->y > pb->y) return 1;
  return 0;
}

Tree flute(int d, DTYPE x[], DTYPE y[], int acc) {
  DTYPE *xs, *ys, minval;
  int *s;
  int i, j, minidx;
  struct point *pt, **ptp, *tmpp;
  Tree t;

  if (d == 2) {
    t.deg = 2;
    t.length = ADIFF(x[0], x[1]) + ADIFF(y[0], y[1]);
    t.branch.resize(2);
    t.branch[0].x = x[0];
    t.branch[0].y = y[0];
    t.branch[0].n = 1;
    t.branch[1].x = x[1];
    t.branch[1].y = y[1];
    t.branch[1].n = 1;
  } else {
    ensureLUT(d);
                
    xs = (DTYPE *)malloc(sizeof(DTYPE) * (d));
    ys = (DTYPE *)malloc(sizeof(DTYPE) * (d));
    s = (int *)malloc(sizeof(int) * (d));
    pt = (struct point *)malloc(sizeof(struct point) * (d + 1));
    std::vector<point*> ptp(d+1);

    for (i = 0; i < d; i++) {
      pt[i].x = x[i];
      pt[i].y = y[i];
      ptp[i] = &pt[i];
    }

    // sort x
    if (d < 200) {
      for (i = 0; i < d - 1; i++) {
        minval = ptp[i]->x;
        minidx = i;
        for (j = i + 1; j < d; j++) {
          if (minval > ptp[j]->x) {
            minval = ptp[j]->x;
            minidx = j;
          }
        }
        tmpp = ptp[i];
        ptp[i] = ptp[minidx];
        ptp[minidx] = tmpp;
      }
    } else {
      std::stable_sort(ptp.begin(), ptp.end(), orderx);
    }

#if FLUTE_REMOVE_DUPLICATE_PIN == 1
    ptp[d] = &pt[d];
    ptp[d]->x = ptp[d]->y = -999999;
    j = 0;
    for (i = 0; i < d; i++) {
      for (k = i + 1; ptp[k]->x == ptp[i]->x; k++)
        if (ptp[k]->y == ptp[i]->y)  // pins k and i are the same
          break;
      if (ptp[k]->x != ptp[i]->x)
        ptp[j++] = ptp[i];
    }
    d = j;
#endif

    for (i = 0; i < d; i++) {
      xs[i] = ptp[i]->x;
      ptp[i]->o = i;
    }

    // sort y to find s[]
    if (d < 200) {
      for (i = 0; i < d - 1; i++) {
        minval = ptp[i]->y;
        minidx = i;
        for (j = i + 1; j < d; j++) {
          if (minval > ptp[j]->y) {
            minval = ptp[j]->y;
            minidx = j;
          }
        }
        ys[i] = ptp[minidx]->y;
        s[i] = ptp[minidx]->o;
        ptp[minidx] = ptp[i];
      }
      ys[d - 1] = ptp[d - 1]->y;
      s[d - 1] = ptp[d - 1]->o;
    } else {
      std::stable_sort(ptp.begin(), ptp.end(), ordery);
      for (i = 0; i < d; i++) {
        ys[i] = ptp[i]->y;
        s[i] = ptp[i]->o;
      }
    }

    t = flutes(d, xs, ys, s, acc);

    free(xs);
    free(ys);
    free(s);
    free(pt);
  }

  return t;
}

// xs[] and ys[] are coords in x and y in sorted order
// s[] is a list of nodes in increasing y direction
//   if nodes are indexed in the order of increasing x coord
//   i.e., s[i] = s_i as defined in paper
// The points are (xs[s[i]], ys[i]) for i=0..d-1
//             or (xs[i], ys[si[i]]) for i=0..d-1

Tree flutes_RDP(int d, DTYPE xs[], DTYPE ys[], int s[], int acc) {
  int i, j, ss;

  ensureLUT(d);
                
  for (i = 0; i < d - 1; i++) {
    if (xs[s[i]] == xs[s[i + 1]] && ys[i] == ys[i + 1]) {
      if (s[i] < s[i + 1])
        ss = s[i + 1];
      else {
        ss = s[i];
        s[i] = s[i + 1];
      }
      for (j = i + 2; j < d; j++) {
        ys[j - 1] = ys[j];
        s[j - 1] = s[j];
      }
      for (j = ss + 1; j < d; j++)
        xs[j - 1] = xs[j];
      for (j = 0; j <= d - 2; j++)
        if (s[j] > ss) s[j]--;
      i--;
      d--;
    }
  }
  return flutes_ALLD(d, xs, ys, s, acc);
}

// For low-degree, i.e., 2 <= d <= FLUTE_D
Tree flutes_LD(int d, DTYPE xs[], DTYPE ys[], int s[]) {
  int k, pi, i, j;
  struct csoln *rlist, *bestrlist;
  DTYPE dd[2 * FLUTE_D - 2];  // 0..D-2 for v, D-1..2*D-3 for h
  DTYPE minl, sum, l[MPOWV + 1];
  int hflip;
  Tree t;

  t.deg = d;
  t.branch.resize(2 * d - 2);
  if (d == 2) {
    minl = xs[1] - xs[0] + ys[1] - ys[0];
    t.branch[0].x = xs[s[0]];
    t.branch[0].y = ys[0];
    t.branch[0].n = 1;
    t.branch[1].x = xs[s[1]];
    t.branch[1].y = ys[1];
    t.branch[1].n = 1;
  } else if (d == 3) {
    minl = xs[2] - xs[0] + ys[2] - ys[0];
    t.branch[0].x = xs[s[0]];
    t.branch[0].y = ys[0];
    t.branch[0].n = 3;
    t.branch[1].x = xs[s[1]];
    t.branch[1].y = ys[1];
    t.branch[1].n = 3;
    t.branch[2].x = xs[s[2]];
    t.branch[2].y = ys[2];
    t.branch[2].n = 3;
    t.branch[3].x = xs[1];
    t.branch[3].y = ys[1];
    t.branch[3].n = 3;
  } else {
    ensureLUT(d);
                
    k = 0;
    if (s[0] < s[2]) k++;
    if (s[1] < s[2]) k++;

    for (i = 3; i <= d - 1; i++) {  // p0=0 always, skip i=1 for symmetry
      pi = s[i];
      for (j = d - 1; j > i; j--)
        if (s[j] < s[i])
          pi--;
      k = pi + (i + 1) * k;
    }

    if (k < numgrp[d]) {  // no horizontal flip
      hflip = 0;
      for (i = 1; i <= d - 3; i++) {
        dd[i] = ys[i + 1] - ys[i];
        dd[d - 1 + i] = xs[i + 1] - xs[i];
      }
    } else {
      hflip = 1;
      k = 2 * numgrp[d] - 1 - k;
      for (i = 1; i <= d - 3; i++) {
        dd[i] = ys[i + 1] - ys[i];
        dd[d - 1 + i] = xs[d - 1 - i] - xs[d - 2 - i];
      }
    }

    minl = l[0] = xs[d - 1] - xs[0] + ys[d - 1] - ys[0];
    rlist = LUT[d][k];
    for (i = 0; rlist->seg[i] > 0; i++)
      minl += dd[rlist->seg[i]];
    bestrlist = rlist;
    l[1] = minl;
    j = 2;
    while (j <= numsoln[d][k]) {
      rlist++;
      sum = l[rlist->parent];
      for (i = 0; rlist->seg[i] > 0; i++)
        sum += dd[rlist->seg[i]];
      for (i = 10; rlist->seg[i] > 0; i--)
        sum -= dd[rlist->seg[i]];
      if (sum < minl) {
        minl = sum;
        bestrlist = rlist;
      }
      l[j++] = sum;
    }

    t.branch[0].x = xs[s[0]];
    t.branch[0].y = ys[0];
    t.branch[1].x = xs[s[1]];
    t.branch[1].y = ys[1];
    for (i = 2; i < d - 2; i++) {
      t.branch[i].x = xs[s[i]];
      t.branch[i].y = ys[i];
      t.branch[i].n = bestrlist->neighbor[i];
    }
    t.branch[d - 2].x = xs[s[d - 2]];
    t.branch[d - 2].y = ys[d - 2];
    t.branch[d - 1].x = xs[s[d - 1]];
    t.branch[d - 1].y = ys[d - 1];
    if (hflip) {
      if (s[1] < s[0]) {
        t.branch[0].n = bestrlist->neighbor[1];
        t.branch[1].n = bestrlist->neighbor[0];
      } else {
        t.branch[0].n = bestrlist->neighbor[0];
        t.branch[1].n = bestrlist->neighbor[1];
      }
      if (s[d - 1] < s[d - 2]) {
        t.branch[d - 2].n = bestrlist->neighbor[d - 1];
        t.branch[d - 1].n = bestrlist->neighbor[d - 2];
      } else {
        t.branch[d - 2].n = bestrlist->neighbor[d - 2];
        t.branch[d - 1].n = bestrlist->neighbor[d - 1];
      }
      for (i = d; i < 2 * d - 2; i++) {
        t.branch[i].x = xs[d - 1 - bestrlist->rowcol[i - d] % 16];
        t.branch[i].y = ys[bestrlist->rowcol[i - d] / 16];
        t.branch[i].n = bestrlist->neighbor[i];
      }
    } else {  // !hflip
      if (s[0] < s[1]) {
        t.branch[0].n = bestrlist->neighbor[1];
        t.branch[1].n = bestrlist->neighbor[0];
      } else {
        t.branch[0].n = bestrlist->neighbor[0];
        t.branch[1].n = bestrlist->neighbor[1];
      }
      if (s[d - 2] < s[d - 1]) {
        t.branch[d - 2].n = bestrlist->neighbor[d - 1];
        t.branch[d - 1].n = bestrlist->neighbor[d - 2];
      } else {
        t.branch[d - 2].n = bestrlist->neighbor[d - 2];
        t.branch[d - 1].n = bestrlist->neighbor[d - 1];
      }
      for (i = d; i < 2 * d - 2; i++) {
        t.branch[i].x = xs[bestrlist->rowcol[i - d] % 16];
        t.branch[i].y = ys[bestrlist->rowcol[i - d] / 16];
        t.branch[i].n = bestrlist->neighbor[i];
      }
    }
  }
  t.length = minl;

  return t;
}

// For medium-degree, i.e., FLUTE_D+1 <= d
Tree flutes_MD(int d, DTYPE xs[], DTYPE ys[], int s[], int acc) 
{
  float *score, *penalty, pnlty, dx, dy;
  int ms, mins, maxs, minsi, maxsi;
  int i, r, p, maxbp, bestbp, bp, nbp, ub, lb, n1, n2, newacc;
  int nn1 = 0;
  int nn2 = 0;
  int *si, *s1, *s2, degree;
  Tree t, t1, t2, bestt1, bestt2;
  DTYPE ll, minl, coord1, coord2;
  DTYPE *distx, *disty, xydiff;
  DTYPE *x1, *x2, *y1, *y2;
        
  degree = d + 1;
  score = (float *)malloc(sizeof(float) * (2 * degree));
  penalty = (float *)malloc(sizeof(float) * (degree));
        
  x1 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  x2 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  y1 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  y2 = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  distx = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  disty = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  si = (int *)malloc(sizeof(int) * (degree));
  s1 = (int *)malloc(sizeof(int) * (degree));
  s2 = (int *)malloc(sizeof(int) * (degree));

  if (s[0] < s[d - 1]) {
    ms = std::max(s[0], s[1]);
    for (i = 2; i <= ms; i++)
      ms = std::max(ms, s[i]);
    if (ms <= d - 3) {
      for (i = 0; i <= ms; i++) {
        x1[i] = xs[i];
        y1[i] = ys[i];
        s1[i] = s[i];
      }
      x1[ms + 1] = xs[ms];
      y1[ms + 1] = ys[ms];
      s1[ms + 1] = ms + 1;

      s2[0] = 0;
      for (i = 1; i <= d - 1 - ms; i++)
        s2[i] = s[i + ms] - ms;

      t1 = flutes_LMD(ms + 2, x1, y1, s1, acc);
      t2 = flutes_LMD(d - ms, xs + ms, ys + ms, s2, acc);
      t = dmergetree(t1, t2);
                        
      free(score);
      free(penalty);
      free(x1);
      free(x2);
      free(y1);
      free(y2);
      free(distx);
      free(disty);
      free(si);
      free(s1);
      free(s2);
                        
      return t;
    }
  } else {  // (s[0] > s[d-1])
    ms = std::min(s[0], s[1]);
    for (i = 2; i <= d - 1 - ms; i++)
      ms = std::min(ms, s[i]);
    if (ms >= 2) {
      x1[0] = xs[ms];
      y1[0] = ys[0];
      s1[0] = s[0] - ms + 1;
      for (i = 1; i <= d - 1 - ms; i++) {
        x1[i] = xs[i + ms - 1];
        y1[i] = ys[i];
        s1[i] = s[i] - ms + 1;
      }
      x1[d - ms] = xs[d - 1];
      y1[d - ms] = ys[d - 1 - ms];
      s1[d - ms] = 0;

      s2[0] = ms;
      for (i = 1; i <= ms; i++)
        s2[i] = s[i + d - 1 - ms];

      t1 = flutes_LMD(d + 1 - ms, x1, y1, s1, acc);
      t2 = flutes_LMD(ms + 1, xs, ys + d - 1 - ms, s2, acc);
      t = dmergetree(t1, t2);
                        
      free(score);
      free(penalty);
      free(x1);
      free(x2);
      free(y1);
      free(y2);
      free(distx);
      free(disty);
      free(si);
      free(s1);
      free(s2);

      return t;
    }
  }

  // Find inverse si[] of s[]
  for (r = 0; r < d; r++)
    si[s[r]] = r;

  // Determine breaking directions and positions dp[]
  lb = (d - 2 * acc + 2) / 4;
  if (lb < 2) lb = 2;
  ub = d - 1 - lb;

  // Compute scores
#define AA 0.6  // 2.0*BB
#define BB 0.3
  float CC = 7.4 / ((d + 10.) * (d - 3.));
  float DD = 4.8 / (d - 1);

  // Compute penalty[]
  dx = CC * (xs[d - 2] - xs[1]);
  dy = CC * (ys[d - 2] - ys[1]);
  for (r = d / 2, pnlty = 0; r >= 2; r--, pnlty += dx)
    penalty[r] = pnlty, penalty[d - 1 - r] = pnlty;
  penalty[1] = pnlty, penalty[d - 2] = pnlty;
  penalty[0] = pnlty, penalty[d - 1] = pnlty;
  for (r = d / 2 - 1, pnlty = dy; r >= 2; r--, pnlty += dy)
    penalty[s[r]] += pnlty, penalty[s[d - 1 - r]] += pnlty;
  penalty[s[1]] += pnlty, penalty[s[d - 2]] += pnlty;
  penalty[s[0]] += pnlty, penalty[s[d - 1]] += pnlty;
  //#define CC 0.16
  //#define v(r) ((r==0||r==1||r==d-2||r==d-1) ? d-3 : abs(d-1-r-r))
  //    for (r=0; r<d; r++)
  //        penalty[r] = v(r)*dx + v(si[r])*dy;

  // Compute distx[], disty[]
  xydiff = (xs[d - 1] - xs[0]) - (ys[d - 1] - ys[0]);
  if (s[0] < s[1])
    mins = s[0], maxs = s[1];
  else mins = s[1], maxs = s[0];
  if (si[0] < si[1])
    minsi = si[0], maxsi = si[1];
  else minsi = si[1], maxsi = si[0];
  for (r = 2; r <= ub; r++) {
    if (s[r] < mins)
      mins = s[r];
    else if (s[r] > maxs)
      maxs = s[r];
    distx[r] = xs[maxs] - xs[mins];
    if (si[r] < minsi)
      minsi = si[r];
    else if (si[r] > maxsi)
      maxsi = si[r];
    disty[r] = ys[maxsi] - ys[minsi] + xydiff;
  }

  if (s[d - 2] < s[d - 1])
    mins = s[d - 2], maxs = s[d - 1];
  else mins = s[d - 1], maxs = s[d - 2];
  if (si[d - 2] < si[d - 1])
    minsi = si[d - 2], maxsi = si[d - 1];
  else minsi = si[d - 1], maxsi = si[d - 2];
  for (r = d - 3; r >= lb; r--) {
    if (s[r] < mins)
      mins = s[r];
    else if (s[r] > maxs)
      maxs = s[r];
    distx[r] += xs[maxs] - xs[mins];
    if (si[r] < minsi)
      minsi = si[r];
    else if (si[r] > maxsi)
      maxsi = si[r];
    disty[r] += ys[maxsi] - ys[minsi];
  }

  nbp = 0;
  for (r = lb; r <= ub; r++) {
    if (si[r] <= 1)
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r] - AA * (ys[2] - ys[1]) - DD * disty[r];
    else if (si[r] >= d - 2)
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r] - AA * (ys[d - 2] - ys[d - 3]) - DD * disty[r];
    else
      score[nbp] = (xs[r + 1] - xs[r - 1]) - penalty[r] - BB * (ys[si[r] + 1] - ys[si[r] - 1]) - DD * disty[r];
    nbp++;

    if (s[r] <= 1)
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]] - AA * (xs[2] - xs[1]) - DD * distx[r];
    else if (s[r] >= d - 2)
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]] - AA * (xs[d - 2] - xs[d - 3]) - DD * distx[r];
    else
      score[nbp] = (ys[r + 1] - ys[r - 1]) - penalty[s[r]] - BB * (xs[s[r] + 1] - xs[s[r] - 1]) - DD * distx[r];
    nbp++;
  }

  if (acc <= 3)
    newacc = 1;
  else {
    newacc = acc / 2;
    if (acc >= nbp) acc = nbp - 1;
  }

  minl = (DTYPE)INT_MAX;
  for (i = 0; i < acc; i++) {
    maxbp = 0;
    for (bp = 1; bp < nbp; bp++)
      if (score[maxbp] < score[bp]) maxbp = bp;
    score[maxbp] = -9e9;

#define BreakPt(bp) ((bp) / 2 + lb)
#define BreakInX(bp) ((bp) % 2 == 0)
    p = BreakPt(maxbp);
    // Breaking in p
    if (BreakInX(maxbp)) {  // break in x
      n1 = n2 = 0;
      for (r = 0; r < d; r++) {
        if (s[r] < p) {
          s1[n1] = s[r];
          y1[n1] = ys[r];
          n1++;
        } else if (s[r] > p) {
          s2[n2] = s[r] - p;
          y2[n2] = ys[r];
          n2++;
        } else {  // if (s[r] == p)  i.e.,  r = si[p]
          s1[n1] = p;
          s2[n2] = 0;
          y1[n1] = y2[n2] = ys[r];
          nn1 = n1;
          nn2 = n2;
          n1++;
          n2++;
        }
      }

      t1 = flutes_LMD(p + 1, xs, y1, s1, newacc);
      t2 = flutes_LMD(d - p, xs + p, y2, s2, newacc);
      ll = t1.length + t2.length;
      coord1 = t1.branch[t1.branch[nn1].n].y;
      coord2 = t2.branch[t2.branch[nn2].n].y;
      if (t2.branch[nn2].y > std::max(coord1, coord2))
        ll -= t2.branch[nn2].y - std::max(coord1, coord2);
      else if (t2.branch[nn2].y < std::min(coord1, coord2))
        ll -= std::min(coord1, coord2) - t2.branch[nn2].y;
    } else {  // if (!BreakInX(maxbp))
      n1 = n2 = 0;
      for (r = 0; r < d; r++) {
        if (si[r] < p) {
          s1[si[r]] = n1;
          x1[n1] = xs[r];
          n1++;
        } else if (si[r] > p) {
          s2[si[r] - p] = n2;
          x2[n2] = xs[r];
          n2++;
        } else {  // if (si[r] == p)  i.e.,  r = s[p]
          s1[p] = n1;
          s2[0] = n2;
          x1[n1] = x2[n2] = xs[r];
          n1++;
          n2++;
        }
      }

      t1 = flutes_LMD(p + 1, x1, ys, s1, newacc);
      t2 = flutes_LMD(d - p, x2, ys + p, s2, newacc);
      ll = t1.length + t2.length;
      coord1 = t1.branch[t1.branch[p].n].x;
      coord2 = t2.branch[t2.branch[0].n].x;
      if (t2.branch[0].x > std::max(coord1, coord2))
        ll -= t2.branch[0].x - std::max(coord1, coord2);
      else if (t2.branch[0].x < std::min(coord1, coord2))
        ll -= std::min(coord1, coord2) - t2.branch[0].x;
    }
    if (minl > ll) {
      minl = ll;
      bestt1 = t1;
      bestt2 = t2;
      bestbp = maxbp;
    } else {
    }
  }

#if FLUTE_LOCAL_REFINEMENT == 1
  if (BreakInX(bestbp)) {
    t = hmergetree(bestt1, bestt2, s);
    local_refinement(degree, &t, si[BreakPt(bestbp)]);
  } else {
    t = vmergetree(bestt1, bestt2);
    local_refinement(degree, &t, BreakPt(bestbp));
  }
#else
  if (BreakInX(bestbp)) {
    t = hmergetree(bestt1, bestt2, s);
  } else {
    t = vmergetree(bestt1, bestt2);
  }
#endif


  free(score);
  free(penalty);
  free(x1);
  free(x2);
  free(y1);
  free(y2);
  free(distx);
  free(disty);
  free(si);
  free(s1);
  free(s2);
        
  return t;
}

Tree dmergetree(Tree t1, Tree t2) {
  int i, d, prev, curr, next, offset1, offset2;
  Tree t;

  t.deg = d = t1.deg + t2.deg - 2;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * d - 2);
  offset1 = t2.deg - 2;
  offset2 = 2 * t1.deg - 4;

  for (i = 0; i <= t1.deg - 2; i++) {
    t.branch[i].x = t1.branch[i].x;
    t.branch[i].y = t1.branch[i].y;
    t.branch[i].n = t1.branch[i].n + offset1;
  }
  for (i = t1.deg - 1; i <= d - 1; i++) {
    t.branch[i].x = t2.branch[i - t1.deg + 2].x;
    t.branch[i].y = t2.branch[i - t1.deg + 2].y;
    t.branch[i].n = t2.branch[i - t1.deg + 2].n + offset2;
  }
  for (i = d; i <= d + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (i = d + t1.deg - 2; i <= 2 * d - 3; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }

  prev = t2.branch[0].n + offset2;
  curr = t1.branch[t1.deg - 1].n + offset1;
  next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

Tree hmergetree(Tree t1, Tree t2, int s[]) {
  int i, prev, curr, next, extra, offset1, offset2;
  int p, n1, n2;
  int nn1 = 0;
  int nn2 = 0;
  int ii = 0;

  DTYPE coord1, coord2;
  Tree t;

  t.deg = t1.deg + t2.deg - 1;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * t.deg - 2);
  offset1 = t2.deg - 1;
  offset2 = 2 * t1.deg - 3;

  p = t1.deg - 1;
  n1 = n2 = 0;
  for (i = 0; i < t.deg; i++) {
    if (s[i] < p) {
      t.branch[i].x = t1.branch[n1].x;
      t.branch[i].y = t1.branch[n1].y;
      t.branch[i].n = t1.branch[n1].n + offset1;
      n1++;
    } else if (s[i] > p) {
      t.branch[i].x = t2.branch[n2].x;
      t.branch[i].y = t2.branch[n2].y;
      t.branch[i].n = t2.branch[n2].n + offset2;
      n2++;
    } else {
      t.branch[i].x = t2.branch[n2].x;
      t.branch[i].y = t2.branch[n2].y;
      t.branch[i].n = t2.branch[n2].n + offset2;
      nn1 = n1;
      nn2 = n2;
      ii = i;
      n1++;
      n2++;
    }
  }
  for (i = t.deg; i <= t.deg + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (i = t.deg + t1.deg - 2; i <= 2 * t.deg - 4; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }
  extra = 2 * t.deg - 3;
  coord1 = t1.branch[t1.branch[nn1].n].y;
  coord2 = t2.branch[t2.branch[nn2].n].y;
  if (t2.branch[nn2].y > std::max(coord1, coord2)) {
    t.branch[extra].y = std::max(coord1, coord2);
    t.length -= t2.branch[nn2].y - t.branch[extra].y;
  } else if (t2.branch[nn2].y < std::min(coord1, coord2)) {
    t.branch[extra].y = std::min(coord1, coord2);
    t.length -= t.branch[extra].y - t2.branch[nn2].y;
  } else
    t.branch[extra].y = t2.branch[nn2].y;
  t.branch[extra].x = t2.branch[nn2].x;
  t.branch[extra].n = t.branch[ii].n;
  t.branch[ii].n = extra;

  prev = extra;
  curr = t1.branch[nn1].n + offset1;
  next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

Tree vmergetree(Tree t1, Tree t2) {
  int i, prev, curr, next, extra, offset1, offset2;
  DTYPE coord1, coord2;
  Tree t;

  t.deg = t1.deg + t2.deg - 1;
  t.length = t1.length + t2.length;
  t.branch.resize(2 * t.deg - 2);
  offset1 = t2.deg - 1;
  offset2 = 2 * t1.deg - 3;

  for (i = 0; i <= t1.deg - 2; i++) {
    t.branch[i].x = t1.branch[i].x;
    t.branch[i].y = t1.branch[i].y;
    t.branch[i].n = t1.branch[i].n + offset1;
  }
  for (i = t1.deg - 1; i <= t.deg - 1; i++) {
    t.branch[i].x = t2.branch[i - t1.deg + 1].x;
    t.branch[i].y = t2.branch[i - t1.deg + 1].y;
    t.branch[i].n = t2.branch[i - t1.deg + 1].n + offset2;
  }
  for (i = t.deg; i <= t.deg + t1.deg - 3; i++) {
    t.branch[i].x = t1.branch[i - offset1].x;
    t.branch[i].y = t1.branch[i - offset1].y;
    t.branch[i].n = t1.branch[i - offset1].n + offset1;
  }
  for (i = t.deg + t1.deg - 2; i <= 2 * t.deg - 4; i++) {
    t.branch[i].x = t2.branch[i - offset2].x;
    t.branch[i].y = t2.branch[i - offset2].y;
    t.branch[i].n = t2.branch[i - offset2].n + offset2;
  }
  extra = 2 * t.deg - 3;
  coord1 = t1.branch[t1.branch[t1.deg - 1].n].x;
  coord2 = t2.branch[t2.branch[0].n].x;
  if (t2.branch[0].x > std::max(coord1, coord2)) {
    t.branch[extra].x = std::max(coord1, coord2);
    t.length -= t2.branch[0].x - t.branch[extra].x;
  } else if (t2.branch[0].x < std::min(coord1, coord2)) {
    t.branch[extra].x = std::min(coord1, coord2);
    t.length -= t.branch[extra].x - t2.branch[0].x;
  } else
    t.branch[extra].x = t2.branch[0].x;
  t.branch[extra].y = t2.branch[0].y;
  t.branch[extra].n = t.branch[t1.deg - 1].n;
  t.branch[t1.deg - 1].n = extra;

  prev = extra;
  curr = t1.branch[t1.deg - 1].n + offset1;
  next = t.branch[curr].n;
  while (curr != next) {
    t.branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = t.branch[curr].n;
  }
  t.branch[curr].n = prev;

  return t;
}

void local_refinement(int deg, Tree *tp, int p) {
  int d, dd, i, ii, j, prev, curr, next, root;
  int *SteinerPin, *index, *ss, degree;
  DTYPE *x, *xs, *ys;
  Tree tt;
        
  degree = deg + 1;
  SteinerPin = (int *)malloc(sizeof(int) * (2 * degree));
  index = (int *)malloc(sizeof(int) * (2 * degree));
  x = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  xs = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  ys = (DTYPE *)malloc(sizeof(DTYPE) * (degree));
  ss = (DTYPE *)malloc(sizeof(DTYPE) * (degree));

  d = tp->deg;
  root = tp->branch[p].n;

  // Reverse edges to point to root
  prev = root;
  curr = tp->branch[prev].n;
  next = tp->branch[curr].n;
  while (curr != next) {
    tp->branch[curr].n = prev;
    prev = curr;
    curr = next;
    next = tp->branch[curr].n;
  }
  tp->branch[curr].n = prev;
  tp->branch[root].n = root;

  // Find Steiner nodes that are at pins
  for (i = d; i <= 2 * d - 3; i++)
    SteinerPin[i] = -1;
  for (i = 0; i < d; i++) {
    next = tp->branch[i].n;
    if (tp->branch[i].x == tp->branch[next].x &&
        tp->branch[i].y == tp->branch[next].y)
      SteinerPin[next] = i;  // Steiner 'next' at Pin 'i'
  }
  SteinerPin[root] = p;

  // Find pins that are directly connected to root
  dd = 0;
  for (i = 0; i < d; i++) {
    curr = tp->branch[i].n;
    if (SteinerPin[curr] == i)
      curr = tp->branch[curr].n;
    while (SteinerPin[curr] < 0)
      curr = tp->branch[curr].n;
    if (curr == root) {
      x[dd] = tp->branch[i].x;
      if (SteinerPin[tp->branch[i].n] == i && tp->branch[i].n != root)
        index[dd++] = tp->branch[i].n;  // Steiner node
      else index[dd++] = i;  // Pin
    }
  }

  if (4 <= dd && dd <= FLUTE_D) {
    // Find Steiner nodes that are directly connected to root
    ii = dd;
    for (i = 0; i < dd; i++) {
      curr = tp->branch[index[i]].n;
      while (SteinerPin[curr] < 0) {
        index[ii++] = curr;
        SteinerPin[curr] = INT_MAX;
        curr = tp->branch[curr].n;
      }
    }
    index[ii] = root;

    for (ii = 0; ii < dd; ii++) {
      ss[ii] = 0;
      for (j = 0; j < ii; j++)
        if (x[j] < x[ii])
          ss[ii]++;
      for (j = ii + 1; j < dd; j++)
        if (x[j] <= x[ii])
          ss[ii]++;
      xs[ss[ii]] = x[ii];
      ys[ii] = tp->branch[index[ii]].y;
    }

    tt = flutes_LD(dd, xs, ys, ss);

    // Find new wirelength
    tp->length += tt.length;
    for (ii = 0; ii < 2 * dd - 3; ii++) {
      i = index[ii];
      j = tp->branch[i].n;
      tp->length -= ADIFF(tp->branch[i].x, tp->branch[j].x)
        + ADIFF(tp->branch[i].y, tp->branch[j].y);
    }

    // Copy tt into t
    for (ii = 0; ii < dd; ii++) {
      tp->branch[index[ii]].n = index[tt.branch[ii].n];
    }
    for (; ii <= 2 * dd - 3; ii++) {
      tp->branch[index[ii]].x = tt.branch[ii].x;
      tp->branch[index[ii]].y = tt.branch[ii].y;
      tp->branch[index[ii]].n = index[tt.branch[ii].n];
    }
  }

  free(SteinerPin);
  free(index);
  free(x);
  free(xs);
  free(ys);
  free(ss);
        
  return;
}

DTYPE wirelength(Tree t) {
  int i, j;
  DTYPE l = 0;

  for (i = 0; i < 2 * t.deg - 2; i++) {
    j = t.branch[i].n;
    l += ADIFF(t.branch[i].x, t.branch[j].x) + ADIFF(t.branch[i].y, t.branch[j].y);
  }

  return l;
}

// Output in a format that can be plotted by gnuplot
void plottree(Tree t) {
  int i;

  for (i = 0; i < 2 * t.deg - 2; i++) {
    printf("%d %d\n", t.branch[i].x, t.branch[i].y);
    printf("%d %d\n\n", t.branch[t.branch[i].n].x,
           t.branch[t.branch[i].n].y);
  }
}

// Write svg file viewable in a web browser.
void write_svg(Tree t,
               const char *filename) {
  int x_min = INT_MAX;
  int y_min = INT_MAX;
  int x_max = INT_MIN;
  int y_max = INT_MIN;
  for (int i = 0; i < 2 * t.deg - 2; i++) {
    x_min = std::min(x_min, t.branch[i].x);
    y_min = std::min(y_min, t.branch[i].y);
    x_max = std::max(x_max, t.branch[i].x);
    y_max = std::max(y_max, t.branch[i].y);
  }

  int dx = x_max - x_min;
  int dy = y_max - y_min;
  const int sz = std::max(std::max(dx, dy) / 400, 1);
  const int hsz = sz / 2;

  FILE* stream = fopen(filename, "w");
  if (stream) {
    fprintf(stream, "<svg xmlns=\"http://www.w3.org/2000/svg\" "
            "viewBox=\"%d %d %d %d\">\n",
            x_min, y_min,
            dx, dy);

    for (int i = 0; i < 2 * t.deg - 2; i++) {
      fprintf(stream, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" "
              "style=\"stroke: black; stroke-width: %d\"/>\n",
              t.branch[i].x, t.branch[i].y,
              t.branch[t.branch[i].n].x, t.branch[t.branch[i].n].y,
              hsz/2);
    }
    fprintf(stream, "</svg>\n");
    fclose(stream);
  }
}

} // namespace flt

} // namespace stt
