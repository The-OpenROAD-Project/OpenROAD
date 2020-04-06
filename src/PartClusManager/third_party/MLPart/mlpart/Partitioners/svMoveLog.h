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

//! author="Igor Markov on 06/06/98"

#ifndef _SVMOVELOG_H_
#define _SVMOVELOG_H_

#include "Combi/partitionIds.h"
#include <utility>

//: single vertex move log
class SVMoveLog {
        Partitioning* _part;
        uofm::vector<unsigned> _moduleIdx;  // both vectors are used as stacks
        uofm::vector<PartitionIds> _oldIds;

       public:
        SVMoveLog() : _part(NULL) {}
        SVMoveLog(Partitioning& part) : _part(&part) {
                _moduleIdx.reserve(part.size());
                _oldIds.reserve(part.size());
        }

        void clear() {
                _moduleIdx.clear();
                _oldIds.clear();
        }
        bool isEmpty() const { return _moduleIdx.empty() || _oldIds.empty(); }
        unsigned getSize() const { return std::min(_moduleIdx.size(), _oldIds.size()); }

        void undo(unsigned count)  // undo the last k moves
        {
                abkassert(count <= _moduleIdx.size(), "count is too large");
                abkassert(_moduleIdx.size() == _oldIds.size(), "size mismatch in log");

                uofm::vector<unsigned>::iterator mIt = _moduleIdx.end();
                uofm::vector<PartitionIds>::iterator oIt = _oldIds.end();

                for (unsigned k = count; k != 0; k--) (*_part)[*(--mIt)] = *(--oIt);

                _moduleIdx.erase(mIt, _moduleIdx.end());
                _oldIds.erase(oIt, _oldIds.end());
        }

        void logMove(unsigned moduleIdx, PartitionIds oldIds) {
                _moduleIdx.push_back(moduleIdx);
                _oldIds.push_back(oldIds);
        }

        void popMove(std::pair<unsigned, PartitionIds>& lastMove);

        void resetTo(Partitioning& part) {
                _part = &part;
                clear();
                _moduleIdx.reserve(part.size());
                _oldIds.reserve(part.size());
        }

        void writeTo(const char*) const;
};

inline void SVMoveLog::popMove(std::pair<unsigned, PartitionIds>& lastMove) {
        lastMove.first = _moduleIdx.back();
        lastMove.second = _oldIds.back();
        _moduleIdx.pop_back();
        _oldIds.pop_back();
}

#endif
