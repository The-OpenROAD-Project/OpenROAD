/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

/*  Modified by Igor Markov, VLSI CAD ABKGROUP UCLA/CS 1997 */

/*
 * Ken Clarkson wrote this.  Copyright (c) 1996 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/*
 * the results should be "robust", and not return a wildly wrong hull,
 *      despite using floating point
 * works in O(n log n); I think a bit faster than Graham scan;
 * somewhat like Procedure 8.2 in Edelsbrunner's "Algorithms in Combinatorial
 *      Geometry".
 */

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <stdlib.h>
#include <stdio.h>
#include "ABKCommon/abkcommon.h"
#include "placement.h"
#include "chull.h"

unsigned _CH_N = 0;
double *points_orig = NULL, **_CH_P = NULL;

int ch2d(double **P, int n);

CHull::CHull(const Placement &pl) {
        _CH_N = pl.getSize();
        points_orig = new double[2 * _CH_N];
        _CH_P = new double *[_CH_N + 1];  // allocating pointer
        unsigned i;
        for (i = 0; i < _CH_N; ++i) {
                points_orig[2 * i] = pl[i].x;
                points_orig[2 * i + 1] = pl[i].y;
                _CH_P[i] = &points_orig[2 * i];
        }
        nPts = ch2d(_CH_P, _CH_N);
        points = new Point[nPts];
        for (i = 0; i < nPts; ++i) {
                points[i].x = *(_CH_P[i]);
                points[i].y = *(_CH_P[i] + 1);
        }
        delete[] _CH_P;
        delete[] points_orig;
}

int ccw(double **P, int i, int j, int k) {
        double a = P[i][0] - P[j][0], b = P[i][1] - P[j][1], c = P[k][0] - P[j][0], d = P[k][1] - P[j][1];
        return a * d - b * c <= 0;
        /* true if points_orig i, j, k counterclockwise */
}

#define CMPM(c, A, B)                                \
        v = (*(double **)A)[c] - (*(double **)B)[c]; \
        if (v > 0) return 1;                         \
        if (v < 0) return -1;

extern "C" {
int cmpl(const void *a, const void *b) {
        double v;
        CMPM(0, a, b);
        CMPM(1, b, a);
        return 0;
}

inline int cmph(const void *a, const void *b) { return cmpl(b, a); }

typedef int (*cmp_func_ptr)(const void *, const void *);
}

int make_chain(double **V, int n, cmp_func_ptr cmp) {
        int i, j, s = 1;
        double *t;

        qsort(V, n, sizeof(double *), cmp);
        for (i = 2; i < n; i++) {
                for (j = s; j >= 1 && ccw(V, i, j, j - 1); j--) {
                }
                s = j + 1;
                t = V[s];
                V[s] = V[i];
                V[i] = t;
        }
        return s;
}

int ch2d(double **P, int n) {
        int u = make_chain(P, n, cmpl); /* make lower hull */
        if (!n) return 0;
        P[n] = P[0];
        return u + make_chain(P + u, n - u + 1, cmph); /* make upper hull */
}
