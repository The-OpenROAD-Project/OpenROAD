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

// class derived from HyperGraph with Constructor from
// nodes/edges of another HGraph.
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <HGraph/subHGraph.h>

using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

SubHGraph::SubHGraph(const HGraphFixed& origHGraph, const vector<const HGFNode*>& nonTerminals, const vector<const HGFNode*>& terminals, const vector<const HGFEdge*>& nets, bool clearTerminalAreas, bool ignoreSize1Nets, unsigned threshold)
    : HGraphFixed(nonTerminals.size() + terminals.size()),
      newNodeToOrig(nonTerminals.size() + terminals.size(), UINT_MAX),
      // newEdgeToOrig(nets.size(), UINT_MAX),
      newEdgeToOrig(0),
      _clearTerminalAreas(clearTerminalAreas),
      _ignoreSize1Nets(ignoreSize1Nets),
      _threshold(threshold) {
        _numTerminals = terminals.size();
        //    _nodeNames = vector<string>(_nodes.size(), string() );
        newEdgeToOrig = new Mapping(nets.size(), UINT_MAX);

        // the HGraph constructor produces the HG nodes with
        // id's 0->numTerminals + numNonTerminals - 1 in the default
        // constructor.
        // setup a mapping from Cluster::_index to HGFNode::mIndex
        // put the terminal nodes first..

        unsigned i, p;

        for (p = 0, i = 0; p < terminals.size(); p++, i++) addTerminal(origHGraph, *(terminals[p]), i);

        for (p = 0; p < nonTerminals.size(); p++, i++) addNode(origHGraph, *(nonTerminals[p]), i);

        vector<const HGFEdge*>::const_iterator e;

        for (e = nets.begin(), i = 0; e != nets.end(); e++, i++) {
                if (_ignoreSize1Nets && (*e)->getDegree() < 2) continue;
                if (threshold > 0 && (*e)->getDegree() >= threshold) continue;

                HGFEdge& newEdge = addNewEdge(**e);

                itHGFNodeLocal n;
                for (n = (*e)->nodesBegin(); n != (*e)->nodesEnd(); n++) {
                        OrigToNewMap::iterator ndItr = origNodeToNew.find((*n)->getIndex());
                        unsigned hgNodeIdx = (*ndItr).second;

#ifdef SIGNAL_DIRECTIONS
                        if ((*e)->isNodeSrc(*n))
                                addSrc(getNodeByIdx(hgNodeIdx), newEdge);
                        else if ((*e)->isNodeSnk(*n))
                                addSnk(getNodeByIdx(hgNodeIdx), newEdge);
                        else
                                addSrcSnk(getNodeByIdx(hgNodeIdx), newEdge);
#else
                        addSrcSnk(getNodeByIdx(hgNodeIdx), newEdge);
#endif
                }
        }
        finalize();
        clearNameMaps();
        clearNames();
}

SubHGraph::~SubHGraph() {}

void SubHGraph::printNodeMap() const {
        cout << endl << "*****************" << endl << "Node's HASH_MAP" << endl;

        OrigToNewMap::const_iterator m;
        unsigned* toprint;
        toprint = reinterpret_cast<unsigned*>(&m);
        unsigned i = 0;

        for (m = origNodeToNew.begin(); m != origNodeToNew.end(); m++) {
                if (i++ % 5 == 0) {
                        if (i) cout << "\n";
                        cout << " ";
                }
                cout << setw(4) << (*m).first << ":" << setw(4) << (*m).second << endl;
        }
        cout << "*********************"
             << "done printing" << endl;
}
