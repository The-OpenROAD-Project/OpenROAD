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

#ifndef _ABKRAND_TEMPL_CXX_INCLUDED
#define _ABKRAND_TEMPL_CXX_INCLUDED

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "abkrand.h"
#include <cfloat>
#include <cstdio>
#include <ABKCommon/uofm_alloc.h>

template <class RK>
inline unsigned RandomNumberGeneratorT<RK>::_getUnsignedRand() {
        return _lowerB + RK::_getRawUnsigned() % _delta;
}

template <class RK>
inline double RandomNumberGeneratorT<RK>::_getDoubleRand() {
        return RK::_getRawDouble() * _dDelta + _dLowerB;
}

template <class RK>
RandomNormalT<RK>::operator double() {
        if (_cacheFull) {
                _cacheFull = false;
                return _cachedValue;
        } else {
                while (true) {
                        double V1, V2, S;
                        V1 = 2 * RK::_getRawDouble() - 1;
                        V2 = 2 * RK::_getRawDouble() - 1;
                        S = V1 * V1 + V2 * V2;
                        if (S < 1.0) {
                                double multiplier = sqrt(-2 * log(S) / S);
                                _cachedValue = _sigma * V1 * multiplier + _mu;
                                _cacheFull = true;
                                return _sigma * V2 * multiplier + _mu;
                        }
                }
        }
}

template <class RK>
std::pair<double, double> RandomNormCorrPairsT<RK>::getPair() {
        double x1 = _norm;
        double x2 = _norm;
        double z1 = _a1 * x1 + _b1 * x2 + _mu1;
        double z2 = _a2 * x1 - _b2 * x2 + _mu2;
        return std::pair<double, double>(z1, z2);
}
template <class RK>
bool RandomNormCorrTuplesT<RK>::_findBasis(const uofm::vector<uofm::vector<double> > &_rho_ij, uofm::vector<uofm::vector<double> > &_v_ij) {
        _v_ij.clear();
        const unsigned n = _rho_ij.size() + 1;
        uofm::vector<double> zeros(n, 0);
        _v_ij.insert(_v_ij.end(), n, zeros);
        _v_ij[0][0] = 1;
        unsigned i, j, k;
        for (i = 1; i < n; i++) {
                double sumsq = 0;
                for (j = 0; j < i; j++) {
                        // dot product of v_i with v_j = rho_ij, that
                        // is _rho_ij[j][i-j-i]
                        double val = _rho_ij[j][i - j - 1];
                        for (k = 0; k < j; k++) {
                                val -= _v_ij[i][k] * _v_ij[j][k];
                        }
                        val /= _v_ij[j][j];

                        _v_ij[i][j] = val;
                        sumsq += val * val;
                }

                if (sumsq >= 1) return false;  // If sumsq>1, can't get new
                // length-1 uofm::vector with correct
                // correlations; if sumsq==1, could,
                // but vecs would be lin. dep.

                _v_ij[i][i] = sqrt(1 - sumsq);  // make uofm::vector have length 1
        }

        return true;
}

template <class RK>
RandomNormCorrTuplesT<RK>::~RandomNormCorrTuplesT() {
        unsigned j;
        for (j = 0; j < _norm_j.size(); j++) delete _norm_j[j];
}

template <class RK>
RandomNormCorrTuplesT<RK>::RandomNormCorrTuplesT(const uofm::vector<double> &means, const uofm::vector<double> &stdDevs,

                                                 // Note:  corrs[i][j] is the desired
                                                 // correlation between X_i and
                                                 // X_{i+j+1} (i.e. only upper-triangular
                                                 // elements are included
                                                 const uofm::vector<uofm::vector<double> > &corrs, const char *locIdent, unsigned counterOverride, Verbosity verb)
    : _n(means.size()), _norm_j(_n, NULL), _y_j(_n, DBL_MAX), _mu_i(means), _sigma_i(stdDevs), _bad(false) {
        char txt[127];
        char ourIdent[] = "RandomNormCorrTuplesT<RK>";
        abkfatal(stdDevs.size() == _n, "Mismatch between size of means and stdDevs");
        abkfatal(corrs.size() == _n - 1, "Mismatch between size of means and corrs");
        unsigned j;
        for (j = 0; j < _n; j++) {
                sprintf(txt, "%d", j);
                char *newIdent = new char[strlen(txt) + strlen(ourIdent) + strlen(locIdent) + 1];
                strcpy(newIdent, locIdent);
                strcat(newIdent, ourIdent);
                strcat(newIdent, txt);
                _norm_j[j] = new RandomNormalT<RK>(0, 1, newIdent, counterOverride, verb);
                delete[] newIdent;
        }

        bool success = _findBasis(corrs, _v_ij);
        _bad = !success;
}
template <class RK>
void RandomNormCorrTuplesT<RK>::getTuple(uofm::vector<double> &tuple) {
        abkfatal(tuple.size() == _n, "Bad tuple size");
        abkfatal(!_bad, "Attempt to get tuple from bad RNG");
        unsigned i, j;
        for (j = 0; j < _n; j++) _y_j[j] = _norm_j[j]->operator double();

        for (i = 0; i < _n; i++) {
                double val = 0;
                const uofm::vector<double> &v_i = _v_ij[i];
                for (j = 0; j <= i; j++) val += v_i[j] * _y_j[j];
                val *= _sigma_i[i];
                val += _mu_i[i];
                tuple[i] = val;
        }
}

#endif  // !defined (_ABKRAND_TEMPL_CXX_INCLUDED)
