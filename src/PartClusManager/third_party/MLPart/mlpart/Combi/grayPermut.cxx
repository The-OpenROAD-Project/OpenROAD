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

#include "ABKCommon/abkcommon.h"
#include "grayPermut.h"
#include <ABKCommon/uofm_alloc.h>

using std::swap;
using std::min;
using std::max;
using std::sort;
using uofm::vector;

// requires ~0.5 GB RAM
const unsigned maxSize = 12;
byte* GrayTranspositionForPermutations::_tables[maxSize];

/* Idea behind the following class:
(see e.g., Trotter, PERM (Algorithm 115), Comm. ACM, 5 (1962))

let size==4; and assume we have g3, a GrayPermPosGen object of size 3; then
the sequence is as follows

2,1,0,g3()+1,0,1,2,g3(),2,1,0,g3()+1,0,1,2,g3(),2,1,...

we define a state as consisting of
    _current (which can be 0,1,2,...);
    _dir (which can be left or right);
    _special (which can indicate g3() or g3()+1 output)

and define the obvious state transitions

we also add _finished flag to track down possible internal errors whereby
not all permutations would be generated (either by the GrayPermPosGen
recursively calling GrayPermPosGen, or by initTable calling GrayPermPosGen)
*/

class GrayPermPosGen {
        unsigned _size;
        unsigned _remains;
        GrayPermPosGen* _recursive;
        enum {
                LEFT,
                RIGHT
        } _dir;
        unsigned _current;
        enum {
                NONE,
                RECUR0,
                RECUR1
        } _special;
        bool _finished;

       public:
        GrayPermPosGen(unsigned size) {
                abkfatal(size > 1, "cannot generate permutations of size 0 or 1");
                _size = size;
                _remains = abkFactorial(size) - 1;
                _dir = LEFT;
                _current = size - 2;
                _special = NONE;
                _finished = false;
                if (size == 2)  // GrayPermPosGen(1) isn't supposed to ever be called
                        _recursive = 0;
                else
                        _recursive = new GrayPermPosGen(size - 1);
        }
        ~GrayPermPosGen() {
                // the error message would report the outermost object's
                // problem,
                // which is what we want for debugging
                // (the innermost error gets reported if we swap the following
                // two lines)
                abkassert2(!_remains || _finished, "internal error: too few calls to GrayPermPosGen of size ", _size);
                delete _recursive;
        }
        unsigned operator()() {
                abkassert2(!_finished, "internal error: too many calls to GrayPermPosGen of size ", _size);
                if (!_remains--) {
                        _finished = true;
                        return CHAR_MAX;
                }
                switch (_special) {
                        case RECUR0:
                                abkassert(_current == _size - 2, "internal error");
                                _dir = LEFT;
                                _special = NONE;
                                return (*_recursive)();
                        case RECUR1:
                                abkassert(_current == 0, "internal error");
                                _dir = RIGHT;
                                _special = NONE;
                                return (*_recursive)() + 1;
                        case NONE:
                                if (_current == 0 && _dir == LEFT) {
                                        _special = RECUR1;
                                        return _current;
                                }
                                if (_current == _size - 2 && _dir == RIGHT) {
                                        _special = RECUR0;
                                        return _current;
                                }
                                switch (_dir) {
                                        case LEFT:
                                                if (_current == 0) {
                                                        _special = RECUR1;
                                                        return _current;
                                                } else
                                                        return _current--;
                                        case RIGHT:
                                                if (_current == _size - 2) {
                                                        _special = RECUR0;
                                                        return _current;
                                                } else
                                                        return _current++;
                                }
                }
                abkfatal(0, "Internal error");
                // satisfy compiler
                return CHAR_MAX;
        }
};

void GrayTranspositionForPermutations::initTable(unsigned size) {
        if (size > maxSize)
                abkfatal3(0,
                          "Go get yourself a decent computer, man! Due to the "
                          "pitiful size of RAM in your box, "
                          "GrayTranspositionForPermutations instance size "
                          "cannot exceed ",
                          maxSize,
                          "; if you were only smart enough to get 80 gigs of "
                          "RAM, we could take you to size 14!");
        unsigned tableSize = abkFactorial(size);
        if (size > 9)
                abkwarn3(0,
                         "note: GrayTranspositionForPermutations of this size "
                         "will need around ",
                         tableSize / (1024. * 1024.),
                         " MB of RAM;\n"
                         "this storage will be allocated just once, for all "
                         "instances of this size\n");
        _tables[size] = new byte[tableSize];
        byte* ptr = _tables[size];
        GrayPermPosGen gen(size);
        while ((*ptr++ = gen()) != CHAR_MAX)
                ;
}

/*
we basically need fast deranking algorithm
idea: the biggest element moves as follows as .next() is applied:
3 2 1 0 0 1 2 3 , etc. (this is the repeated subsequence for _size = 4)
thus, the current permutation's index module (_size*2) uniquely determines the
position of the biggest element in the permutation
the position of the second biggest element is determined by the similar
periodic sequence of the (_size-1) permutations; so all that remains to solve
the problem recursively is the index in the sequence of permutations of the
one-smaller size; this index is obviously equal to the number of RECUR? items
encountered, since they (and only they) cause the one-smaller permutation to
advance further in its own sequence
*/

void setPos(byte* positions, unsigned size, unsigned index) {
        if (size == 1) {
                positions[0] = 0;
                return;
        }
        unsigned tmp = index % (2 * size);
        if (tmp < size)
                positions[size - 1] = size - 1 - tmp;
        else
                positions[size - 1] = tmp - size;
        setPos(positions, size - 1, index / size);
}

Permutation GrayTranspositionForPermutations::getPermutation() const {
        // abkfatal(0,"will be implemented on 13th May, 1998");
        unsigned index = _current - _tables[_size];
        byte positions[maxSize];
        setPos(positions, _size, index);
        // now positions[i] is the position the element i should take
        // from among the positions left empty after all bigger elements are
        // placed
        // positions is consistent iff for any i: positions[i]<=i
        const byte available = 100;
        vector<unsigned> perm(_size, available);
        for (int element = _size - 1; element >= 0; --element) {
                int count = positions[element];
                for (unsigned j = 0; j != _size; ++j)
                        if (perm[j] == available)
                                if (!count--) {
                                        perm[j] = element;
                                        break;
                                }
        }
        return Permutation(_size, perm);
}
