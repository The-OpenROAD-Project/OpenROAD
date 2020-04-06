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

//  Adapted by Igor Markov from an earlier prototype by Andrew Caldwell
//  March 3, 1998

//  Reworked for standalone ML by Igor Markov and Andy Caldwell, March 30, 1998

// CHANGES
//  970304 ilm removed _allNets and calcMinMaxes()   (see Capo/Archive)
//             moved _calcTerminals into _resetTerminals
//  980305 ilm removed fixedConstraint handling for Partitioner::Parameters
//  980325 ilm reworked minMaxes with _part*Bounds* for n*m-way support
//  980412 ilm reworked minMaxes with _part*Bounds* for n*m-way support
//  980413 ilm split into callPartitioner.cxx, rebuildTree.cxx and remainder
//  980912 aec version4.0 with ClusteredHGraph
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "baseMLPart.h"
#include "ClusteredHGraph/clustHGraph.h"
#include "Partitioning/partitioning.h"
#include "Partitioners/multiStartPart.h"
#include "HGraph/subHGraph.h"

using uofm::vector;

unsigned BaseMLPart::_majCallCounter = 0;
unsigned BaseMLPart::_minCallCounter = 0;

BaseMLPart::BaseMLPart(PartitioningProblem& problem, const Parameters& params, BBPartBitBoardContainer& bbBitBoards, MaxMem* maxMem, FillableHierarchy* hier) : _userTime(-1), _bestCost(UINT_MAX), _aveCost(UINT_MAX), _numLegalSolns(UINT_MAX), _totalClusteringTime(0.0), _totalUnClusteringTime(0.0), _bestSolnPartWeights(problem.getNumPartitions(), vector<double>(problem.getHGraphPointer()->getNumWeights(), -1.0)), _bestSolnPartNumNodes(problem.getNumPartitions(), UINT_MAX), _bestCostSoFar(UINT_MAX), _bestCostsMidWayCost(problem.getHGraphPointer()->getNumEdges()), _midWayPoint(problem.getHGraphPointer()->getNumNodes()), _problem(problem), _hgraphs(NULL), _params(params), _doingCycling(false), _soln2Buffers(NULL), _bestSolnNum(UINT_MAX), _fixedConstraints(NULL), _terminalBlocksPtr(NULL), _termBlockBBoxesPtr(NULL), _levelsToGo(UINT_MAX), _callPartTime(0.0), _partitioner(NULL), _bbBitBoards(bbBitBoards), _maxMem(maxMem), _hierarchy(hier) {
        abkfatal(_params.pruningPoint > 1 && _params.pruningPoint < 100, "pruningPoint out of range");

        const HGraphFixed* hgraph = problem.getHGraphPointer();

        _midWayPoint = static_cast<unsigned>(rint(hgraph->getNumNodes() * (_params.pruningPoint / 100.0)));
}

void BaseMLPart::populate(ClusteredHGraph& hgraphs, const vector<unsigned>& terminalBlocks, const vector<BBox>& termBlockBBoxes) {
        _hgraphs = &hgraphs;

        _terminalBlocksPtr = &terminalBlocks;
        _termBlockBBoxesPtr = &termBlockBBoxes;
        _resetTerminals();  // moveables are set in the derived class ctor
}

BaseMLPart::~BaseMLPart() {
        if (_fixedConstraints) delete _fixedConstraints;
        if (_partitioner) delete _partitioner;
        if (_soln2Buffers) delete _soln2Buffers;
}

void BaseMLPart::_resetTerminals() {
        if (_fixedConstraints == NULL) _fixedConstraints = new Partitioning(_problem.getFixedConstr());

        PartitioningBuffer* mainBuf = NULL, *shadowBuf = NULL;
        _soln2Buffers->checkOutBuf(mainBuf, shadowBuf);

        unsigned numTerminals = _hgraphs->getNumTerminals();
        for (unsigned nodeId = 0; nodeId < numTerminals; nodeId++) {
                unsigned boxNum = (*_terminalBlocksPtr)[nodeId];
                if (boxNum == UINT_MAX) continue;
                const BBox& termBBox = (*_termBlockBBoxesPtr)[boxNum];
                PartitionIds terminalPart = _calcTerminalPartitions(termBBox.getGeomCenter());

                (*_fixedConstraints)[nodeId] = terminalPart;
                for (unsigned k = mainBuf->beginUsedSoln(); k < mainBuf->endUsedSoln(); k++) {
                        ((*mainBuf)[k])[nodeId] = terminalPart;
                        ((*shadowBuf)[k])[nodeId] = terminalPart;
                }
        }
        _soln2Buffers->checkInBuf(mainBuf, shadowBuf);
}

PartitionIds BaseMLPart::_calcTerminalPartitions(Point termLoc) {
        TerminalPropagator propag(_problem.getPartitions(), _params.partFuzziness);

        PartitionIds terminalPart, fixedConstr;
        BBox terminalBox(termLoc);
        propag.doOneTerminal(terminalBox, fixedConstr, terminalPart);
        return terminalPart;
}

double BaseMLPart::getPartitionRatio(unsigned partNumber) const { return getPartitionArea(partNumber) / (getPartitionArea(partNumber) + getPartitionArea(1 - partNumber)); }

double BaseMLPart::getPartitionArea(unsigned partNumber) const {
        abkassert(partNumber < _bestSolnPartWeights.size(), "Partition index out of range");
        return _bestSolnPartWeights[partNumber][0];
}

unsigned BaseMLPart::getPartitionNumNodes(unsigned partNumber) const {
        abkassert(partNumber < _bestSolnPartNumNodes.size(), "Partition index out of range");
        return _bestSolnPartNumNodes[partNumber];
}
