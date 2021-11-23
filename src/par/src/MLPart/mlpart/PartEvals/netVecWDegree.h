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

// created by Stefanus Mantik on 05/27/98

#ifndef __NETVECWDEGREE_H__
#define __NETVECWDEGREE_H__

#include <vector.h>
#include "netVec.h"

class NetVecGenericWDegree : public NetVecGeneric {
        void reinitializeProper() {
                unsigned numEdges = _hg.getNumEdges();
                _degree.clear();
                for (unsigned i = 0; i < numEdges; i++) _degree.push_back(0);

                itHGFEdgeGlobal e = _hg.edgesBegin();
                for (; e != _hg.edgesEnd(); e++) {
                        unsigned currentDegree = 0;

                        itHGFNodeLocal n = (*e)->nodesBegin();
                        for (; n != (*e)->nodesEnd(); n++) {
                                if (_hg.isTerminal((*n)->getIndex()))
                                        currentDegree += _terminalsCountAs;
                                else
                                        currentDegree++;
                        }
                        _degree[(*e)->getIndex()] = currentDegree;
                }
        }

       protected:
        vector<unsigned> _degree;

       public:
        NetVecGenericWDegree(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs = 0) : NetVecGeneric(problem, part, terminalsCountAs) { reinitializeProper(); }

        NetVecGenericWDegree(const PartitioningProblem& problem, unsigned terminalsCountAs = 0) : NetVecGeneric(problem, terminalsCountAs) { reinitializeProper(); }

        NetVecGenericWDegree(const HGraphFixed& hg, const Partitioning& part, unsigned nParts, unsigned terminalsCountAs = 0) : NetVecGeneric(hg, part, nParts, terminalsCountAs) { reinitializeProper(); }

        virtual ~NetVecGenericWDegree() {};

        virtual void reinitialize() { NetVecGeneric::reinitialize(); }
};

#endif
