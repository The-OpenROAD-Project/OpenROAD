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

//! author="Andrew Caldwell on 03/16/98"

//! CONTACTS="Andy Igor ABK"

//! CHANGES="partProb.h 980320  ilm  reworked file I/O methods and ctors ilm
// added initToNULL() 980321  ilm  made #include's leaner 980325 ilm independent
// fix and blk files, AUX file I/O, stream I/O 980407  aec  added minCapacities
// 980520  ilm  added getXTics(), getYTics(), getNumXParts(), getNumYParts()"

/* CHANGES
980320 ilm  reworked file I/O methods and ctors ilm  added initToNULL()
980321  ilm  made #include's leaner
980325  ilm  independent fix and blk files, AUX file I/O, stream I/O
980407  aec  added minCapacities
980520  ilm  added getXTics(), getYTics(), getNumXParts(), getNumYParts()
*/

#ifndef __PART_PROB_H__
#define __PART_PROB_H__

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <HGraph/hgFixed.h>
#include <Placement/placement.h>
#include <Partitioning/partitionData.h>
#include <iostream>
#include <algorithm>

class Partitioning;

//:  Enscapsulates data necessary to specify most common partitioning
// formulations
//   and their solutions, including the hgraph, partition sizes,
//   locations, capacities, balance tolerances, etc.
class PartitioningProblem {

       public:
        class Parameters : public HGraphParameters
                           //: Derived form base class HGraphBaseParams.
                           // Used to read the parameter: verbosity, from commandline
                           {
               public:
                Verbosity verbosity;

               public:
                Parameters(Verbosity VERB) : verbosity(VERB) {}
                Parameters() : verbosity("1_1_1") {}
                Parameters(int argc, const char** argv) : HGraphParameters(argc, argv), verbosity(argc, argv) {}
        };

        struct Attributes
            //: Used to indicate if this PartProblem has a particular
            //  data item populated. The following are not included, as
            //  they are ALWAYS populated:  hgraph, solnBuffers,
            // fixedConstraints
            {
                unsigned char hasMaxCap : 1;
                unsigned char hasMinCap : 1;
                unsigned char hasCap : 1;
                unsigned char hasPartBBoxes : 1;
                unsigned char hasPadBBoxes : 1;
                unsigned char hasPartCenters : 1;
                unsigned char hasPadCenters : 1;
                unsigned char hasTotalWeight : 1;
                unsigned char hasClusterDegrees : 1;

                Attributes() : hasMaxCap(false), hasMinCap(false), hasCap(false), hasPartBBoxes(false), hasPadBBoxes(false), hasPartCenters(false), hasPadCenters(false), hasTotalWeight(false), hasClusterDegrees(false) {}

                void clearAll() {
                        hasMaxCap = false;
                        hasMinCap = false;
                        hasCap = false;
                        hasPartBBoxes = false;
                        hasPadBBoxes = false;
                        hasPartCenters = false;
                        hasPadCenters = false;
                        hasTotalWeight = false;
                        hasClusterDegrees = false;
                }

                void setAll() {
                        hasMaxCap = true;
                        hasMinCap = true;
                        hasCap = true;
                        hasPartBBoxes = true;
                        hasPadBBoxes = true;
                        hasPartCenters = true;
                        hasPadCenters = true;
                        hasTotalWeight = true;
                        hasClusterDegrees = true;
                }
        };

       protected:
        Parameters _params;

        Attributes _attributes;

        bool _ownsData;
        // if ownsData is true, then the object's constructor created each of
        // the items
        //	below, and it will need to delete then in the destructor. If it
        // is false,
        //	they were passed in to the constructor, and should not be
        // deleted.
        //	these can't be const, as they may be created in the body of one
        //	of the constructor.

        //    RandomRawUnsigned		_ru;

        /** Not being empty, these vectors imply that
            partitions come from a grid, so that there are
            _xTics.size()-1 times _yTics.size()-1 partitions
        */
        uofm::vector<double> _xTics, _yTics;

        HGraphFixed* _hgraph;

        //  USparseMatrix*		_clusterDegrees;
        // for a given Hedge, e, and an Hnode, n, on that edge,
        // clusterDegrees[e][n] == the number of clusters contained in and
        //	which are attached to n.

        PartitioningBuffer* _solnBuffers;
        // solnBuffers both store initial solutions, and the part'ers results
        unsigned _bestSolnNum;
        uofm::vector<double> _costs;
        uofm::vector<double> _violations;
        uofm::vector<double> _imbalances;
        // This should be a vector of size #node
        // every element should have bits set in partitions
        // the node is allowed to move to (see how it is populated
        // when PartProb is ctructed from file --- partProbIO.cxx
        Partitioning* _fixedConstr;

        uofm::vector<BBox>* _partitions;
        //	information about the regular (ie. non-termila) partitions
        //	these three vectors should all have length k, for k-way
        // partitioning.
        //  they are indexed by the partition number.
        // size/location of each part.
        uofm::vector<uofm::vector<double> >* _capacities;
        // for each partition, a vector of capacity targets for each partition
        uofm::vector<uofm::vector<double> >* _maxCapacities;
        // for each parition, the max allowed value for each capacity
        uofm::vector<uofm::vector<double> >* _maxCapacitiesTemp;
        // for each parition, the max allowed value for each capacity

        uofm::vector<uofm::vector<double> >* _minCapacities;
        // for each partition/weight,the min allowed value
        uofm::vector<uofm::vector<double> >* _minCapacitiesTemp;
        // for each partition/weight,the min allowed value

        uofm::vector<double>* _totalWeight;
        // non-terminal nodes only.
        uofm::vector<Point>* _partitionCenters;
        // geometric center of each partition

        uofm::vector<unsigned>* _terminalToBlock;
        // information about the pad blocks. A pad block is a bbox which
        // contains
        //	a terminal.  Note that they may have area 0 (ie XMin=XMax).
        //	for each terminal in hgraph, terminalToBlock gives the index
        // into
        //	padBlocks for the BBox that terminal is in.
        uofm::vector<BBox>* _padBlocks;
        uofm::vector<Point>* _padBlockCenters;
        // geometric center of each pad block

        void _copyDynamicData(const PartitioningProblem& prob);
        void _freeDynamicData();

        void _setMaxCapacities(const uofm::vector<double>& tolerance, const uofm::vector<uofm::vector<double> >& capacities);
        void _setMinCapacities(const uofm::vector<double>& tolerance, const uofm::vector<uofm::vector<double> >& capacities);
        void _setPartitionCenters();
        void _setPadBlockCenters();

        void _setTotalWeight();
        //  void _setTrivUSparseMatrix();

        void _computeXYTics(const uofm::vector<BBox>&);
        // this will fail if partition bboxes are bloated ("fuzzy")
        void _postProcess();
        // sets the attributes + populates data, all ctors should call this last

        bool read(const char*);
        // read the data files

        bool readHG(const char* f1, const char* f2 = NULL, const char* f3 = NULL);
        bool read(const char* blkFileName, const char* fixFileName, const char* solFileName = NULL);

        bool readBLK(std::istream&);
        bool readFIX(std::istream&);
        bool readSOL(std::istream&);

        void initToNULL() {
                //     _hgraph=NULL, _clusterDegrees=NULL, _solnBuffers=NULL;
                _hgraph = NULL, _solnBuffers = NULL;
                _costs.clear(), _violations.clear(), _imbalances.clear(), _fixedConstr = NULL, _partitions = NULL, _capacities = NULL;
                _maxCapacities = NULL, _maxCapacitiesTemp = NULL, _totalWeight = NULL;
                _partitionCenters = NULL;
                _terminalToBlock = NULL, _padBlocks = NULL, _padBlockCenters = NULL;
                _bestSolnNum = UINT_MAX, _minCapacities = NULL, _minCapacitiesTemp = NULL;
        }

        bool save(const char* blkFileName, const char* fixFileName, const char* solFileName, bool saveAsNetD) const;

       public:
        PartitioningProblem(const HGraphFixed& hgraph, const PartitioningBuffer& solnBuffers, const Partitioning& fixedConstr,  // size=#nodes
                            const uofm::vector<BBox>& partitions, const uofm::vector<uofm::vector<double> >& capacities,
                            //	capacities for each partition & weight in absolute units
                            const uofm::vector<double>& tolerances,
                            // tolerance for each weight, as a fraction of area (ie..0.1 == 10%)
                            const uofm::vector<unsigned>& terminalToBlock, const uofm::vector<BBox>& padBlocks, Parameters parameters = Parameters());
        PartitioningProblem(const char* netDFileName,
                            /// either .are or .areM
                            const char* areFileName, const char* blkFileName, const char* fixFileName, const char* solFileName = NULL, Parameters parameters = Parameters());
        // read the separated data files to get the partitioning problem

        PartitioningProblem(const PartitioningProblem& prob);
        PartitioningProblem(const char* fileNameRoot, Parameters parameters = Parameters());
        // read the basefile to get the partitioinging problem

        virtual ~PartitioningProblem() { _freeDynamicData(); }
        void propagateTerminals(double fuzziness = 0.0);
        // fuzziness can be 10.0 (%). ?? function of propagateTerminals

        bool saveAsNetDAre(const char* baseFileName) const;
        bool saveAsNodesNets(const char* baseFileName) const;

        bool saveBestSol(const char* solFileName) const;
        void reserveBuffers(unsigned num);
        // makes sure there are at least num buffers. abkfatals if the data is
        // not owned.

        bool isDataOwned() const { return _ownsData; }

        bool partsComeFromGrid() const { return _xTics.size() > 1 || _yTics.size() > 1; }

        unsigned getNumXParts() const {
                abkfatal2(partsComeFromGrid(), "Partition bboxes ", " do not come from a grid");
                return _xTics.size() - 1;
        }

        unsigned getNumYParts() const {
                abkfatal2(partsComeFromGrid(), "Partition bboxes ", " do not come from a grid");
                return _yTics.size() - 1;
        }

        const uofm::vector<double>& getXTics() const { return _xTics; }
        const uofm::vector<double>& getYTics() const { return _yTics; }

        const HGraphFixed& getHGraph() const { return *_hgraph; }

        // NOTE: This is a workaround for g++ debug libs on Linux.
        // it has problems w/ the above function call...but using
        // the pointer directly seems to work fine!

        const HGraphFixed* getHGraphPointer() const { return _hgraph; }

        const Parameters& getParameters() const { return _params; }
        Parameters& getParameters() { return _params; }

        PartitioningBuffer& getSolnBuffers() { return *_solnBuffers; }
        const PartitioningBuffer& getSolnBuffers() const { return *_solnBuffers; }
        const Partitioning& getBestSoln() const { return (*_solnBuffers)[getBestSolnNum()]; }
        void setSolnBuffers(PartitioningBuffer* newbufs);
        // LOOK: deletes the old ones !!! will own the new ones !!! -ILM

        // same as setSolnBuffers except does *not* delete old ones.
        // return value is old buffers.
        PartitioningBuffer* swapOutSolnBuffers(PartitioningBuffer* newbufs);
        Partitioning* swapOutFixedConst(Partitioning* fixedConstr) {
                std::swap(_fixedConstr, fixedConstr);
                return fixedConstr;
        }
        HGraphFixed* swapOutHGraph(HGraphFixed* hgraph) {
                std::swap(_hgraph, hgraph);
                return hgraph;
        }

        const uofm::vector<double>& getCosts() const { return _costs; }
        const uofm::vector<double>& getViolations() const { return _violations; }
        const uofm::vector<double>& getImbalances() const { return _imbalances; }
        const Partitioning& getFixedConstr() const { return *_fixedConstr; }
        const uofm::vector<uofm::vector<double> >& getCapacities() const { return *_capacities; }
        const uofm::vector<uofm::vector<double> >& getMaxCapacities() const { return *_maxCapacities; }

        void temporarilyIncreaseTolerance(const uofm::vector<double>& byHowMuch, const uofm::vector<double>& currPartAreas);

        // called to restore the tolerance to what it was before the most recent
        // call to temporarilyIncreaseTolerance()
        void revertTolerance(void);

        const uofm::vector<uofm::vector<double> >& getMinCapacities() const { return *_minCapacities; }
        const uofm::vector<double>& getTotalWeight() const { return *_totalWeight; }
        const uofm::vector<BBox>& getPartitions() const { return *_partitions; }
        const uofm::vector<Point>& getPartitionCenters() const { return *_partitionCenters; }
        double getScalingFactorForBBoxQuantization(unsigned quantizationBound) const;
        const uofm::vector<BBox>& getPadBlocks() const { return *_padBlocks; }
        const uofm::vector<Point>& getPadBlockCenters() const { return *_padBlockCenters; }
        const uofm::vector<unsigned>& getTerminalToBlock() const { return *_terminalToBlock; }
        unsigned getNumPartitions() const { return _partitions->size(); }
        unsigned getBestSolnNum() const {
                abkfatal2(_bestSolnNum < _solnBuffers->size(), " Best Soln Num not set: ", _solnBuffers->size());
                return _bestSolnNum;
        }
        void setBestSolnNum(unsigned num) { _bestSolnNum = num; }
        void printLargestCellStats(unsigned N = 10) const;
        uofm::vector<unsigned> getPinBalances() const;
        void printPinBalances() const;

        const Attributes& getAttributes() const { return _attributes; }

        PartitioningProblem& operator=(const PartitioningProblem& prob);

        friend std::ostream& operator<<(std::ostream& os, const PartitioningProblem& prob);

        friend std::ostream& operator>>(std::ostream& os, const PartitioningProblem& prob);
        // note: the following reads a partitioning problem from the stream
        //	format operator << prints it it.  This is not the same as
        //	the .netD, .are, etc files the fileName ctor uses.

       protected:
        PartitioningProblem() : _ownsData(false), _maxCapacities(NULL), _maxCapacitiesTemp(NULL), _minCapacities(NULL), _minCapacitiesTemp(NULL), _totalWeight(NULL), _partitionCenters(NULL), _padBlockCenters(NULL) {}
        // for use by derived class ctors
};

#endif
