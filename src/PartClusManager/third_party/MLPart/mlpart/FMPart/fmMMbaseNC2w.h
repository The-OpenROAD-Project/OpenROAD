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

// created on 09/17/98 by Igor Markov

#ifndef _FM_MOVE_MANAGER_BASE_NETCUT2WAYH_
#define _FM_MOVE_MANAGER_BASE_NETCUT2WAYH_

#include "Partitioners/mmTemplBase.h"
#include "PartLegality/1balanceChk.h"
#include "dcGainCont.h"
#include "Partitioners/svMoveLog.h"
#include "maxGain.h"

#include <sstream>

class FMMMbaseNC2w : public MoveManagerInterface {
       protected:
        uofm_bit_vector _lockedModules;
        uofm_bit_vector _lockedNets;
        uofm::vector<unsigned> _lockedNetConfigIds;

        unsigned _numMovable;
        unsigned _numTerminals;
        unsigned _numParts;

        MaxGain _maxGain;                  // ORDER DEPENDENCY
        DestCentricGainContainer _gainCo;  // ORDER DEPENDENCY
        SingleBalanceLegality _partLeg;
        SVMoveLog _moveLog;

        unsigned _movingModuleIdx;
        unsigned _moveFromPart;
        unsigned _moveToPart;
        unsigned _numIllegalMoves;

        bool _allowCorkingNodes;
        double _tol;  // used to exclude nodes w/ weight
        //>= the balance tol from the gainCo
        //(they are treated as locked)

        bool _solnLegalAtStart;  // reset at the
        // begining of each pass, this is
        // true if the solution was true
        // at the time.

        //          virtual bool  pickMove()=0;
        /* false if a move can not be chosen, true otherwise */
        /* side effect: stores the result as 3 unsigneds */
        /* calls isMoveLegal() and can change gain container, thus not const */

        inline bool isMoveLegal() const /* verifies all PartLegalities */
        {
                return _partLeg.isLegalMove(_movingModuleIdx, _moveFromPart, _moveToPart);
        }

        //          virtual
        void initializeGainBuckets();

        void computeGainsOfModule(unsigned moduleIdx);
        void computeGainsOfModuleWW(unsigned moduleIdx);
        //          virtual void  computeGains()=0;  // using the above

        //  inline virtual void   updateGainOfModule
        //         (unsigned moduleIdx, unsigned from, unsigned to, int
        // deltaGain)=0;

        //         virtual void   updateAffectedModules()=0;
        //         virtual void   updateGains()=0

        inline void lockModule(unsigned moduleIdx) {
                _gainCo.removeAllNodesElements(moduleIdx);
                _lockedModules[moduleIdx] = true;
        }

       public:
        FMMMbaseNC2w(const PartitioningProblem& problem, const PartitionerParams& params, bool allowCorkingNodes = false);

        virtual ~FMMMbaseNC2w() {
                if (_tallies) delete[] _tallies;
        }

        virtual void setupClip() { _gainCo.setupClip(); }
        virtual bool pickMoveApplyIt() = 0; /* calls pickMove(), updateGains(),
                             _moveLog.logMove() and updates *_curPart */

        virtual void reinitialize();
        virtual void reinitTolerances();

        virtual void resetTo(PartitioningSolution& newSol);
        void undo(unsigned count) { _moveLog.undo(count); }
        const SVMoveLog& getMoveLog() const { return _moveLog; }

        virtual unsigned getCost() const { return getTotalCost(); }
        virtual double getCostDouble() const { return getCost(); }

        virtual double getImbalance() const { return _partLeg.getDiffFromIdeal(); }

        virtual double getViolation() const { return _partLeg.getViolation(); }
        virtual const uofm::vector<double>& getAreas() const { return _partLeg.getActualBalances(); }

       protected:  // this protected section is used for keeping track of
                   // tallies only
        // and is thus an elimination of the PartEval library

        const Partitioning* _part;            // maintained by caller
        const PartitioningProblem* _problem;  // use only to get info
                                              // not available otherwise
        const HGraphFixed& _hg;
        unsigned _terminalsCountAs;
        unsigned short* _tallies;

        unsigned _totalCost;
        uofm::vector<unsigned> _netCosts;  // at some point we can take this
                                           // away to make
        // a truly abstract class(children can use doubles)

        uofm::vector<unsigned> _edgeWeights;
        unsigned _maxEdgeWeight;
        bool _useWts;
        /*	NetCut2way(const PartitioningProblem& problem, const
          Partitioning& part);
          NetCut2way(const PartitioningProblem& problem);
          NetCut2way(const HGraphFixed& hg, const Partitioning& part);

          ~NetCut2way() { delete[] _tallies; };*/

        inline std::ostream& print_tallies(std::ostream& os) const {
                os << "  NetId    Cost       Tally (terminals count as " << terminalsCountAs() << " module(s)) " << std::endl;
                for (unsigned i = 0; i != _hg.getNumEdges(); i++) {
                        os << std::setw(6) << i << " : " << std::setw(6) << getNetCost(i) << " :  ";
                        os << " " << std::setw(4) << static_cast<unsigned>(_tallies[2 * i]) << " " << std::setw(4) << static_cast<unsigned>(_tallies[2 * i + 1]) << std::endl;
                }
                os << "   Total cost : " << getTotalCost() << std::endl << std::endl;

                return os;
        }

        const unsigned short* getTallies() const { return _tallies; }
        void updateAllCosts();

        inline unsigned computeCostOfOneNet(unsigned netIdx) const {
                unsigned short* pt = _tallies + 2 * netIdx;
                if ((*pt) == 0 || (*(pt + 1)) == 0)
                        return 0;
                else
                        return 1;
        }

        inline unsigned computeCostOfOneNetWW(unsigned netIdx) const {
                unsigned short* pt = _tallies + 2 * netIdx;
                if ((*pt) == 0 || (*(pt + 1)) == 0)
                        return 0;
                else
                        return _edgeWeights[netIdx];
        }

        inline double computeCostOfOneNetDouble(unsigned netIdx) const { return computeCostOfOneNet(netIdx); }

        inline void recomputeCostOfOneNet(unsigned netIdx) {
                unsigned newCost, oldCost = _netCosts[netIdx];
                _netCosts[netIdx] = newCost = computeCostOfOneNet(netIdx);
                _totalCost += (newCost - oldCost);
        }

        inline void recomputeCostOfOneNetWW(unsigned netIdx) {
                unsigned newCost, oldCost = _netCosts[netIdx];
                _netCosts[netIdx] = newCost = computeCostOfOneNetWW(netIdx);
                _totalCost += (newCost - oldCost);
        }
        // "get cost" means "get precomputed cost"

        unsigned getTotalCost() const {
                abkassert2(_totalCost != UINT_MAX, "Total cost uninitialized: ", "check if tables, e.g. net-vectors, have to be built\n");
                return _totalCost;
        }

        double getTotalCostDouble() const { return getTotalCost(); }
        unsigned getNetCost(unsigned netId) const { return _netCosts[netId]; }
        double getNetCostDouble(unsigned netId) const { return getNetCost(netId); }

        // Reinitializes the evaluator if the partition changed
        // "non-incrementally"
        // Must be redefined in derived classes by calling
        // same method in the immediate parent (with scopre resolution operator
        // ::)
        void reinitialize_tallies() {

                // begin code from old partEvalXFace::reinitialize()
                unsigned nodes = _hg.getNumNodes();
                if (nodes > _part->size()) {
                        uofm::stringstream text1;
                        text1 << "HGraph.nodes == " << nodes << ", ";
                        uofm::stringstream text2;
                        text2 << "Partitioning.size() == " << _part->size() << std::endl;
                        abkfatal3(0, "Size mismatch while reinit'ing PartEval:\n", text1.str().c_str(), text2.str().c_str());
                }
                // end code from old partEvalXFace::reinitialize()

                // begin code from old nettallies2way::reinitializeProper()
                std::fill(_tallies, _tallies + 2 * _hg.getNumEdges(), 0);

                itHGFNodeGlobal n = _hg.nodesBegin();
                for (; n != _hg.nodesEnd(); n++) {
                        unsigned nodeId = (*n)->getIndex();
                        PartitionIds partIds = (*_part)[nodeId];

                        itHGFEdgeLocal e = (*n)->edgesBegin();
                        if (partIds.isInPart(0)) {
                                if (_hg.isTerminal(nodeId))
                                        for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex()] += _terminalsCountAs;
                                else
                                        for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex()]++;
                        } else if (partIds.isInPart(1)) {
                                if (_hg.isTerminal(nodeId))
                                        for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex() + 1] += _terminalsCountAs;
                                else
                                        for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex() + 1]++;
                        } else
                                abkfatal(0, "module is not in any partition");
                }
                // end code from old nettallies2way::reinitializeProper()

                updateAllCosts();
        }

        void tallies_resetTo(const Partitioning& newPart) {
                _part = &newPart;
                reinitialize_tallies();
        }

        // These represent incremental update by one move (with cost updates),
        inline void moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to);

        void moveModuleTo(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) { moveModuleTo(moduleNumber, oldVal.lowestNumPart(), newVal.lowestNumPart()); }

        void moveWithReplication(unsigned moduleNumber, PartitionIds old, PartitionIds newIds);

        inline void moveModuleOnNet(unsigned netIdx, unsigned from, unsigned to) {
                unsigned short* pt = _tallies + 2 * netIdx;
                //  unsigned weight = _edgeWeights[netIdx];
                if (pt[to] == 0) {
                        _netCosts[netIdx] = 1;
                        _totalCost += 1;
                } else if (pt[from] == 1) {
                        _netCosts[netIdx] = 0;
                        _totalCost -= 1;
                }
                pt[to]++, pt[from]--;
        }

        inline void moveModuleOnNetWW(unsigned netIdx, unsigned from, unsigned to) {
                unsigned short* pt = _tallies + 2 * netIdx;
                unsigned weight = _edgeWeights[netIdx];
                if (pt[to] == 0) {
                        _netCosts[netIdx] = weight;
                        _totalCost += weight;
                } else if (pt[from] == 1) {
                        _netCosts[netIdx] = 0;
                        _totalCost -= weight;
                }
                pt[to]++, pt[from]--;
        }

        inline void updateNetForMovedModule(unsigned netIdx, unsigned from, unsigned)  // to
                                                                                       // don't check for trivial moves as they aren't likely
        {
                unsigned short* pt = _tallies + 2 * netIdx;
                if (from == 0) {
                        (*pt)--, (*(pt + 1))++;
                } else {
                        (*pt)++, (*(pt + 1))--;
                }
        }

        // these are overwhelmingly typical implementations. override if
        // necessary.
        unsigned getMinCostOfOneNet() const { return 0; }
        double getMinCostOfOneNetDouble() const { return getMinCostOfOneNet(); }

        // These highly depend on the evaluator and hypergraph
        unsigned getMaxCostOfOneNet() const { return 1; }
        unsigned getMaxCostOfOneNetWW() const { return _maxEdgeWeight; }
        double getMaxCostOfOneNetDouble() const { return getMaxCostOfOneNet(); }

        double getMaxCostOfOneNetDoubleWW() const { return getMaxCostOfOneNetWW(); }

        bool isNetCut() const { return true; }
        bool isNetCut2way() const { return true; }

        int getDeltaGainDueToNet(unsigned netIdx, unsigned movingFrom, unsigned /*movingTo*/, unsigned gainingFrom, unsigned /*gainingTo*/) const {
                unsigned short* tally = _tallies + 2 * netIdx;
                switch (tally[1 - movingFrom]) {
                        case 0:
                                return ((tally[movingFrom] == 2) ? 2 : 1);
                        case 1:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? 1 : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -2 : -1);
                        default:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? 1 : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -1 : 0);
                }
        }

        int getDeltaGainDueToNetWW(unsigned netIdx, unsigned movingFrom, unsigned /*movingTo*/, unsigned gainingFrom, unsigned /*gainingTo*/) const {
                unsigned short* tally = _tallies + 2 * netIdx;
                unsigned weight = _edgeWeights[netIdx];
                switch (tally[1 - movingFrom]) {
                        case 0:
                                return ((tally[movingFrom] == 2) ? 2 * weight : weight);
                        case 1:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? weight : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -2 * weight : -1 * weight);
                        default:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? weight : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -1 * weight : 0);
                }
        }

        bool netCostNotAffectedByMove(unsigned netIdx, unsigned from, unsigned to)
            // should be called before the move is applied
        {
                unsigned short* tally = _tallies + 2 * netIdx;
                return tally[from] >= 3 && tally[to] >= 2;
        }

        unsigned terminalsCountAs() const { return _terminalsCountAs; }

       private:  // this private section contains a member function used by the
                 // tallies
        void reinitializeProper();
};

inline void FMMMbaseNC2w::moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to)
    // no replication allowed
{
        const HGFNode& n = _hg.getNodeByIdx(moduleNumber);

        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                unsigned netIdx = (*e)->getIndex();
                updateNetForMovedModule(netIdx, from, to);
                if (_useWts) {
                        recomputeCostOfOneNetWW(netIdx);
                        moveModuleOnNetWW((*e)->getIndex(), from, to);
                } else {
                        recomputeCostOfOneNet(netIdx);
                        moveModuleOnNet((*e)->getIndex(), from, to);
                }
        }
}

inline void FMMMbaseNC2w::reinitializeProper() {}

inline void FMMMbaseNC2w::updateAllCosts()
    // derived classes need to call it in the end of reinitializeProper()
{
        _totalCost = 0;
        for (itHGFEdgeGlobal e = _hg.edgesBegin(); e != _hg.edgesEnd(); e++) {
                unsigned netIdx = (*e)->getIndex();
                if (_useWts)
                        _totalCost += _netCosts[netIdx] = computeCostOfOneNetWW(netIdx);
                else
                        _totalCost += _netCosts[netIdx] = computeCostOfOneNet(netIdx);
        }
}

#endif
