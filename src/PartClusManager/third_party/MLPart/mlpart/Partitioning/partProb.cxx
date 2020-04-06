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

// created by Andrew Caldwell on 03/16/98  caldwell@cs.ucla.edu

// CHANGES

// 980320 ilm  safety measures in _set methods : a pointer can be newed
//                    only if it is NULL
// 980407 aec  added minCapacities data and functions
// 980520 ilm  added _computeXYTicSet(const vector<BBox>&);
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <Partitioning/partProb.h>
#include <Partitioning/partitionData.h>
#include <Partitioning/termiProp.h>
#include <set>

using std::cout;
using std::endl;
using std::setw;
using std::less;
using std::max;
using std::set;
using uofm::vector;

PartitioningProblem::PartitioningProblem(const HGraphFixed& hgraph, const PartitioningBuffer& solnBuffers, const Partitioning& fixedConstr, const vector<BBox>& partitions, const vector<vector<double> >& capacities, const vector<double>& tolerances, const vector<unsigned>& terminalToBlock, const vector<BBox>& padBlocks, Parameters parameters) : _params(parameters), _hgraph(const_cast<HGraphFixed*>(&hgraph)), _solnBuffers(const_cast<PartitioningBuffer*>(&solnBuffers)), _costs(solnBuffers.size(), -1), _violations(solnBuffers.size(), -1), _imbalances(solnBuffers.size(), -1), _fixedConstr(const_cast<Partitioning*>(&fixedConstr)), _partitions(const_cast<vector<BBox>*>(&partitions)), _capacities(const_cast<vector<vector<double> >*>(&capacities)), _maxCapacities(NULL), _minCapacities(NULL), _totalWeight(NULL), _terminalToBlock(const_cast<vector<unsigned>*>(&terminalToBlock)), _padBlocks(const_cast<vector<BBox>*>(&padBlocks)) {
        abkfatal(_partitions->size() == _capacities->size(), "partition size does not match capacities size");
        abkfatal(tolerances.size() == (*_capacities)[0].size(), "tolerences size does not match capacitites.size");

        abkfatal(_terminalToBlock->size() >= _hgraph->getNumTerminals(), "terminalToBlock size does not match hgraph.numTerminals");

        abkfatal(fixedConstr.size() == _hgraph->getNumNodes(), "fixed constraints must be given for every hypergraph node\n");

        //  _clusterDegrees = NULL;
        _ownsData = false;

        _setTotalWeight();
        _setMaxCapacities(tolerances, capacities);
        _setMinCapacities(tolerances, capacities);

        _postProcess();
}

void PartitioningProblem::setSolnBuffers(PartitioningBuffer* newbufs)
    //  LOOK: deletes the old ones !!! will own the new ones !!! -ILM
    //  to be used for reordering partitioning solutions
{
        abkfatal(_ownsData, "can't call setSolnBuffers if PartProb does not own data");

        delete _solnBuffers;
        _solnBuffers = newbufs;
        _costs.clear();
        _violations.clear();
        _imbalances.clear();
        _costs.insert(_costs.end(), _solnBuffers->size(), -1);
        _violations.insert(_violations.end(), _solnBuffers->size(), -1);
        _imbalances.insert(_imbalances.end(), _solnBuffers->size(), -1);
}

// same as setSolnBuffers except does *not* delete old ones.
// return value is old buffers.
PartitioningBuffer* PartitioningProblem::swapOutSolnBuffers(PartitioningBuffer* newbufs) {
        // abkfatal(_ownsData,
        //		"can't call swapOutBuffers if PartProb does not own
        // data");

        PartitioningBuffer* retval = _solnBuffers;
        _solnBuffers = newbufs;
        _costs.clear();
        _violations.clear();
        _imbalances.clear();
        _costs.insert(_costs.end(), _solnBuffers->size(), -1);
        _violations.insert(_violations.end(), _solnBuffers->size(), -1);
        _imbalances.insert(_imbalances.end(), _solnBuffers->size(), -1);
        return retval;
}

void PartitioningProblem::_freeDynamicData() {
        if (_ownsData) {
                delete _hgraph;
                if (_solnBuffers) delete _solnBuffers;

                delete _fixedConstr;
                delete _partitions;
                delete _capacities;
                delete _maxCapacities;
                delete _minCapacities;
                delete _totalWeight;
                delete _partitionCenters;
                delete _terminalToBlock;
                delete _padBlocks;
                delete _padBlockCenters;
                //      delete   _clusterDegrees;
        } else {
                if (_partitionCenters != NULL) {
                        delete _partitionCenters;
                        _partitionCenters = NULL;
                }
                if (_padBlockCenters != NULL) {
                        delete _padBlockCenters;
                        _padBlockCenters = NULL;
                }
                if (_maxCapacities != NULL) {
                        delete _maxCapacities;
                        _maxCapacities = NULL;
                }
                if (_minCapacities != NULL) {
                        delete _minCapacities;
                        _minCapacities = NULL;
                }
                if (_totalWeight != NULL) {
                        delete _totalWeight;
                        _totalWeight = NULL;
                }
                //	if(_clusterDegrees!=NULL){delete
                //_clusterDegrees;_clusterDegrees=NULL;}
        }
}

void PartitioningProblem::_postProcess() {
        //_setTrivUSparseMatrix(); // does nothing if there is a USparseMat
        // already
        if (_partitions != NULL) _setPartitionCenters();
        if (_padBlocks != NULL && _terminalToBlock != NULL) _setPadBlockCenters();
        _computeXYTics(*_partitions);
        _attributes.clearAll();

        if (_capacities != NULL) _attributes.hasCap = true;
        if (_maxCapacities != NULL) _attributes.hasMaxCap = true;
        if (_minCapacities != NULL) _attributes.hasMinCap = true;
        if (_partitions != NULL) _attributes.hasPartBBoxes = true;
        if (_partitionCenters != NULL) _attributes.hasPartCenters = true;
        if (_padBlocks != NULL) _attributes.hasPadBBoxes = true;
        if (_padBlockCenters != NULL) _attributes.hasPadCenters = true;
        if (_totalWeight != NULL) _attributes.hasTotalWeight = true;
        //  if(_clusterDegrees != NULL) _attributes.hasClusterDegrees = true;

        PartitionIds freeToMove, allOnes;
        freeToMove.setToAll(getNumPartitions());
        allOnes.setToAll(allOnes.totalBits());

        for (unsigned k = 0; k != _fixedConstr->size(); k++) {
                PartitionIds& fixedPart = (*_fixedConstr)[k];
                if (fixedPart.isEmpty() || fixedPart == allOnes) fixedPart = freeToMove;
                for (unsigned n = _solnBuffers->beginUsedSoln(); n != _solnBuffers->endUsedSoln(); n++) {
                        PartitionIds& solPart = ((*_solnBuffers)[n])[k];
                        if (solPart.isEmpty())
                                solPart = fixedPart;
                        else
                                solPart &= fixedPart;
                }
        }
}

/*
void PartitioningProblem::_setTrivUSparseMatrix()
{
    if (_hgraph==NULL || _clusterDegrees!=NULL) return;
    _clusterDegrees
       =new USparseMatrix(_hgraph->getNumEdges(),_hgraph->getNumNodes());

    // fill the matrix here with 1s where appropriate
    for (itHGFNodeGlobal v=(*_hgraph).nodesBegin();
                        v!=(*_hgraph).nodesEnd(); ++v)
        for (itHGFEdgeLocal e=(*v)->edgesBegin(); e!=(*v)->edgesEnd(); e++)
            (*_clusterDegrees)((*e)->getIndex(),(*v)->getIndex())=1;
}
*/

void PartitioningProblem::_setMaxCapacities(const vector<double>& tolerances, const vector<vector<double> >& capacities) {
        // compute the max value for each partition/capacity
        // from tolerances and capacities

        if (_maxCapacities) return;

        unsigned numCapacities = tolerances.size();

        _maxCapacities = new vector<vector<double> >(capacities.size(), vector<double>(numCapacities));

        for (unsigned b = 0; b < capacities.size(); b++) {
#ifdef PARTDEBUG
                abkfatal(capacities[b].size() == numCapacities, "all partitions must have the same number of capacities");
#endif

                for (int x = capacities[b].size() - 1; x >= 0; --x) {
                        (*_maxCapacities)[b][x] = capacities[b][x];
                }

                for (unsigned c = 0; c < numCapacities; c++) {
#ifdef PARTDEBUG
                        abkfatal(tolerances[c] < 1.0,
                                 "suspicious tolerance > 1. Tolerances should "
                                 "be in percent");
#endif
                        (*_maxCapacities)[b][c] *= tolerances[c] + 1;
                }
        }
}

void PartitioningProblem::_setMinCapacities(const vector<double>& tolerances, const vector<vector<double> >& capacities) {
        // compute the min value for each partition/capacity
        // from tolerances and capacities

        if (_minCapacities) return;

        unsigned numCapacities = tolerances.size();

        _minCapacities = new vector<vector<double> >(capacities.size(), vector<double>(numCapacities));

        for (unsigned b = 0; b < capacities.size(); b++) {
#ifdef PARTDEBUG
                abkfatal(capacities[b].size() == numCapacities, "all partitions must have the same number of capacities");
#endif

                for (int x = capacities[b].size() - 1; x >= 0; --x) {
                        (*_minCapacities)[b][x] = capacities[b][x];
                }

                for (unsigned c = 0; c < numCapacities; c++) {
#ifdef PARTDEBUG
                        abkfatal(tolerances[c] < 1.0,
                                 "suspicious tolerance > 1. Tolerances should "
                                 "be in percent");
#endif
                        (*_minCapacities)[b][c] *= 1 - tolerances[c];
                }
        }
}

void PartitioningProblem::_setPadBlockCenters() {
        if (_padBlocks == NULL) return;

        _padBlockCenters = new vector<Point>(_padBlocks->size());
        for (unsigned k = 0; k < _padBlocks->size(); k++) {
                (*_padBlockCenters)[k] = (*_padBlocks)[k].getGeomCenter();
        }
}

void PartitioningProblem::_setPartitionCenters() {
        _partitionCenters = new vector<Point>(_partitions->size());
        for (unsigned k = 0; k < _partitions->size(); k++) (*_partitionCenters)[k] = (*_partitions)[k].getGeomCenter();
}

void PartitioningProblem::_setTotalWeight() {
        // compute the total non-terminal weight for each weight
        if (_totalWeight) return;
        abkfatal(_hgraph, " Can't compute total weights --- no hypergraph");
        unsigned numWeights = _hgraph->getNumWeights();
        _totalWeight = new vector<double>(numWeights, 0.0);

        itHGFNodeGlobal nItr = _hgraph->nodesBegin();  //+_hgraph->getNumTerminals();
        for (; nItr != _hgraph->nodesEnd(); nItr++) {
                for (unsigned w = 0; w < numWeights; w++) (*_totalWeight)[w] += _hgraph->getWeight((*nItr)->getIndex(), w);
        }
}

void PartitioningProblem::reserveBuffers(unsigned num) {
        _solnBuffers->setEndUsedSoln(num);
        _solnBuffers->setBeginUsedSoln(0);

        int toInsert = num - _costs.size();
        if (toInsert > 0) {
                _costs.insert(_costs.end(), toInsert, -1);
                _violations.insert(_violations.end(), toInsert, -1);
                _imbalances.insert(_imbalances.end(), toInsert, -1);
        }
}

// copy ctor
PartitioningProblem::PartitioningProblem(const PartitioningProblem& prob) : _params(prob._params), _attributes(prob._attributes), _ownsData(true), _bestSolnNum(prob._bestSolnNum), _costs(prob._costs), _violations(prob._violations), _imbalances(prob._imbalances) { _copyDynamicData(prob); }

void PartitioningProblem::_copyDynamicData(const PartitioningProblem& prob) {
        _capacities = new vector<vector<double> >(*prob._capacities);

        /*  if (prob._clusterDegrees)
                _clusterDegrees = new USparseMatrix(*prob._clusterDegrees);
            else _clusterDegrees=NULL;
        */

        _fixedConstr = new Partitioning(*prob._fixedConstr);
        _hgraph = new HGraphFixed(*prob._hgraph);

        if (prob._maxCapacities)
                _maxCapacities = new vector<vector<double> >(*prob._maxCapacities);
        else
                _maxCapacities = NULL;

        if (prob._minCapacities)
                _minCapacities = new vector<vector<double> >(*prob._minCapacities);
        else
                _minCapacities = NULL;

        if (prob._padBlockCenters)
                _padBlockCenters = new vector<Point>(*prob._padBlockCenters);
        else
                _padBlockCenters = NULL;

        _padBlocks = new vector<BBox>(*prob._padBlocks);

        if (prob._partitionCenters)
                _partitionCenters = new vector<Point>(*prob._partitionCenters);
        else
                _partitionCenters = NULL;

        _partitions = new vector<BBox>(*prob._partitions);

        _solnBuffers = new PartitioningBuffer(*prob._solnBuffers);

        _terminalToBlock = new vector<unsigned>(*prob._terminalToBlock);

        if (prob._totalWeight)
                _totalWeight = new vector<double>(*prob._totalWeight);
        else
                _totalWeight = NULL;
}

PartitioningProblem& PartitioningProblem::operator=(const PartitioningProblem& prob) {
        _attributes = prob._attributes;
        _bestSolnNum = prob._bestSolnNum;
        _costs = prob._costs;
        _violations = prob._violations;
        _imbalances = prob._imbalances;
        _params = prob._params;
        _freeDynamicData();
        _ownsData = true;
        _copyDynamicData(prob);
        return *this;
}

void PartitioningProblem::propagateTerminals(double fuzziness)  // can be 10 (%)
{
        abkfatal(_terminalToBlock && _padBlocks, " terminal block info missing ");
        unsigned numParts = _partitions->size();
        PartitionIds freeToMove;
        freeToMove.setToAll(numParts);

        TerminalPropagator propag(*_partitions, fuzziness);
        unsigned termNo = 0;

        for (vector<unsigned>::const_iterator termIt = _terminalToBlock->begin(); termIt != _terminalToBlock->end(); ++termIt, ++termNo) {
                if (*termIt == UINT_MAX) continue;
                PartitionIds& constrParts = (*_fixedConstr)[termNo];
                PartitionIds propagatedParts;

                propag.doOneTerminal((*_padBlocks)[*termIt], constrParts, propagatedParts);
                constrParts = propagatedParts;

                for (unsigned k = _solnBuffers->beginUsedSoln(); k != _solnBuffers->endUsedSoln(); ++k) {
                        ((*_solnBuffers)[k])[termNo] = constrParts;
                }
        }
}

void PartitioningProblem::printPinBalances() const {
        if (_bestSolnNum == UINT_MAX) {
                cout << " The best solution is not available " << endl;
                return;
        }

        cout << "Pin balances: " << getPinBalances();
        return;
}

vector<unsigned> PartitioningProblem::getPinBalances() const {
        vector<unsigned> pinBalances(getNumPartitions(), 0);
        if (_bestSolnNum == UINT_MAX) return pinBalances;

        const Partitioning& part = (*_solnBuffers)[_bestSolnNum];

        vector<PartitionIds> samples(getNumPartitions());
        unsigned k;
        for (k = 0; k != getNumPartitions(); k++) samples[k].setToPart(k);

        for (unsigned m = 0; m != _hgraph->getNumNodes(); m++) {
                for (k = 0; k != getNumPartitions(); k++)
                        if (part[m] == samples[k]) break;
                if (k == getNumPartitions()) {
                        cout << " Node " << m << " is not assigned to one partition :" << part[m] << endl;
                        abkfatal(0, "\n");
                }
                pinBalances[k] += _hgraph->getNodeByIdx(m).getDegree();
        }
        return pinBalances;
}

void PartitioningProblem::printLargestCellStats(unsigned N) const {
        unsigned m, numMovables = _hgraph->getNumNodes();

        const Permutation& sortAscWts_1 = _hgraph->getNodesSortedByWeights();
        if (numMovables < N) N = numMovables;

        double totalArea = (*_totalWeight)[0];
        unsigned soln = (_bestSolnNum == UINT_MAX ? 0 : _bestSolnNum);
        const Partitioning& part = (*_solnBuffers)[soln];

        cout << setw(4) << N << "  largest cells: \n";
        unsigned k = 1;
        for (m = numMovables - 1; m != numMovables - 1 - N; m--) {
                unsigned nodeIdx = sortAscWts_1[m];
                HGFNode& node = _hgraph->getNodeByIdx(nodeIdx);
                double nodeWt = _hgraph->getWeight(node.getIndex());
                double percent = 100.0 * nodeWt / totalArea;
                unsigned degree = node.getDegree();
                cout << setw(4) << k++ << ". area: " << setw(10) << nodeWt << " (" << setw(10) << percent << "%)  degree:" << setw(3) << degree << " assignment: " << part[nodeIdx];
        }
}

void PartitioningProblem::_computeXYTics(const vector<BBox>& bboxes) {
        set<double, less<double> > _xTicSet, _yTicSet;
        for (unsigned i = 0; i != bboxes.size(); i++) {
                _xTicSet.insert(bboxes[i].xMin);
                _xTicSet.insert(bboxes[i].xMax);
                _yTicSet.insert(bboxes[i].yMin);
                _yTicSet.insert(bboxes[i].yMax);
        }

        /*
        cout<<"num bboxes(partitions) "<<bboxes.size()<<endl;
        cout<<"xTicSet: "<<_xTicSet.size()<<endl;
        cout<<"yTicSet: "<<_yTicSet.size()<<endl;
        cout<<"BOXES "<<bboxes<<endl;
        */

        if ((_xTicSet.size() - 1) * (_yTicSet.size() - 1) != bboxes.size()) {
                if (_params.verbosity.getForActions() > 1)
                        abkwarn(0,
                                " BBoxes do not come from a grid (may be "
                                "bloated/fuzzy)\n");
                return;
        }

        _xTics.clear();
        _yTics.clear();

        std::set<double, less<double> >::iterator setIt;
        for (setIt = _xTicSet.begin(); setIt != _xTicSet.end(); setIt++) _xTics.push_back(*setIt);

        std::set<double, less<double> >::reverse_iterator rsetIt;
        for (rsetIt = _yTicSet.rbegin(); rsetIt != _yTicSet.rend(); rsetIt++) _yTics.push_back(*rsetIt);

        abkfatal(_xTicSet.size() > 1 && _yTicSet.size() > 1, " Internal error in PartProblem: only one tic for x- and/or y- ");
}

extern double getBBoxQuantization(unsigned quantizationBound, const vector<Point>& pts);

double PartitioningProblem::getScalingFactorForBBoxQuantization(unsigned quantizationBound) const { return getBBoxQuantization(quantizationBound, *_partitionCenters); }

extern double getBBoxQuantization(unsigned quantizationBound, const vector<Point>& pts) {

        abkfatal(pts.size(), "Can not process empty point-set");
        set<double, less<double> > _xTicSet, _yTicSet;
        for (unsigned i = 0; i != pts.size(); i++) {
                _xTicSet.insert(pts[i].x);
                _yTicSet.insert(pts[i].y);
        }

        if (_xTicSet.size() * _yTicSet.size() != pts.size())
                abkwarn(0,
                        " BBoxQuantization: Given points do not come from a "
                        "grid.\n");

        vector<double> xcTics, ycTics;

        std::set<double, less<double> >::iterator setIt;
        for (setIt = _xTicSet.begin(); setIt != _xTicSet.end(); setIt++) xcTics.push_back(*setIt);

        for (setIt = _yTicSet.begin(); setIt != _yTicSet.end(); setIt++) ycTics.push_back(*setIt);

        int k;
        for (k = xcTics.size() - 1; k >= 0; k--) xcTics[k] -= xcTics[0];
        for (k = ycTics.size() - 1; k >= 0; k--) ycTics[k] -= ycTics[0];

        // this is somewhat redundant for now, but will be used later

        abkfatal(xcTics.size() > 0 && ycTics.size() > 0, "Internal error in PartProb");

        if (xcTics.size() == 1) {
                if (ycTics.size() == 1) return 1.0;
                return 1 / ycTics[1];
        } else if (ycTics.size() == 1)
                return 1 / xcTics[1];

        double delta = 1.0 / (max(xcTics.back(), ycTics.back()));
        double a = quantizationBound / (xcTics.back() + ycTics.back());
        double dx = xcTics[1];
        double dy = ycTics[1];

        // Now we will assume that finding a delta-gauge of dx and dy is enough
        // (improve later)

        double result = a / abkGcd(static_cast<unsigned>(rint(dx * a)), static_cast<unsigned>(rint(dy * a)));

        if (result < delta) result = delta;

        abkfatal3(result * (xcTics.back() + ycTics.back()) < quantizationBound, " BBox quantization failed: quantization bound ", quantizationBound, " exceeded ");

        return result;
}

void PartitioningProblem::temporarilyIncreaseTolerance(const vector<double>& byHowMuch, const vector<double>& currPartAreas) {
        abkassert(byHowMuch.size() <= _maxCapacities->size(), "byHowMuch too large");
        abkassert(currPartAreas.size() <= _maxCapacities->size(), "currPartAreas too large");
        abkassert(byHowMuch.size() <= _minCapacities->size(), "byHowMuch too large");
        abkassert(currPartAreas.size() <= _maxCapacities->size(), "currPartAreas too large");

        _maxCapacitiesTemp = _maxCapacities;
        _maxCapacities = new vector<vector<double> >(*_maxCapacitiesTemp);

        _minCapacitiesTemp = _minCapacities;
        _minCapacities = new vector<vector<double> >(*_minCapacitiesTemp);

        /*
        if(currPartAreas[0] < currPartAreas[1])
          {
            (*_maxCapacities)[0][0] = currPartAreas[0]+byHowMuch[0];
            (*_maxCapacities)[0][0] = std::max((*_maxCapacities)[0][0],
        (*_maxCapacitiesTemp)[0][0]);
            (*_minCapacities)[0][0] = currPartAreas[0]-byHowMuch[0];
            (*_minCapacities)[0][0] = std::min((*_minCapacities)[0][0],
        (*_minCapacitiesTemp)[0][0]);
            (*_minCapacities)[0][0] = std::max((*_minCapacities)[0][0], 0.0);

            (*_maxCapacities)[1][0] = (*_totalWeight)[0]*1.02 +
        (*_minCapacities)[0][0];
            (*_minCapacities)[1][0] = (*_maxCapacities)[0][0] -
        (*_totalWeight)[0]*1.02;
          }
        else
          {
            (*_maxCapacities)[1][0] = currPartAreas[1]+byHowMuch[0];
            (*_maxCapacities)[1][0] = std::max((*_maxCapacities)[1][0],
        (*_maxCapacitiesTemp)[1][0]);
            (*_minCapacities)[1][0] = currPartAreas[1]-byHowMuch[0];
            (*_minCapacities)[1][0] = std::min((*_minCapacities)[1][0],
        (*_minCapacitiesTemp)[1][0]);
            (*_minCapacities)[1][0] = std::max((*_minCapacities)[1][0], 0.0);

            (*_maxCapacities)[0][0] = (*_totalWeight)[0]*1.02 +
        (*_minCapacities)[1][0];
            (*_minCapacities)[0][0] = (*_maxCapacities)[1][0] -
        (*_totalWeight)[0]*1.02;
          }
        */

        /*
        for(unsigned j = 0; j<_maxCapacities->size();++j)
          for(unsigned i = 0;i<byHowMuch.size();++i)
            {
              //make sure adding byHowMuch doesnt increase beyond 30%
              //(*_maxCapacities)[j][i] = std::min((*_totalWeight)[i]*0.3 +
        (*_capacities)[j][i],(*_maxCapacities)[j][i]+byHowMuch[i]);
              //(*_maxCapacities)[j][i] = (*_capacities)[j][i]+byHowMuch[i];
              //(*_maxCapacities)[j][i] = std::max((*_maxCapacities)[j][i],
        (*_maxCapacitiesTemp)[j][i]);


              (*_maxCapacities)[j][i] = currPartAreas[j]+byHowMuch[i];
              (*_maxCapacities)[j][i] = std::max((*_maxCapacities)[j][i],
        (*_maxCapacitiesTemp)[j][i]);
            }

        //now decrease the minCapacities
        for(unsigned j = 0; j<_minCapacities->size();++j)
          for(unsigned i = 0;i<byHowMuch.size();++i)
            {
              //make sure adding byHowMuch doesnt decrease beyond 30%
              //(*_minCapacities)[j][i] = (*_capacities)[j][i]-byHowMuch[i];
              //(*_minCapacities)[j][i] = std::min((*_minCapacities)[j][i],
        (*_minCapacitiesTemp)[j][i]);
              //(*_minCapacities)[j][i] = std::max((*_minCapacities)[j][i],
        0.0);


              (*_minCapacities)[j][i] = currPartAreas[j]-byHowMuch[i];
              (*_minCapacities)[j][i] = std::min((*_minCapacities)[j][i],
        (*_minCapacitiesTemp)[j][i]);
              (*_minCapacities)[j][i] = std::max((*_minCapacities)[j][i], 0.0);
            }
        */

        for (unsigned j = 0; j < _maxCapacities->size(); ++j)
                for (unsigned i = 0; i < byHowMuch.size(); ++i) {
                        (*_maxCapacities)[j][i] = (*_capacities)[j][i] + byHowMuch[i];
                        (*_maxCapacities)[j][i] = std::max((*_maxCapacities)[j][i], (*_maxCapacitiesTemp)[j][i]);
                }

        // now decrease the minCapacities
        for (unsigned j = 0; j < _minCapacities->size(); ++j)
                for (unsigned i = 0; i < byHowMuch.size(); ++i) {
                        (*_minCapacities)[j][i] = (*_capacities)[j][i] - byHowMuch[i];
                        (*_minCapacities)[j][i] = std::min((*_minCapacities)[j][i], (*_minCapacitiesTemp)[j][i]);
                        (*_minCapacities)[j][i] = std::max((*_minCapacities)[j][i], 0.0);
                }

        // now limit the max tolerance to 80% for each partition
        for (unsigned j = 0; j < _maxCapacities->size(); ++j)
                for (unsigned i = 0; i < byHowMuch.size(); ++i) {
                        double diff = (*_maxCapacities)[j][i] - (*_minCapacities)[j][i];
                        if ((diff / (*_totalWeight)[i]) > 0.8) {
                                double meanCap = ((*_maxCapacities)[j][i] + (*_minCapacities)[j][i]) / 2.0;
                                (*_maxCapacities)[j][i] = meanCap + (*_totalWeight)[i] * 0.4;
                                (*_minCapacities)[j][i] = meanCap - (*_totalWeight)[i] * 0.4;
                        }
                }
}
// called to restore the tolerance to what it was before the most recent call to
// temporarilyIncreaseTolerance()

void PartitioningProblem::revertTolerance(void) {
        if (_maxCapacities == _maxCapacitiesTemp || _maxCapacitiesTemp == NULL) return;
        delete _maxCapacities;
        _maxCapacities = _maxCapacitiesTemp;

        if (_minCapacities == _minCapacitiesTemp || _minCapacitiesTemp == NULL) return;
        delete _minCapacities;
        _minCapacities = _minCapacitiesTemp;
}
