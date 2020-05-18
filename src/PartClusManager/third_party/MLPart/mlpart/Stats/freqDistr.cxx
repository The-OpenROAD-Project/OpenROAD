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

// Created: Sep 23, 1997 by Igor Markov

// CHANGES
// 970619 ilm
// 1.5.1 980126 ilm -- added an optional argument to the FreqDistribution
//                    ctors setting the number of subranges for ASCII output.

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "Combi/combi.h"
#include "Combi/mapping.h"
#include "freqDistr.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <ABKCommon/uofm_alloc.h>

using uofm::stringstream;
using std::ofstream;
using std::ostream;
using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

void CumulativeFrequencyDistribution::computePercBelow() {
        unsigned k;
        double totInv;
        switch (_type) {
                case Magnitudes:
                        totInv = 1.0 / (_data[getSize() - 1] - _data[0]);
                        for (k = 0; k < _data.size(); k++) _percBelow[k] = (_data[k] - _data[0]) * totInv;
                        break;

                case Quantities:
                        totInv = 1.0 / _data.size();
                        for (k = 0; k < _data.size(); k++) _percBelow[k] = (k + 1) * totInv;
                        break;

                default:
                        abkfatal(0, " CumulativeFrequencyDistribution: unkown type");
        }
}

// the argument can be any (not necessarily sorted) vector
CumulativeFrequencyDistribution::CumulativeFrequencyDistribution(const vector<double>& data, unsigned nSubRanges, Type type) : numSubranges(nSubRanges), _type(type), _data(data.size()), _percBelow(data.size(), -1.0), _sortedIdx(data) {
        for (unsigned k = 0; k != data.size(); k++) _data[_sortedIdx[k]] = data[k];
        //_sortedIdx.getInverse(_sortedIdxInverse);
        computePercBelow();
}

CumulativeFrequencyDistribution::CumulativeFrequencyDistribution(const vector<unsigned>& data, unsigned nSubRanges, Type type) : numSubranges(nSubRanges), _type(type), _data(data.size()), _percBelow(data.size(), -1.0) {
        vector<double> tmpVec(_data.size());
        unsigned k;
        // for(k=0; k!=data.size(); k++) tmpVec[0]=data[k];
        std::copy(data.begin(), data.end(), tmpVec.begin());
        _sortedIdx = Permutation(tmpVec, 0.0);
        //_sortedIdx.getInverse(_sortedIdxInverse);
        for (k = 0; k != data.size(); k++) _data[_sortedIdx[k]] = data[k];

        computePercBelow();
}

void CumulativeFrequencyDistribution::saveXYPlot(const char* xyFileName) const {
        abkfatal(xyFileName != NULL, "CumulativeFrequencyDistribution: XY file name corrupt ");
        cout << " Opening " << xyFileName << " ... ";
        ofstream os(xyFileName);
        abkfatal(os, " Unable to open XY file for writing");
        cout << " Ok " << endl;

        double step = getSize() / 500.0;
        if (step < 1.0) step = 1.0;

        for (double curNum = 0.0; curNum != getSize(); curNum += step) {
                unsigned idx = static_cast<unsigned>(curNum);
                os << setw(12) << _data[idx] << " " << setw(12) << _percBelow[idx] << endl;
        }
}

unsigned CumulativeFrequencyDistribution::getNumSubranges() const {
        if (_data[getSize() - 1] == _data[0]) return 1;
        if (numSubranges == 0) return 1;
        size_t sz = _data.size();
        if (sz == 0) return 1;
        if (numSubranges < sz) return numSubranges;
        return unsigned(sz);
}

void CumulativeFrequencyDistribution::printEqualSubranges(ostream& out) const {
        unsigned subRanges = getNumSubranges();
        unsigned size = _data.size();
        double globalRangeSize = _data[size - 1] - _data[0];
        double subRangeSize = globalRangeSize / subRanges;

        out << " Distribution in ";
        if (subRanges != 1)
                out << subRanges << " equal subranges : ";
        else
                out << "the global range";
        double curMinVal = _data[0];
        vector<double>::const_iterator curMinIter = _data.begin();
        char buf1[1023], cloBrac;
        double curMaxVal;
        for (unsigned k = 0; k != subRanges; k++) {
                if (k % 3 == 0) out << endl;
                const char* spc = ((k + 1) % 3 == 0 ? "" : " ");

                if (k == subRanges - 1) {
                        cloBrac = ']';
                        curMaxVal = _data[size - 1];
                } else {
                        cloBrac = ')';
                        curMaxVal = curMinVal + (1 - 1e-10) * subRangeSize;
                }

                if (curMinVal == curMaxVal) cloBrac = ']';

                vector<double>::const_iterator curMaxIter = std::upper_bound(_data.begin(), _data.end(), curMaxVal);
                sprintf(buf1, "[%.3g,%.3g%c:", curMinVal, curMaxVal, cloBrac);
                stringstream buf2;
                buf2 << setw(20) << buf1 << " " << setw(5) << setiosflags(std::ios::left) << curMaxIter - curMinIter << spc;
                out << setw(26) << buf2.str() << setw(0);
                curMinVal = curMaxVal;
                curMinIter = curMaxIter;
        }
        out << endl;
}

void CumulativeFrequencyDistribution::printEquipotentSubranges(ostream& out) const {
        unsigned subRanges = getNumSubranges();
        // unsigned size=_data.size();
        double subRangeSize = 1.0 / subRanges;

        out << " Distribution in ";
        if (subRanges != 1)
                out << subRanges << " deciles (equipotent subranges) : ";
        else
                out << "the global range";

        double curMinPerc = 0.0;
        double curMinVal = _data[0];
        unsigned curMinNum = UINT_MAX;
        // vector<double>::const_iterator curMinIter=_data.begin();
        char buf1[1023], cloBrac;
        double curMaxPerc;
        for (unsigned k = 0; k != subRanges; k++) {
                if (k % 3 == 0) out << endl;
                const char* spc = ((k + 1) % 3 == 0 ? "" : " ");

                if (k == (subRanges - 1)) {
                        cloBrac = ']';
                        curMaxPerc = 1.0;
                } else {
                        cloBrac = ')';
                        curMaxPerc = curMinPerc + (1 - 1e-10) * subRangeSize;
                }

                unsigned curMaxNum = std::upper_bound(_percBelow.begin(), _percBelow.end(), curMaxPerc) - _percBelow.begin();
                curMaxNum = std::min(curMaxNum, unsigned(getSize() - 1));

                double curMaxVal = _data[curMaxNum];

                if (curMinVal == curMaxVal) cloBrac = ']';
                sprintf(buf1, "[%.3g,%.3g%c:", curMinVal, curMaxVal, cloBrac);
                stringstream buf2;
                buf2 << setw(20) << buf1 << " " << setw(5) << setiosflags(std::ios::left) << curMaxNum - curMinNum << spc;
                out << setw(26) << buf2.str() << setw(0);

                curMinPerc = curMaxPerc;
                curMinVal = curMaxVal;
                curMinNum = curMaxNum;
        }
        out << endl;
}

ostream& operator<<(ostream& out, const CumulativeFrequencyDistribution& stats) {
        stats.printEqualSubranges(out);
        stats.printEquipotentSubranges(out);
        return out;
}
