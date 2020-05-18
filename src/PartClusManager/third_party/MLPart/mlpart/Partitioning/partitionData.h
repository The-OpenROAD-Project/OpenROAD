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

//! author=" Andy Caldwell on 02/10/98"

//! CHANGES="partitionData.h 980420 aec  added &= |= to PartitionIds 980426 ilm
// added classes PartitioningBuffer and PartitioningDoubleBuffer 980506 ilm
// added firstSolnUsed and lastSolnUsed to the above two 980514 aec  moved
// PartitionIds to Combinatorics"

/* CHANGES
980420 aec  added &= |= to PartitionIds
980426 ilm  added classes PartitioningBuffer and PartitioningDoubleBuffer
980506 ilm  added firstSolnUsed and lastSolnUsed to the above two
980514 aec  moved PartitionIds to Combinatorics
*/

#include <ABKCommon/uofm_alloc.h>
#include <HGraph/hgFixed.h>
#include <Combi/partitionIds.h>
#include <Partitioning/relaxPart.h>

#ifndef __PARTITION_DATA_H__
#define __PARTITION_DATA_H__

#ifdef _MSC_VER
#define rint(a) floor((a) + 0.5)
#endif

class PartitioningSolution;
class PartitioningBuffer;
class PartitioningDoubleBuffer;

//: Get the partitioning solution parameters of a given partitioning.
//  Comment from the code: the point of not deriving this from Partitioning
//	is we don't want to update these values all the time (I don't think),
//	but rather, once you've settled on a partitioning, you want to calculate
//	them all, and store then with the partitioning.
//	see main7.cxx for "modus operandi".
class PartitioningSolution {
       public:
        Partitioning& part;
        unsigned numPartitions;
        // Total number of the clusters in current partitioning
        double totalArea;
        // total non-terminal area
        uofm::vector<unsigned> partCount;
        // non-terminals in each partition
        uofm::vector<uofm::vector<double> > partArea;
        // total area of non-terminals in each partition
        uofm::vector<unsigned> partPins;
        // total number of pins in each partition

        unsigned cost;
        // Cost may be cut, absorbtion, WL of various types, etc..
        double imbalance;
        // By now, imbalance is set as Double-Maximum
        double violation;
        // By now, violation is set as Double-Maximum
        unsigned pinImbalance;

        PartitioningSolution(const Partitioning& srcPart, unsigned nParts, double tArea, const uofm::vector<unsigned>& pCount, const uofm::vector<uofm::vector<double> >& pArea, unsigned newCost = UINT_MAX) : part(const_cast<Partitioning&>(srcPart)), numPartitions(nParts), totalArea(tArea), partCount(pCount), partArea(pArea), partPins(pArea.size(), UINT_MAX), cost(newCost), imbalance(DBL_MAX), violation(DBL_MAX), pinImbalance(0) {}

        PartitioningSolution(const Partitioning& srcPart, unsigned nParts, double tArea, const uofm::vector<unsigned>& pCount, const uofm::vector<uofm::vector<double> >& pArea, const uofm::vector<unsigned>& pPins, unsigned newCost = UINT_MAX) : part(const_cast<Partitioning&>(srcPart)), numPartitions(nParts), totalArea(tArea), partCount(pCount), partArea(pArea), partPins(pPins), cost(newCost), imbalance(DBL_MAX), violation(DBL_MAX), pinImbalance(0) {
                unsigned minPins = UINT_MAX;
                unsigned maxPins = 0;
                for (unsigned k = 0; k < partPins.size(); k++) {
                        maxPins = std::max(maxPins, partPins[k]);
                        minPins = std::min(minPins, partPins[k]);
                }
                pinImbalance = maxPins - minPins;
        }

        PartitioningSolution(const Partitioning& srcPart, const HGraphFixed& hgraph, unsigned nParts, unsigned cost);
        // Constructor to calculate the total area of non-terminals
        // in each partition, non-terminals in each partition and
        // total non-terminal area.  To judge the validity of
        // current partitioning by checking whether
        // the partitioning number of a given node is less than total
        // number of partitionings and whether the sum area of partitions
        // equals to the total area.

        void calculatePartData(const HGraphFixed& hgraph);
};

//: Vector of Partitioning objects with size "solutions",
//  and each Partitioning object is a vector of "modules" PartitionIds,
//	Used to store variety of partitioning solutions.
class PartitioningBuffer : public uofm::vector<Partitioning> {
        unsigned _modulesUsed;
        unsigned _firstUsedSoln;
        unsigned _lastUsedSoln;

       public:
        double maxMbsUsed;
        // Maximum Mbits to be allowed to use. By default, it is less than 30Mbs
       public:
        // Constructor to initial PartitionBuffer with default parameters: one
        //	solution and 30 Mbs memeory.
        // Set the partitionID of each module of all partitioning results
        // to be 32,  that is, belong to all partitions.

        PartitioningBuffer(unsigned modules, unsigned solutions = 1, double maxMbs = 30.0) : uofm::vector<Partitioning>(solutions, Partitioning(modules, PartitionIds())), _modulesUsed(modules), _firstUsedSoln(0), _lastUsedSoln(solutions), maxMbsUsed(maxMbs) {
                abkfatal(solutions, " Can't create empty partition buffer");
                abkfatal(modules * solutions < maxMbs * 1024 * 1024, " Memory cap for partitioning buffers reached");

                PartitionIds allBits;
                allBits.setToAll(32);

                for (unsigned p = 0; p < size(); p++) {
                        for (int x = modules - 1; x >= 0; --x) {
                                (*this)[p][x] = allBits;
                        }
                }
        }

        unsigned getNumModulesUsed() const { return _modulesUsed; }
        // get the number of modules used in the partitioning
        unsigned beginUsedSoln() const { return _firstUsedSoln; }
        // get the first partitioning solution which is used
        unsigned endUsedSoln() const { return _lastUsedSoln; }
        // get the last partitioning solution which is used
        void setNumModulesUsed(unsigned modUsed);
        // abkfatal  if arg too big
        void setBeginUsedSoln(unsigned solnsUsed);
        // abkfatal  if arg too big
        void setEndUsedSoln(unsigned solnsUsed);
        // add solution into the vector if arg too big
};

//: Includes two PartitioningBuffer objects.
//	one of which is \_buf1(\_mainBuf), the other of which is
//\_buf2(\_shadowBuf)
//	Be sure to make the two buffer identified all the time.
class PartitioningDoubleBuffer {
        PartitioningBuffer _buf1, _buf2;
        PartitioningBuffer* _mainBuf, *_shadowBuf;
        bool swapped, checkedOut;

        const char* _checkedOutInFile;
        unsigned _checkedOutInLine;

       public:
        double maxMbsUsed;
        //	Maximum Mbits allowed to be used. Default value is 60 Mbs

       public:
        PartitioningDoubleBuffer(unsigned modules, unsigned solutions = 1, double maxMbs = 60.0)
            : _buf1(modules, solutions),
              _buf2(modules, solutions),
              _mainBuf(&_buf1),
              _shadowBuf(&_buf2),
              swapped(false),
              checkedOut(false),
              _checkedOutInFile(NULL),
              _checkedOutInLine(UINT_MAX),
              maxMbsUsed(maxMbs)
              //  Constructor: initial the two partitioningBuffers with default
              //	MaxMbs=30.0. Boolean variable swapped and chechedOut are
              // both
              //  false
        {
                abkfatal(2 * solutions * modules < maxMbs * 1024 * 1024, "Memory cap for Partitioning Double Buffer reached");
        }

        PartitioningDoubleBuffer(const PartitioningBuffer& origBuff) : _buf1(origBuff), _buf2(origBuff), _mainBuf(&_buf1), _shadowBuf(&_buf2), swapped(false), checkedOut(false), _checkedOutInFile(NULL), _checkedOutInLine(UINT_MAX), maxMbsUsed(origBuff.maxMbsUsed) {}
        // Constructed from an existed PartitioingBuffer

        ~PartitioningDoubleBuffer();

        void setNumModulesUsed(unsigned modUsed);
        // same as the member function of class PartitioningBuffer except that
        // it is
        // for two buffers
        void setBeginUsedSoln(unsigned firstUsedSoln);
        // same as the member function of class PartitioningBuffer except that
        // it is
        // for two buffers
        void setEndUsedSoln(unsigned lastUsedSoln);
        // same as the member function of class PartitioningBuffer except that
        // it is
        // for two buffers

        unsigned getNumModulesUsed() const;
        // get the number of modules used in the partitioning Buffer 1
        unsigned beginUsedSoln() const;
        // get the first partitioning solution of Buffer which is used
        unsigned endUsedSoln() const;
        // get the last partitioning solution of Buffer which is used

        void swapBuf();
        // Assigns the contents of buff1 to buff2 and the contents of buff2 to
        // buff1.
        bool isSwapped() const { return swapped; }
        bool isCheckedOut() const { return checkedOut; }

        void checkout_(PartitioningBuffer*& mainBuf, PartitioningBuffer*& shadowBuf, const char* checkedOutInFile, unsigned checkedOutInLine);
        // chech out whether the two Partitioning Buffers is indetified.
        // to be used only via macro checkOutBuf(mainBuf,shadowBuf)

        void checkInBuf(const PartitioningBuffer* mainBuf, const PartitioningBuffer* shadowBuf);
        // checkOutBuff allows user code to "borrow"
        //  buffers from the double buffer and, possibly, change them.
        //  The checkInBuf returns the buffers and runs basic
        //  consistency checks. CheckIn/CheckOut is used as a locking
        //  mechanism --- buffers can't be checked out if they are
        //  already checked out, or checked in if they are already in.
        //  Also, a warning may be given if a double buffer is
        // destructed (e.g. when it goes out of scope) and its buffers
        // are checked out.
        // The main trick with double buffers is the order in which
        //  buffers are presented. You can check in two buffers, then
        // call the swap method, then check them out and they will
        // be in a difft. order. This is used during unclustering in
        //  multilevel partitioing.
};

#define checkOutBuf(mainBuf, shadowBuf) checkout_(mainBuf, shadowBuf, SgnPartOfFileName(__FILE__), __LINE__)

#endif
