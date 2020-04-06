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

// Created 990322 by Stefanus Mantik
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <Stats/multiRegre.h>

using std::cout;
using std::endl;
using uofm::vector;

double MultipleRegression::determinant(const vector<vector<double> >& m) {
        vector<vector<double> > e(m);
        double det = 1;
        unsigned i;
        for (i = 0; i < e.size() - 1; i++) {
                if (e[i][i] == 0) {
                        unsigned j, jFound = UINT_MAX;
                        for (j = i + 1; j < e.size(); j++)
                                if (e[j][i] != 0) {
                                        jFound = j;
                                        break;
                                }
                        if (jFound != UINT_MAX)
                                for (j = 0; j < e[i].size(); j++) e[i][j] += e[jFound][j];
                        else
                                det = 0;
                }
                if (det == 0) break;
                for (unsigned j = i + 1; j < e.size(); j++) {
                        double xm = e[j][i] / e[i][i];
                        for (unsigned k = i; k < e[j].size(); k++) e[j][k] = e[j][k] - xm * e[i][k];
                }
        }
        if (det == 0) return det;
        for (i = 0; i < e.size(); i++) det *= e[i][i];
        return det;
}

void MultipleRegression::calculateRegression(const vector<vector<double> >& xIn) {
        double f1, f2, phi, sy, deter, dvest;
        /// unsigned ids=1; double tl=0.0001;
        unsigned istp = 1, wdta = xIn[0].size(), i, j, k, iy, ir = 2;
        f1 = f2 = 3.29;
        vector<vector<double> > r(xIn.size(), vector<double>(xIn.size(), 0));
        vector<double> wsum(r.size(), 0), wmean(r.size(), 0), sigma(r.size(), 0);
        for (i = 0; i < r.size(); i++)
                for (j = 0; j < r[i].size(); j++) wsum[i] += xIn[i][j];
        for (i = 0; i < r.size(); i++)
                for (j = 0; j < r.size(); j++)
                        for (k = 0; k < r[i].size(); k++) r[i][j] += xIn[i][k] * xIn[j][k];
        for (i = 0; i < r.size(); i++) wmean[i] = wsum[i] / wdta;
        for (i = 0; i < r.size(); i++)
                for (j = 0; j < r.size(); j++) r[i][j] -= wsum[i] * wsum[j] / wdta;
        for (i = 0; i < r.size(); i++) sigma[i] = sqrt(r[i][i]);
        for (i = 0; i < r.size(); i++)
                for (j = 0; j < r.size(); j++) r[i][j] /= sigma[i] * sigma[j];
        for (i = 1; i < r.size(); i++)
                for (j = 0; j < i; j++) r[i][j] = r[j][i];
        double det = determinant(r);
        abkfatal(det != 0, "Variables are not independent");
        phi = wdta - 1;
        vector<double> sb(r.size(), 0), b(r.size(), 0);
        while (true) {

                if (r[r.size() - 1][r.size() - 1] < 0) r[r.size() - 1][r.size() - 1] = 0;
                sy = sqrt(sigma[r.size() - 1] * r[r.size() - 1][r.size() - 1] / phi);
                if (istp > 1) {
                        for (i = 0; i < istp - 1; i++) b[i] = r[i][r.size() - 1] * sigma[r.size() - 1] / sigma[i];
                        abkfatal(r[i][i] != 0, "Linear combination var found");
                        sb[i] = sy * sqrt(r[i][i]) / sigma[i];
                }
                if (ir <= 1) {
                        b[r.size() - 1] = wmean[r.size() - 1];
                        for (i = 0; i < r.size() - 1; i++) b[r.size() - 1] -= b[i] * wmean[i];
                        deter = 1 - r[r.size() - 1][r.size() - 1];
                        if (deter < 0.000001) deter = 0;
                        dvest = sy;
                        if (dvest < 0.000001) dvest = 0;
                        _c = b[r.size() - 1];
                        for (i = 0; i < r.size() - 1; i++) _k[i] = b[i];
                        cout << "determination coeff = " << deter << endl;
                        cout << "STDEV of estimative = " << dvest << endl;
                }
                istp++;
                if (istp > r.size()) break;
                if (istp == r.size()) {
                        iy = ir;
                        ir = 1;
                }
                k = istp - 1;
                phi--;
                if (ir > 2) break;
                for (i = 0; i < r.size(); i++) {
                        if (i == k) continue;
                        for (j = 0; j < r.size(); j++) {
                                if (j == k) continue;
                                r[i][j] -= r[i][k] * r[k][j] / r[k][k];
                        }
                }
                for (i = 0; i < r.size(); i++) {
                        if (i == k) continue;
                        r[i][k] = -r[i][k] / r[k][k];
                        r[k][i] = r[k][i] / r[k][k];
                }
                r[k][k] = 1 / r[k][k];
        }
}

MultipleRegression::MultipleRegression(const vector<vector<double> >& x, const vector<double>& y) : _k(x.size(), 0.0) {
        abkfatal(x[0].size() == y.size(),
                 "Attempt to compute linear regression for vectors of difft "
                 "sizes \n");

        /*
          unsigned i, size=x.size();
          double   sx = 0, sy = 0;

          for (i=0;i!=size; i++)
          {
            sx += x[i];
            sy += y[i];
          }

          double sxoss = sx/size;
          double st2 = 0;
          for(i=0;i!=size;i++)
          {
            double  t = x[i] - sxoss;
            st2   += square(t);
            _k    += t*y[i];
          }

          _k /= st2;
          _c = (sy-sx*_k)/size;
        */
        vector<vector<double> > r(x);
        r.push_back(y);
        calculateRegression(r);
}
