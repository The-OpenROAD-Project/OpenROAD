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

#include "hgFixed.h"
#include "ABKCommon/abkcommon.h"
#include <ABKCommon/uofm_alloc.h>

using std::cerr;
using std::endl;
using uofm::vector;

enum Relation {
        None,
        Src,
        Snk,
        SrcSnk
};

class IncidencyMatrix {
        vector<Relation> _matrix;
        unsigned _numNodes;
        unsigned _numEdges;

       public:
        IncidencyMatrix(unsigned nodes, unsigned edges) : _matrix(nodes * edges, None), _numNodes(nodes), _numEdges(edges) {}

        Relation& operator()(unsigned node, unsigned edge) {
                abkassert(node < _numNodes, "node index too big");
                abkassert(edge < _numEdges, "edge index too big");
                return _matrix[node * _numEdges + edge];
        }
};

bool HGraphFixed::isConsistent() const {
        IncidencyMatrix matrix(getNumNodes(), getNumEdges());
        itHGFNodeGlobal nodeIt;
        itHGFEdgeLocal eIt;
        for (nodeIt = nodesBegin(); nodeIt != nodesEnd(); ++nodeIt) {
                const HGFNode& nd = **nodeIt;

#ifdef SIGNAL_DIRECTIONS
                for (eIt = nd.srcEdgesBegin(); eIt != nd.srcEdgesEnd(); ++eIt) matrix(nd.getIndex(), (*eIt)->getIndex()) = Src;

                for (eIt = nd.snkEdgesBegin(); eIt != nd.snkEdgesEnd(); ++eIt) matrix(nd.getIndex(), (*eIt)->getIndex()) = Snk;

                for (eIt = nd.srcSnkEdgesBegin(); eIt != nd.srcSnkEdgesEnd(); ++eIt) matrix(nd.getIndex(), (*eIt)->getIndex()) = SrcSnk;
#else
                for (eIt = nd.edgesBegin(); eIt != nd.edgesEnd(); ++eIt) matrix(nd.getIndex(), (*eIt)->getIndex()) = SrcSnk;
#endif
        }

        IncidencyMatrix matrix1(getNumNodes(), getNumEdges());

        itHGFEdgeGlobal edgeIt;
        itHGFNodeLocal nIt;
        for (edgeIt = edgesBegin(); edgeIt != edgesEnd(); ++edgeIt) {
                const HGFEdge& eg = **edgeIt;
#ifdef SIGNAL_DIRECTIONS
                for (nIt = eg.srcsBegin(); nIt != eg.srcsEnd(); ++nIt) matrix1((*nIt)->getIndex(), eg.getIndex()) = Src;

                for (nIt = eg.snksBegin(); nIt != eg.snksEnd(); ++nIt) matrix1((*nIt)->getIndex(), eg.getIndex()) = Snk;

                for (nIt = eg.srcSnksBegin(); nIt != eg.srcSnksEnd(); ++nIt) matrix1((*nIt)->getIndex(), eg.getIndex()) = SrcSnk;
#else
                for (nIt = eg.nodesBegin(); nIt != eg.nodesEnd(); ++nIt) matrix1((*nIt)->getIndex(), eg.getIndex()) = SrcSnk;
#endif
        }

        for (unsigned v = 0; v != getNumNodes(); ++v)
                for (unsigned e = 0; e != getNumEdges(); ++e)
                        if (matrix(v, e) != matrix1(v, e)) {
                                cerr << "node: " << v << " edge: " << e << endl << " nodes traversal yields status: " << matrix(v, e) << " edge traversal yields status: " << matrix1(v, e) << endl;
                                abkfatal(0, "consistency check failed");
                        }
        return true;
}

/*
static ostream& operator<<(ostream& out, Relation r)
{
   switch (r)
    {
        case None : out<<"None"; break;
        case Src : out<<"Src"; break;
        case Snk : out<<"Snk"; break;
        case SrcSnk : out<<"SrcSnk"; break;
    }
    return out;
}
*/
