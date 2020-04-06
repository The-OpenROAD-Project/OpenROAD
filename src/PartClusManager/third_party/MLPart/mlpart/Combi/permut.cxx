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

#include "permut.h"
#include <utility>
#include <ABKCommon/uofm_alloc.h>

typedef std::pair<double, unsigned> NumNum;

using std::ostream;
using std::endl;
using std::sort;
using uofm::vector;

class CompareNumNum {
        double _epsilon;

       public:
        CompareNumNum(double epsilon = 0.0) : _epsilon(epsilon) {}
        bool operator()(const NumNum& nn1, const NumNum& nn2) {
                if (nn1.first + _epsilon < nn2.first) return true;
                if (nn1.first - _epsilon > nn2.first) return false;
                if (nn1.second < nn2.second) return true;
                return false;
        }
};

Permutation::Permutation(const vector<double>& unsorted, double epsilon) : Mapping(unsorted.size(), unsorted.size()) {
        std::vector<NumNum> data(unsorted.size());
        unsigned k;
        for (k = 0; k < unsorted.size(); k++) {
                data[k].first = unsorted[k];
                data[k].second = k;
        }
        sort(data.begin(), data.end(), CompareNumNum(epsilon));
        for (k = 0; k < unsorted.size(); k++) _meat[data[k].second] = k;
}

Permutation& Permutation::getInverse(Permutation& result) const {
        // see comments next to declaration
        // NB: no need to check for surjectivity
        result = Permutation(getSize());
        unsigned i;
        for (i = 0; i != _meat.size(); i++) result._meat[i] = (unsigned)-1;
        for (i = 0; i < getSize(); i++) {
                unsigned tmp = result[_meat[i]];
                abkassert(tmp == (unsigned)-1, "Can\'t invert permutation: not injective");
                result[_meat[i]] = i;
        };
        return result;
}

void reorderBitVector(uofm_bit_vector& bvec, const Permutation& pm) {
        abkassert(bvec.size() == pm.getSize(), "Size mismatch during reordering");
        uofm_bit_vector tmpBVec(bvec.size());
        for (unsigned k = 0; k < bvec.size(); k++) tmpBVec[pm[k]] = bvec[k];
        bvec = tmpBVec;
}

void reorderBitVectorBack(uofm_bit_vector& bvec, const Permutation& pm) {
        abkassert(bvec.size() == pm.getSize(), "Size mismatch during reordering");
        uofm_bit_vector tmpBVec(bvec.size());
        for (unsigned k = 0; k < bvec.size(); k++) tmpBVec[k] = bvec[pm[k]];
        bvec = tmpBVec;
}

ostream& operator<<(ostream& out, const Permutation& pm) {
        out << " Permutation size : " << pm.getSize() << endl;
        for (unsigned i = 0; i < pm.getSize(); i++) out << i << " : " << pm[i] << endl;
        return out;
}
