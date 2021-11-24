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

// created by David Papa 22/11/05

#include "maxGain.h"

#include <algorithm>

using std::max;

MaxGain::MaxGain(const HGraphFixed& hg) {
        double accumWeight = 0.0;
        _maxGain = 0;
        for (unsigned i = 0; i < hg.getNumNodes(); ++i) {
                double nodeMaxGain = 0.0;
                itHGFEdgeLocal edgeOnNode = hg.getNodeByIdx(i).edgesBegin();
                for (unsigned j = 0; j < hg.getNodeByIdx(i).getDegree(); ++j, ++edgeOnNode) {
                        const unsigned e = (*edgeOnNode)->getIndex();
                        nodeMaxGain += hg.getEdgeByIdx(e).getWeight();
                }
                accumWeight += nodeMaxGain;
                _maxGain = max(_maxGain, static_cast<int>(rint(nodeMaxGain)));
        }

        _avgMaxGain = accumWeight / static_cast<double>(hg.getNumNodes());
}

int MaxGain::getMaxGain(void) { return _maxGain; }

double MaxGain::getAvgMaxGain(void) { return _avgMaxGain; }
