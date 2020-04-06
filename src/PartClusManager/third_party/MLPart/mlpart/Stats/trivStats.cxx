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

// Created: September 22, 1997 by Igor Markov
// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "trivStats.h"
#include <ABKCommon/abkcommon.h>
#include <iostream>
#include <cmath>
#include <ABKCommon/uofm_alloc.h>

using std::ostream;
using std::endl;
using uofm::vector;

Correlation::Correlation(const vector<double>& x1, const vector<double>& x2) {
        abkfatal(x1.size() == x2.size(),
                 " Attempted to compute correlation of vectors of different "
                 "sizes \n");
        unsigned k, size = x1.size();
        double x1sum = 0.0, x2sum = 0.0;
        for (k = 0; k != size; k++) {
                x1sum += x1[k];
                x2sum += x2[k];
        }
        double x1ave = x1sum / size, x2ave = x2sum / size;

        double dx1dx1 = 0.0, dx2dx2 = 0.0, dx1dx2 = 0.0;
        for (k = 0; k != size; k++) {
                double dx1 = x1[k] - x1ave, dx2 = x2[k] - x2ave;
                dx1dx1 += square(dx1);
                dx2dx2 += square(dx2);
                dx1dx2 += dx1 * dx2;
        }

        _corr = dx1dx2 / sqrt(dx1dx1 * dx2dx2);
}

Correlation::Correlation(const vector<unsigned>& x1, const vector<unsigned>& x2) {
        abkfatal(x1.size() == x2.size(),
                 " Attempted to compute correlation of vectors of different "
                 "sizes \n");
        unsigned k, size = x1.size();
        double x1sum = 0.0, x2sum = 0.0;
        for (k = 0; k != size; k++) {
                x1sum += x1[k];
                x2sum += x2[k];
        }
        double x1ave = x1sum / size, x2ave = x2sum / size;

        double dx1dx1 = 0.0, dx2dx2 = 0.0, dx1dx2 = 0.0;
        for (k = 0; k != size; k++) {
                double dx1 = x1[k] - x1ave, dx2 = x2[k] - x2ave;
                dx1dx1 += square(dx1);
                dx2dx2 += square(dx2);
                dx1dx2 += dx1 * dx2;
        }

        _corr = dx1dx2 / sqrt(dx1dx1 * dx2dx2);
}

TrivialStats::TrivialStats(const vector<float>& data) : _tot(0.0), _num(data.size()) {
        abkwarn(_num != 0, " Can't create TrivialStats from a zero-sized vector ");
        _max = _min = data[0];
        if (_num == 0) {
                _avg = _tot = _max = _min = DBL_MAX;
                return;
        }
        for (unsigned k = 0; k != _num; k++) {
                double cur(data[k]);
                _tot += cur;
                _max = std::max(_max, cur);
                _min = std::min(_min, cur);
        }
        _avg = (_tot + 0.0) / _num;  // 0.0 needed in case we reuse this code for int types
}

TrivialStats::TrivialStats(const vector<double>& data) : _tot(0.0), _num(data.size()) {
        abkwarn(_num != 0, " Can't create TrivialStats from a zero-sized vector ");
        _max = _min = data[0];
        if (_num == 0) {
                _avg = _tot = _max = _min = DBL_MAX;
                return;
        }
        for (unsigned k = 0; k != _num; k++) {
                double cur(data[k]);
                _tot += cur;
                _max = std::max(_max, cur);
                _min = std::min(_min, cur);
        }
        _avg = (_tot + 0.0) / _num;  // 0.0 needed in case we reuse this code for int types
}

TrivialStats::TrivialStats(const vector<unsigned>& data) : _tot(0.0), _num(data.size()) {
        abkfatal(_num != 0, " Can't create TrivialStats from a zero-sized vector\n");
        _max = _min = data[0];
        for (unsigned k = 0; k != _num; k++) {
                double cur(data[k]);
                _tot += cur;
                _max = std::max(_max, cur);
                _min = std::min(_min, cur);
        }
        _avg = (_tot + 0.0) / _num;  // 0.0 needed in case we reuse this code for int types
}

ostream& operator<<(ostream& out, const TrivialStats& stats) {
        out << " Max: " << stats.getMax() << "  Min: " << stats.getMin() << "  Avg: " << stats.getAvg() << endl;
        return out;
}

TrivialStatsWithStdDev::TrivialStatsWithStdDev(const vector<float>& data) : TrivialStats(data) {
        float totDev = 0.0;
        for (unsigned k = 0; k != _num; k++) totDev += square(_avg - data[k]);
        _stdDev = sqrt(totDev / _num);
}

TrivialStatsWithStdDev::TrivialStatsWithStdDev(const vector<double>& data) : TrivialStats(data) {
        double totDev = 0.0;
        for (unsigned k = 0; k != _num; k++) totDev += square(_avg - data[k]);
        _stdDev = sqrt(totDev / _num);
}

TrivialStatsWithStdDev::TrivialStatsWithStdDev(const vector<unsigned>& data) : TrivialStats(data) {
        double totDev = 0.0;
        for (unsigned k = 0; k != _num; k++) totDev += square(_avg - data[k]);
        _stdDev = sqrt(totDev / _num);
}

ostream& operator<<(ostream& out, const TrivialStatsWithStdDev& stats) {
        out << " Max: " << stats.getMax() << "  Min: " << stats.getMin() << "  Avg: " << stats.getAvg() << "  StdDev: " << stats.getStdDev() << endl;
        return out;
}
