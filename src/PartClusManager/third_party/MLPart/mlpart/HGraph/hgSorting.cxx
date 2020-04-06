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

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <HGraph/hgFixed.h>
#include <Stats/stats.h>

using std::sort;
using std::random_shuffle;
using uofm::vector;

void HGraphFixed::sortNodes() {

        if (_param.nodeSortMethod == HGraphParameters::SORT_NODES_BY_INDEX) {
                HGNodeSortByIndex compFun;
                for (itHGFEdgeGlobal edgeIt = edgesBegin(); edgeIt != edgesEnd(); ++edgeIt) {
                        HGFEdge& edge = **edgeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(edge._nodes.begin(), edge._srcSnksBegin, compFun);
                        sort(edge._srcSnksBegin, edge._snksBegin, compFun);
                        sort(edge._snksBegin, edge._nodes.end(), compFun);
#else
                        sort(edge._nodes.begin(), edge._nodes.end(), compFun);
#endif
                }
        } else if (_param.nodeSortMethod == HGraphParameters::SORT_NODES_BY_ASCENDING_DEGREE) {
                HGNodeSortByAscendingDegree compFun;
                for (itHGFEdgeGlobal edgeIt = edgesBegin(); edgeIt != edgesEnd(); ++edgeIt) {
                        HGFEdge& edge = **edgeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(edge._nodes.begin(), edge._srcSnksBegin, compFun);
                        sort(edge._srcSnksBegin, edge._snksBegin, compFun);
                        sort(edge._snksBegin, edge._nodes.end(), compFun);
#else
                        sort(edge._nodes.begin(), edge._nodes.end(), compFun);
#endif
                }
        } else if (_param.nodeSortMethod == HGraphParameters::SORT_NODES_BY_DESCENDING_DEGREE) {
                HGNodeSortByDescendingDegree compFun;
                for (itHGFEdgeGlobal edgeIt = edgesBegin(); edgeIt != edgesEnd(); ++edgeIt) {
                        HGFEdge& edge = **edgeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(edge._nodes.begin(), edge._srcSnksBegin, compFun);
                        sort(edge._srcSnksBegin, edge._snksBegin, compFun);
                        sort(edge._snksBegin, edge._nodes.end(), compFun);
#else
                        sort(edge._nodes.begin(), edge._nodes.end(), compFun);
#endif
                }
        } else if (_param.nodeSortMethod == HGraphParameters::DONT_SORT_NODES)
                return;
        else
                abkfatal(0, "incorrect nodeSortMethod value in Parameters");
}

void HGraphFixed::sortEdges() {
        if (_param.edgeSortMethod == HGraphParameters::SORT_EDGES_BY_INDEX) {
                HGEdgeSortByIndex compFun;
                for (itHGFNodeGlobal nodeIt = nodesBegin(); nodeIt != nodesEnd(); ++nodeIt) {
                        HGFNode& node = **nodeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(node._edges.begin(), node._srcSnksBegin, compFun);
                        sort(node._srcSnksBegin, node._snksBegin, compFun);
                        sort(node._snksBegin, node._edges.end(), compFun);
#else
                        sort(node._edges.begin(), node._edges.end(), compFun);
#endif
                }
        } else if (_param.edgeSortMethod == HGraphParameters::SORT_EDGES_BY_ASCENDING_DEGREE) {

                HGEdgeSortByAscendingDegree compFun;
                for (itHGFNodeGlobal nodeIt = nodesBegin(); nodeIt != nodesEnd(); ++nodeIt) {
                        HGFNode& node = **nodeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(node._edges.begin(), node._srcSnksBegin, compFun);
                        sort(node._srcSnksBegin, node._snksBegin, compFun);
                        sort(node._snksBegin, node._edges.end(), compFun);
#else
                        sort(node._edges.begin(), node._edges.end(), compFun);
#endif
                }
        } else if (_param.edgeSortMethod == HGraphParameters::SORT_EDGES_BY_DESCENDING_DEGREE) {
                HGEdgeSortByDescendingDegree compFun;
                for (itHGFNodeGlobal nodeIt = nodesBegin(); nodeIt != nodesEnd(); ++nodeIt) {
                        HGFNode& node = **nodeIt;
#ifdef SIGNAL_DIRECTIONS
                        sort(node._edges.begin(), node._srcSnksBegin, compFun);
                        sort(node._srcSnksBegin, node._snksBegin, compFun);
                        sort(node._snksBegin, node._edges.end(), compFun);
#else
                        sort(node._edges.begin(), node._edges.end(), compFun);
#endif
                }
        } else if (_param.edgeSortMethod == HGraphParameters::DONT_SORT_EDGES)
                return;
        else
                abkfatal(0, "incorrect edgeSortMethod value in Parameters");
}

void HGraphFixed::sortAsDB() {
        for (itHGFNodeGlobal nodeIt = nodesBegin(); nodeIt != nodesEnd(); ++nodeIt) {
                HGFNode& node = **nodeIt;
#ifdef SIGNAL_DIRECTIONS
                sort(node._edges.begin(), node._srcSnksBegin, HGEdgeSortByIndex());
                sort(node._srcSnksBegin, node._snksBegin, HGEdgeSortByIndex());
                sort(node._snksBegin, node._edges.end(), HGEdgeSortByIndex());
#else
                sort(node._edges.begin(), node._edges.end(), HGEdgeSortByIndex());
#endif
        }
}

void HGraphFixed::computeNodesSortedByWeights() const {
        HGraphFixed* fakeThis = const_cast<HGraphFixed*>(this);
        vector<unsigned> sortedNumbers(getNumNodes());

        for (int v = sortedNumbers.size() - 1; v >= 0; --v) {
                sortedNumbers[v] = v;
        }

        sort(sortedNumbers.begin(), sortedNumbers.end(), HGNodeIdSortByWeights(*this));

        fakeThis->_weightSort = Permutation(getNumNodes(), sortedNumbers);
}

//////TODO
void HGraphFixed::computeNodesSortedByWeightsWShuffle() const {
        vector<unsigned> sortedNumbers(getNumNodes());

        for (int v = sortedNumbers.size() - 1; v >= 0; --v) {
                sortedNumbers[v] = v;
        }

        sort(sortedNumbers.begin(), sortedNumbers.end(), HGNodeIdSortByWeights(*this));

        double totwt = 0.0;
        for (unsigned i = 0; i < getNumNodes(); i++) totwt += getWeight(i);

        double epsilon = 0.005 * totwt;
        unsigned idx = 0;
        for (vector<unsigned>::iterator it = sortedNumbers.begin(); it != sortedNumbers.end(); ++it, ++idx) {
                vector<unsigned>::iterator it2;
                double currW = getWeight(*it);
                it2 = it + 1;
                ++idx;
                while (it2 != sortedNumbers.end()) {
                        if (fabs(getWeight(*it2) - currW) < epsilon) break;
                        ++it2;
                        ++idx;
                }
                random_shuffle(it, it2);
                it = sortedNumbers.begin() + idx;
                // it = it2;

                if (it == sortedNumbers.end()) break;
        }

        // TODO: faster get total weight function

        double accumwt = 0.0;
        const double maxaccum = 0.50;       // 50% of total node area
        const double maxSingleNode = 0.05;  // 5% of total node area

        vector<unsigned>::reverse_iterator rit;
        for (rit = sortedNumbers.rbegin(); rit != sortedNumbers.rend(); ++rit) {
                double wt = getWeight(*rit);
                accumwt += wt;
                if (wt < maxSingleNode * totwt) break;
                if (accumwt > maxaccum * totwt) break;
        }
        random_shuffle(sortedNumbers.rbegin(), rit);
        _weightSort = Permutation(getNumNodes(), sortedNumbers);
}

void HGraphFixed::computeNodesSortedByDegrees() const {
        HGraphFixed* fakeThis = const_cast<HGraphFixed*>(this);
        vector<unsigned> sortedNumbers(getNumNodes());

        for (int v = sortedNumbers.size() - 1; v >= 0; --v) {
                sortedNumbers[v] = v;
        }

        sort(sortedNumbers.begin(), sortedNumbers.end(), HGNodeIdSortByDegrees(*this));

        fakeThis->_degreeSort = Permutation(getNumNodes(), sortedNumbers);
}
