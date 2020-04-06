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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <Stats/expMins.h>
#include <numeric>

using std::ostream;
using std::endl;
using std::min;
using uofm::vector;

ExpectedMins::ExpectedMins(const vector<unsigned>& data, unsigned oversampl) : _data(data.size()), _exps(data.size() / 5 + 1, 0.0) {
        for (unsigned k = 0; k != data.size(); k++) _data[k] = data[k];
        abkfatal(data.size() >= 10, " Smpl too small ");
        abkfatal(oversampl != 0, " Can not oversample 0 times");
        if (oversampl == UINT_MAX)
                _oversampl = min(_data.size(), size_t(20));
        else
                _oversampl = min(unsigned(_data.size()), oversampl);
        _ctruct();
}

ExpectedMins::ExpectedMins(const vector<double>& data, unsigned oversampl) : _data(data), _exps(data.size() / 5 + 1, 0.0) {
        abkfatal(data.size() >= 10, " Smpl too small ");
        abkfatal(oversampl != 0, " Can not oversampl 0 times");
        if (oversampl == UINT_MAX)
                _oversampl = min(_data.size(), size_t(20));
        else
                _oversampl = min(unsigned(_data.size()), oversampl);
        _ctruct();
}

void ExpectedMins::_ctruct() {
        _exps[1] = accumulate(_data.begin(), _data.end(), 0.0);
        _exps[1] /= _data.size();

        RandomRawUnsigned ru;

        for (unsigned minOf = 2; minOf != _exps.size(); minOf++) {
                unsigned runs = (_oversampl * (_data.size() / minOf));
                for (unsigned k = runs; k != 0; k--) {
                        double tmp = _data[ru % _data.size()];
                        for (unsigned i = 1; i != minOf; i++) {
                                double d = _data[ru % _data.size()];
                                if (d < tmp) tmp = d;
                        }
                        _exps[minOf] += tmp;
                }
                _exps[minOf] /= runs;
        }
}

ostream& operator<<(ostream& os, const ExpectedMins& em) {
        os << endl << " Expected minima for subsamples of sizes 1 through " << em.getSize() << endl;

        for (unsigned k = 1; k != em.getSize(); k++) {
                if (k % 5 == 0) os << endl;
                char buf[100];
                sprintf(buf, " % 4d: %-10.3g  ", k, em[k]);
                os << buf;
        }
        os << endl;

        return os;
}
