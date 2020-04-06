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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <Ctainers/bitBoard.h>
#include <HGraph/cluHGraph.h>

using std::cerr;
using std::endl;
using uofm::vector;

CluHGraph::CluHGraph(const HGraphFixed &hgraph, const vector<unsigned> &cluMap, unsigned numClusters, unsigned numTerminalClusters) {
        _finalized = false;
        _haveNameMaps = true;
        if (cluMap.size() != hgraph.getNumNodes()) {
                cerr << " Original hgraph has " << hgraph.getNumNodes() << " nodes, but"
                     << " the clustering map has " << cluMap.size() << "entries" << endl;
                abkfatal(0, "Size mismatch");
        }
        abkfatal(numClusters, "Cannot cluster up into 0 clusters");

        _numTerminals = numTerminalClusters;
        //    _nodeNames = vector<string>(numClusters);

        unsigned i, nw = hgraph.getNumWeights();
        abkfatal(nw == 1, "Not implemented for multi-weights");
        init(numClusters, nw);

        for (i = 0; i != numClusters; ++i) setWeight(i, 0.0);
        for (i = 0; i != cluMap.size(); ++i) {
                HGFNode &node = (*_nodes[cluMap[i]]);
                setWeight(node.getIndex(), getWeight(node.getIndex()) + hgraph.getWeight(i));
        }

        BitBoard bitBd(numClusters);
        itHGFEdgeGlobal edgeIt;
        for (edgeIt = hgraph.edgesBegin(); edgeIt != hgraph.edgesEnd(); ++edgeIt) {
                HGFEdge const &oldEdge = *(*edgeIt);
                if (oldEdge.getDegree() < 2) continue;  // without adding an edge
                itHGFNodeLocal adjIt = oldEdge.nodesBegin();
                unsigned cluIdx1 = cluMap[(*adjIt)->getIndex()], cluIdx2 = cluMap[(*++adjIt)->getIndex()];
                if (oldEdge.getDegree() == 2) {
                        if (cluIdx1 == cluIdx2) continue;  // without adding an edge
                        HGFEdge &newEdge = *addEdge(oldEdge.getWeight());
                        addSrcSnk(getNodeByIdx(cluIdx1), newEdge);
                        addSrcSnk(getNodeByIdx(cluIdx2), newEdge);
                        continue;
                }

                if (cluIdx1 == cluIdx2) {
                        bool edgeCollapses = true;  // let's try to prove this false
                        for (adjIt++; adjIt != oldEdge.nodesEnd(); ++adjIt)
                                if (cluIdx1 != cluMap[(*adjIt)->getIndex()]) {
                                        edgeCollapses = false;
                                        break;
                                }
                        if (edgeCollapses) continue;  // without adding an edge
                }

                bitBd.clear();
                HGFEdge &newEdge = *addEdge(oldEdge.getWeight());
                for (adjIt = oldEdge.nodesBegin(); adjIt != oldEdge.nodesEnd(); ++adjIt) {
                        unsigned idx = cluMap[(*adjIt)->getIndex()];
                        if (bitBd.isBitSet(idx)) continue;
                        bitBd.setBit(idx);
                        addSrcSnk(getNodeByIdx(idx), newEdge);
                }
        }
        finalize();
        clearNameMaps();
        clearNames();
}
