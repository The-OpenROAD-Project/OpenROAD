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

//////////////////////////////////////////////////////////////////////
//
// Created by Igor Markov on Oct 15, 2000
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CLUHGRAPH_H__INCLUDED_)
#define _CLUHGRAPH_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000

#include "hgFixed.h"

class CluHGraph : public HGraphFixed {
       protected:
        void _copyNodes(HGFEdge &newEdge, HGFEdge const &oldEdge, unsigned excludeIdx = UINT_MAX);

       public:
        CluHGraph(const HGraphFixed &hgraph, const uofm::vector<unsigned> &cluMap, unsigned numClusters, unsigned numTerminalClusters);
        CluHGraph(const HGraphFixed &hgraph, const uofm::vector<unsigned> &cluMap) { abkfatal(0, " Not implemented --- call a difft ctor "); }
};

#endif  // !defined(_CLUHGRAPH_H__INCLUDED_)
