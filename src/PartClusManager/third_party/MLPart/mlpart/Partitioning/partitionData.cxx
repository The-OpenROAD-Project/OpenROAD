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

// 980426 ilm  added classes PartitioningBuffer and PartitioningDoubleBuffer
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <Partitioning/partitionData.h>
#include <algorithm>

using std::cout;
using std::endl;
using std::max;
using std::min;
using uofm::vector;

PartitioningSolution::PartitioningSolution(const Partitioning& srcPart, const HGraphFixed& hgraph, unsigned nParts, unsigned newCost) : part(const_cast<Partitioning&>(srcPart)), numPartitions(nParts), totalArea(0.), partCount(nParts, 0), partArea(nParts, vector<double>(hgraph.getNumWeights(), 0.0)), partPins(nParts, 0), cost(newCost), imbalance(DBL_MAX), violation(DBL_MAX) {
        itHGFNodeGlobal n;

        for (n = hgraph.nodesBegin(); n != hgraph.nodesEnd(); n++) {
                unsigned nodesPart = part[(*n)->getIndex()].lowestNumPart();
                if (nodesPart > numPartitions) {
                        cout << "Node in an invalid partition" << endl;
                        cout << "Node: " << (*n)->getIndex() << " in " << part[(*n)->getIndex()] << endl;
                }

                partCount[nodesPart]++;
                partPins[nodesPart] += (*n)->getDegree();
                totalArea += hgraph.getWeight((*n)->getIndex());

                for (unsigned b = 0; b < hgraph.getNumWeights(); ++b) {
                        partArea[nodesPart][b] += hgraph.getWeight((*n)->getIndex(), b);
                }
        }

        double sumOfParts = 0.0;
        unsigned minPins = UINT_MAX;
        unsigned maxPins = 0;

        for (unsigned k = 0; k < numPartitions; ++k) {
                sumOfParts += partArea[k][0];
                maxPins = max(maxPins, partPins[k]);
                minPins = min(minPins, partPins[k]);
        }

        abkfatal(equalDouble(sumOfParts, totalArea), "partitions do not sum to the total area");

        pinImbalance = maxPins - minPins;
}

void PartitioningSolution::calculatePartData(const HGraphFixed& hgraph) {
        totalArea = 0.;
        partCount = vector<unsigned>(numPartitions, 0);
        partArea = vector<vector<double> >(numPartitions, vector<double>(hgraph.getNumWeights(), 0.));
        partPins = vector<unsigned>(numPartitions, 0);

        itHGFNodeGlobal n;

        for (n = hgraph.nodesBegin(); n != hgraph.nodesEnd(); n++) {
                unsigned nodesPart = part[(*n)->getIndex()].lowestNumPart();
                if (nodesPart > numPartitions) {
                        cout << "Node in an invalid partition" << endl;
                        cout << "Node: " << (*n)->getIndex() << " in " << part[(*n)->getIndex()] << endl;
                }

                partCount[nodesPart]++;
                partPins[nodesPart] += (*n)->getDegree();
                totalArea += hgraph.getWeight((*n)->getIndex());

                for (unsigned b = 0; b < hgraph.getNumWeights(); ++b) {
                        partArea[nodesPart][b] += hgraph.getWeight((*n)->getIndex(), b);
                }
        }

        double sumOfParts = 0.0;
        unsigned minPins = UINT_MAX;
        unsigned maxPins = 0;

        for (unsigned k = 0; k < numPartitions; ++k) {
                sumOfParts += partArea[k][0];
                maxPins = max(maxPins, partPins[k]);
                minPins = min(minPins, partPins[k]);
        }

        abkfatal(equalDouble(sumOfParts, totalArea), "partitions do not sum to the total area");

        pinImbalance = maxPins - minPins;
}

//------------------
void PartitioningBuffer::setNumModulesUsed(unsigned modUsed) {
        abkfatal(modUsed < (*this)[0].size(), " Not enough modules in PartitioningBuffer ");
        _modulesUsed = modUsed;
}

void PartitioningBuffer::setBeginUsedSoln(unsigned firstUsedSoln) {
        abkfatal(firstUsedSoln < size(), " first used solution # too big");
        _firstUsedSoln = firstUsedSoln;
}

void PartitioningBuffer::setEndUsedSoln(unsigned lastUsedSoln) {
        while (lastUsedSoln > size()) this->push_back(Partitioning((*this)[0]));
        _lastUsedSoln = lastUsedSoln;
}

PartitioningDoubleBuffer::~PartitioningDoubleBuffer() {
        if (checkedOut) {
                char tmpbuf[300];
                sprintf(tmpbuf, "%s:%d\n", _checkedOutInFile, _checkedOutInLine);
                abkwarn2(0,
                         "PartitioningDoubleBuffer just destructed was checked "
                         "out in ",
                         tmpbuf);
        }
}

void PartitioningDoubleBuffer::setNumModulesUsed(unsigned modUsed) {
        _buf1.setNumModulesUsed(modUsed);
        _buf2.setNumModulesUsed(modUsed);
}

void PartitioningDoubleBuffer::setBeginUsedSoln(unsigned firstUsedSoln) {
        _buf1.setBeginUsedSoln(firstUsedSoln);
        _buf2.setBeginUsedSoln(firstUsedSoln);
}

void PartitioningDoubleBuffer::setEndUsedSoln(unsigned lastUsedSoln) {
        _buf1.setEndUsedSoln(lastUsedSoln);
        _buf2.setEndUsedSoln(lastUsedSoln);
}

unsigned PartitioningDoubleBuffer::getNumModulesUsed() const {
        abkfatal(_buf1.getNumModulesUsed() == _buf2.getNumModulesUsed(), "Conflicting PartitioningBuffers");
        return _buf1.getNumModulesUsed();
}

unsigned PartitioningDoubleBuffer::beginUsedSoln() const {
        abkfatal(_buf1.beginUsedSoln() == _buf2.beginUsedSoln(), "Conflicting PartitioningBuffers");
        return _buf1.beginUsedSoln();
}

unsigned PartitioningDoubleBuffer::endUsedSoln() const {
        abkfatal(_buf1.endUsedSoln() == _buf2.endUsedSoln(), "Conflicting PartitioningBuffers");
        return _buf1.endUsedSoln();
}

void PartitioningDoubleBuffer::swapBuf() {
        char tmpbuf[300];
        sprintf(tmpbuf, "%s:%d\n", _checkedOutInFile, _checkedOutInLine);

        abkfatal2(!checkedOut, "Can't swap PartitioningDoubleBuffer checked out in ", tmpbuf);

        abkfatal(_buf1.getNumModulesUsed() == _buf2.getNumModulesUsed(), "Conflicting PartitioningBuffers");
        abkfatal(_buf1.beginUsedSoln() == _buf2.beginUsedSoln(), "Conflicting PartitioningBuffers");
        abkfatal(_buf1.endUsedSoln() == _buf2.endUsedSoln(), "Conflicting PartitioningBuffers");
        std::swap(_mainBuf, _shadowBuf);
}

void PartitioningDoubleBuffer::checkout_(PartitioningBuffer*& mainBuf, PartitioningBuffer*& shadowBuf, const char* checkedOutInFile, unsigned checkedOutInLine) {
        if (checkedOut) {
                char tmpbuf[300];
                sprintf(tmpbuf, "%s:%d\n", _checkedOutInFile, _checkedOutInLine);

                abkfatal2(0, "Can't check out PartitioningDoubleBuffer checked out in ", tmpbuf);
        }

        abkfatal(_buf1.getNumModulesUsed() == _buf2.getNumModulesUsed(), "Conflicting PartitioningBuffers during check-out");
        abkfatal(_buf1.beginUsedSoln() == _buf2.beginUsedSoln(), "Conflicting PartitioningBuffers");
        abkfatal(_buf1.endUsedSoln() == _buf2.endUsedSoln(), "Conflicting PartitioningBuffers");

        _checkedOutInFile = checkedOutInFile;
        _checkedOutInLine = checkedOutInLine;

        mainBuf = _mainBuf;
        shadowBuf = _shadowBuf;
        checkedOut = true;
}

void PartitioningDoubleBuffer::checkInBuf(const PartitioningBuffer* mainBuf, const PartitioningBuffer* shadowBuf) {
        abkfatal(checkedOut,
                 "Can't check in PartitioningDoubleBuffer that wasn't checked "
                 "out\n");

        abkfatal(mainBuf == _mainBuf, "  Check-in error pointers do not match\n");
        abkfatal(shadowBuf == _shadowBuf, "  Check-in error: shadow pointers do not match\n");

        abkfatal(_buf1.getNumModulesUsed() == _buf2.getNumModulesUsed(), "Conflicting PartitioningBuffers during check-in");
        abkfatal(_buf1.beginUsedSoln() == _buf2.beginUsedSoln(), "Conflicting PartitioningBuffers");
        abkfatal(_buf1.endUsedSoln() == _buf2.endUsedSoln(), "Conflicting PartitioningBuffers");

        checkedOut = false;
}
