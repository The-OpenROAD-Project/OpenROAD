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

//! author="Igor Markov June 28, 1997"
//! CHANGES = "permut.h 050398  Lexicographic step added by Max Moroz"
#ifndef _PERMUT_H_
#define _PERMUT_H_

#include "mapping.h"
#include "ABKCommon/abkcommon.h"
#include <ABKCommon/uofm_alloc.h>

class PermutationOrdering {
       public:
        virtual bool operator()(unsigned, unsigned) {
                abkfatal(0, "Must be overloaded");
                return false;
        }
        virtual ~PermutationOrdering() {}
};

//: A mapping from the finite set {0,...,N-1}
// to the finite set {0,...,N-1}.
// In a permutation, every element of the range is "hit", which
// means the mapping is "surjective" or "onto".    (A mapping that
// is both injective and surjective is a 1-to-1 correspondence.)
class Permutation : public Mapping {
       public:
        Permutation() : Mapping(0, 0) {}
        Permutation(unsigned size) : Mapping(size, size) {};
        Permutation(unsigned size, const unsigned data[]) : Mapping(size, size, data) {};
        Permutation(unsigned size, const uofm::vector<unsigned>& data) : Mapping(size, size, data) {};
        Permutation(unsigned size, Mapping::Example ex) : Mapping(size, size, ex) {};
        Permutation(const Mapping& mp) : Mapping(mp) {
                abkfatal(mp.getSourceSize() == mp.getDestSize(), "Can\'t convert mapping to permutation: not surjective");
        };
        Permutation(const Permutation& arg1, const Permutation& arg2) : Mapping(arg1, arg2) {
                // compose arg1(arg2)
                abkfatal(arg2.getDestSize() == arg1.getSourceSize(), "The product of mappings is not permutation");
        }
        Permutation(const Permutation& pm) : Mapping(pm) {};

        Permutation(const uofm::vector<double>& unsorted, double epsilon = 0.0);
        // "sorting permutation" construction

        unsigned getSize() const { return getSourceSize(); }
        Permutation& getInverse(Permutation& result) const;
        //  the size of the result should match the size of the permutation
        //     to be inverted

        inline bool nextLexicographic();
        // causes the permutation to change to the lexicographically
        // next permutation. It returns a bool, indicating the status
        // ..that is, if you hit the lexicographically 'last' permutation,
        // it returns false.  This is useful when looping thorough
        // all permutations.

        friend std::ostream& operator<<(std::ostream& out, const Permutation& pm);
};

template <class T>
void reorderVector(uofm::vector<T>& vec, const Permutation& pm) {
        abkassert(vec.size() == pm.getSize(), "Size mismatch during reordering");
        uofm::vector<T> tmpVec(vec.size());
        for (unsigned k = 0; k < vec.size(); k++) tmpVec[pm[k]] = vec[k];
        vec = tmpVec;
}

template <class T>
void reorderVectorBack(uofm::vector<T>& vec, const Permutation& pm) {
        abkassert(vec.size() == pm.getSize(), "Size mismatch during reordering");
        uofm::vector<T> tmpVec(vec.size());
        for (unsigned k = 0; k < vec.size(); k++) tmpVec[k] = vec[pm[k]];
        vec = tmpVec;
}

void reorderBitVectorBack(uofm_bit_vector& bvec, const Permutation& pm);
void reorderBitVector(uofm_bit_vector& bvec, const Permutation& pm);

/*------------------------      INTERFACES END HERE    -------------------
------------------------ IMPLEMENTATION BEGINS HERE  -------------------*/

inline void _swapInsideNextLex(unsigned& a, unsigned& b) {
        unsigned tmp = a;
        a = b;
        b = tmp;
}

inline bool Permutation::nextLexicographic() {
        unsigned n = getSize();
        // unsigned* v=_meat;
        int k = n - 2;  //
        //
        while (k >= 0 && _meat[k] > _meat[k + 1]) k--;
        if (k < 0)
                return 0;
        else {
                int j = n - 1;  //
                while (_meat[k] > _meat[j]) j--;
                _swapInsideNextLex(_meat[j], _meat[k]);
                int r = n - 1;  //
                int s = k + 1;
                while (r > s) {
                        _swapInsideNextLex(_meat[r], _meat[s]);
                        r--;
                        s++;
                }
        }
        return 1;
}

#endif
