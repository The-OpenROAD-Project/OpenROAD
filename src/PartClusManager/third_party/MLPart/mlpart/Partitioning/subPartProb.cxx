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

#include <ABKCommon/uofm_alloc.h>
#include <HGraph/subHGraph.h>
#include <Partitioning/subPartProb.h>

using std::cout;
using std::endl;
using uofm::vector;

SubPartitioningProblem::SubPartitioningProblem(const LayoutBBoxes& partition, const HGraphFixed& hgraphOfNetlist, const Placement& placement) : PartitioningProblem(), _ptrPartitionsLB(NULL), _partitionsLB(partition), _placement(placement), _hgraphOfNetlist(hgraphOfNetlist) {
        _bestSolnNum = UINT_MAX;
        _capacities = new vector<vector<double> >(partition.size(), vector<double>(1, 0));
        _partitions = new LayoutBBoxes(partition);
        _totalWeight = new vector<double>(1, 0);

        vector<double> tol(1, 0.1);

        setHGraph();
        setBuffer();

        _setMaxCapacities(tol, *_capacities);
        _setMinCapacities(tol, *_capacities);
        PartitionIds movable;
        movable.setToAll(partition.size());
        _fixedConstr = new Partitioning(_hgraph->getNumNodes(), movable);
}

SubPartitioningProblem::SubPartitioningProblem(const vector<BBox>& partBBoxes, const HGraphFixed& hgraphOfNetlist, const Placement& placement) : PartitioningProblem(), _ptrPartitionsLB(new LayoutBBoxes(partBBoxes)), _partitionsLB(*_ptrPartitionsLB), _placement(placement), _hgraphOfNetlist(hgraphOfNetlist) {
        _bestSolnNum = UINT_MAX;
        _capacities = new vector<vector<double> >(partBBoxes.size(), vector<double>(1, 0));
        _partitions = new LayoutBBoxes(_partitionsLB);
        _totalWeight = new vector<double>(1, 0);

        vector<double> tol(1, 0.1);

        setHGraph();
        setBuffer();

        _setMaxCapacities(tol, *_capacities);
        _setMinCapacities(tol, *_capacities);
        PartitionIds movable;
        movable.setToAll(_partitions->size());
        _fixedConstr = new Partitioning(_hgraph->getNumNodes(), movable);
}

void SubPartitioningProblem::setBuffer() {
        unsigned numStarts;
        unsigned numNodes = _hgraph->getNumNodes();

        if (numNodes >= 500)
                numStarts = 8;
        else if (numNodes >= 250)
                numStarts = 8;
        else if (numNodes >= 100)
                numStarts = 8;
        else
                numStarts = 4;

        _solnBuffers = new PartitioningBuffer(_hgraph->getNumNodes(), numStarts);
        _solnBuffers->setBeginUsedSoln(0);
        _solnBuffers->setEndUsedSoln(numStarts);
}

void SubPartitioningProblem::setHGraph() {
        uofm_bit_vector insideWindow(_hgraphOfNetlist.getNumNodes(), false);
        // node is inside window
        uofm_bit_vector visitedNode(_hgraphOfNetlist.getNumNodes(), false);
        // node has been checked
        uofm_bit_vector visitedEdge(_hgraphOfNetlist.getNumEdges(), false);
        // edge has been checked

        vector<const HGFNode*> nonTerminals;
        vector<const HGFNode*> terminals;
        vector<const HGFEdge*> edges;

        itHGFNodeGlobal n;

        for (n = _hgraphOfNetlist.nodesBegin(); n != _hgraphOfNetlist.nodesEnd(); ++n) {
                const unsigned index = (*n)->getIndex();

                if (!_hgraphOfNetlist.isTerminal(index) && _partitionsLB.contains(_placement[index])) {
                        insideWindow[index] = visitedNode[index] = true;
                        nonTerminals.push_back(*n);

                        itHGFEdgeLocal e;
                        for (e = (*n)->edgesBegin(); e != (*n)->edgesEnd(); e++) {
                                unsigned edgeIdx = (*e)->getIndex();

                                if (!visitedEdge[edgeIdx]) {
                                        visitedEdge[edgeIdx] = true;
                                        edges.push_back(*e);

                                        itHGFNodeLocal adjN;
                                        for (adjN = (*e)->nodesBegin(); adjN != (*e)->nodesEnd(); adjN++) visitedNode[(*adjN)->getIndex()] = true;
                                }
                        }

                        // calculate the capacity
                        (*_capacities)[_partitionsLB.locate(_placement[index])][0] += _hgraphOfNetlist.getWeight((*n)->getIndex());
                        // calculate totalWeight
                        (*_totalWeight)[0] += _hgraphOfNetlist.getWeight((*n)->getIndex());
                }
        }

        _terminalToBlock = new vector<unsigned>;
        _padBlocks = new vector<BBox>;
        _padBlockCenters = new vector<Point>;
        unsigned blockID = 0;

        for (n = _hgraphOfNetlist.nodesBegin(); n != _hgraphOfNetlist.nodesEnd(); n++) {
                unsigned index = (*n)->getIndex();

                if (visitedNode[index] && !insideWindow[index]) {
                        terminals.push_back(*n);

                        Point terminalLoc = _placement[index];
                        _padBlockCenters->push_back(terminalLoc);
                        _padBlocks->push_back(BBox(terminalLoc));
                        _terminalToBlock->push_back(blockID);
                        blockID++;
                        abkwarn(blockID == _padBlocks->size(), "block ID does not match");
                }
        }

        cout << "SubPartProb thinks there are " << terminals.size() << " Pads" << endl;

        _hgraph = new SubHGraph(_hgraphOfNetlist, nonTerminals, terminals, edges);
}

SubPartitioningProblem::~SubPartitioningProblem() {
        if (_capacities != NULL) {
                delete _capacities;
                _capacities = NULL;
        }
        if (_totalWeight != NULL) {
                delete _totalWeight;
                _totalWeight = NULL;
        }
        if (_padBlocks != NULL) {
                delete _padBlocks;
                _padBlocks = NULL;
        }
        if (_terminalToBlock != NULL) {
                delete _terminalToBlock;
                _terminalToBlock = NULL;
        }
        if (_padBlockCenters != NULL) {
                delete _padBlockCenters;
                _padBlockCenters = NULL;
        }
        if (_partitions != NULL) {
                delete _partitions;
                _partitions = NULL;
        }
        if (_hgraph != NULL) {
                delete _hgraph;
                _hgraph = NULL;
        }
        if (_solnBuffers != NULL) {
                delete _solnBuffers;
                _solnBuffers = NULL;
        }
        if (_fixedConstr != NULL) {
                delete _fixedConstr;
                _fixedConstr = NULL;
        }
        if (_ptrPartitionsLB != NULL) {
                delete _ptrPartitionsLB;
                _ptrPartitionsLB = NULL;
        }
}

void SubPartitioningProblem::makeVanilla() {
        abkfatal(_partitions->size() == 2, "makeVanilla only works for 2 way");

        BBox& p0Box = (*_partitions)[0];
        BBox& p1Box = (*_partitions)[1];
        _padBlocks->erase(_padBlocks->begin(), _padBlocks->end());
        _padBlocks->insert(_padBlocks->begin(), BBox());
        _padBlocks->insert(_padBlocks->begin(), BBox());
        _padBlocks->insert(_padBlocks->begin(), BBox());
        abkfatal(_padBlocks->size() == 3, "padBlocks size error");

        if (p0Box.xMin == p1Box.xMin)  // horiz partitioning
        {
                p0Box.xMin = p1Box.xMin = 0;
                p0Box.xMax = p1Box.xMax = 100;
                p1Box.yMin = 0;
                p1Box.yMax = p0Box.yMin = 100;
                p0Box.yMax = 200;
                (*_padBlocks)[0] += Point(50, 200);  // p0 is the top partition
                (*_padBlocks)[1] += Point(50, 0);    // p1 is the bottom partition
                (*_padBlocks)[2] += Point(50, 100);  // for terms in both partitions
        } else if (p0Box.yMin == p1Box.yMin)         // vertical partitioning
        {
                p0Box.yMin = p1Box.yMin = 0;
                p0Box.yMax = p1Box.yMax = 100;
                p0Box.xMin = 0;
                p0Box.xMax = p1Box.xMin = 100;
                p1Box.xMax = 200;
                (*_padBlocks)[0] += Point(0, 50);    // p0 is the left partition
                (*_padBlocks)[1] += Point(200, 50);  // p1 is the right
                                                     // partition
                (*_padBlocks)[2] += Point(100, 50);  // for terms in both partitions
        } else
                abkfatal(0, "must have aligning xMins or yMins");

        // set the targets for each partition to 1/2 the total cell area
        for (unsigned w = 0; w < _totalWeight->size(); w++) {
                double halfWeight = (*_totalWeight)[w] / 2.0;
                (*_capacities)[0][w] = halfWeight;
                (*_capacities)[1][w] = halfWeight;
                (*_maxCapacities)[0][w] = halfWeight * 1.1;
                (*_maxCapacities)[1][w] = halfWeight * 1.1;
                (*_minCapacities)[0][w] = halfWeight * 0.9;
                (*_minCapacities)[1][w] = halfWeight * 0.9;
        }

        for (unsigned n = 0; n < _hgraph->getNumTerminals(); n++) {
                if ((*_fixedConstr)[n].isInPart(0) && (*_fixedConstr)[n].isInPart(1))
                        (*_terminalToBlock)[n] = 2;
                else if ((*_fixedConstr)[n].isInPart(0))
                        (*_terminalToBlock)[n] = 0;
                else if ((*_fixedConstr)[n].isInPart(1))
                        (*_terminalToBlock)[n] = 1;
                else
                        abkfatal(0, "terminal not propagated to any partition");
        }
}
