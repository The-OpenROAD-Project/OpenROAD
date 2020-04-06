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

// Created 970926 by Igor Markov
// CHANGES
// 970108 mro  restored part of correction for ties in RankCorrelation
//             and took scope resolution operator out of definition
//             of compute_rankcorr() (ask Chuck if it gives him trouble)
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include <Stats/rancor.h>
#include <algorithm>
#include <cmath>

using uofm::vector;

class CompareByX {
       public:
        bool operator()(const RankCorrelation::RankCorrElem& i, const RankCorrelation::RankCorrElem& j) { return i.x < j.x; }
};

class CompareByY {
       public:
        bool operator()(const RankCorrelation::RankCorrElem& i, const RankCorrelation::RankCorrElem& j) { return i.y < j.y; }
};

double crankx_(vector<RankCorrelation::RankCorrElem>& data);
double cranky_(vector<RankCorrelation::RankCorrElem>& data);

RankCorrelation::RankCorrelation(const vector<double>& x1, const vector<double>& x2) {
        abkfatal(x1.size() == x2.size(),
                 " Attempt to compute rank correlation of vectors of different "
                 "sizes \n");
        unsigned k, size = x1.size();
        vector<RankCorrelation::RankCorrElem> tmpVec(size);
        for (k = 0; k < size; k++) tmpVec[k] = RankCorrelation::RankCorrElem(x1[k], x2[k]);
        compute_rankcorr(tmpVec);
}

RankCorrelation::RankCorrelation(const vector<unsigned>& x1, const vector<unsigned>& x2) {
        abkfatal(x1.size() == x2.size(),
                 " Attempt to compute rank correlation of vectors of different "
                 "sizes \n");
        unsigned k, size = x1.size();
        vector<RankCorrelation::RankCorrElem> tmpVec(size);
        for (k = 0; k < size; k++) tmpVec[k] = RankCorrelation::RankCorrElem(x1[k], x2[k]);
        compute_rankcorr(tmpVec);
}

void RankCorrelation::compute_rankcorr(vector<RankCorrElem>& data) {
        unsigned i, size = data.size();
        // double syy = 0.0, sxy = 0.0, sxx = 0.0
        double ay = 0.0, ax = 0.0;
        double dsize = size;

        for (i = 0; i != size; i++) {
                ax += data[i].x;
                ay += data[i].y;
        }
        ax = ax / size;
        ay = ay / size;

        std::sort(data.begin(), data.end(), CompareByX());
        abkfatal(data[0].x < data[size - 1].x,
                 "Attempt to compute rank correlation "
                 "with all x values tied\n");
        double sf = crankx_(data);

        std::sort(data.begin(), data.end(), CompareByY());
        abkfatal(data[0].y < data[size - 1].y,
                 "Attempt to compute rank correlation "
                 "with all y values tied\n");
        double sg = cranky_(data);

        double d = 0.0;
        for (i = 0; i < size; i++) d += (data[i].x - data[i].y) * (data[i].x - data[i].y);

        double en3n = (square(dsize) - 1.0) * dsize;
        double fac = (1.0 - sf / en3n) * (1.0 - sg / en3n);
        _rankcorr = (1.0 - (6.0 / en3n) * (d + (sf + sg) / 12.0)) / sqrt(fac);
}

double crankx_(vector<RankCorrelation::RankCorrElem>& data) {
        unsigned j, ji, jt, size = data.size();
        double t, rank, res = 0.0;

        j = 0;
        while (j < size - 1) {
                if (data[j + 1].x != data[j].x) {
                        data[j].x = j;
                        ++j;
                } else {
                        for (jt = j + 1; jt <= size - 1 && data[jt].x == data[j].x; jt++)
                                ;
                        rank = 0.5 * (j + jt - 1);
                        for (ji = j; ji <= (jt - 1); ji++) data[ji].x = rank;
                        t = jt - j;
                        res += t * t * t - t;
                        j = jt;
                }
        }
        if (j == size - 1) data[size - 1].x = size - 1;
        return res;
}

double cranky_(vector<RankCorrelation::RankCorrElem>& data) {
        unsigned j, ji, jt, size = data.size();
        double t, rank, res = 0.0;
        j = 0;
        while (j < size - 1) {
                if (data[j + 1].y != data[j].y) {
                        data[j].y = j;
                        ++j;
                } else {
                        for (jt = j + 1; jt <= size - 1 && data[jt].y == data[j].y; jt++)
                                ;
                        rank = 0.5 * (j + jt - 1);
                        for (ji = j; ji <= (jt - 1); ji++) data[ji].y = rank;
                        t = jt - j;
                        res += t * t * t - t;
                        j = jt;
                }
        }
        if (j == size - 1) data[size - 1].y = size - 1;
        return res;
}
