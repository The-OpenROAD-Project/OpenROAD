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

//! author=" Andrew Caldwell on 05/14/98 "
//! CHANGES =" partitionIds.h 980514 aec taken from Partitioning/partitionData.h
// 980605 ilm added random ctor for Partitioning"
//! CONTACTS =" Andy Igor ABK"

/*
CHANGES
 980514 aec taken from Partitioning/partitionData.h
 980605 ilm added random ctor for Partitioning
*/

#ifndef __PARTITION_IDS_H__
#define __PARTITION_IDS_H__

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>

#ifdef _MSC_VER
#define rint(a) floor((a) + 0.5)
#pragma warning(disable : 4800)
#endif

class PartitionIds;

//: Used as a bit-vector in which one bit stands for one partitions.
class PartitionIds {
        unsigned _partitions;
        // a 32-bit bit-vector of partitions i.e. one bit stands for one
        // partition.

       public:
        PartitionIds() : _partitions(0) {}
        PartitionIds(const PartitionIds& p2) { _partitions = p2._partitions; }

        unsigned totalBits() const { return 8 * sizeof(_partitions); }

        PartitionIds& operator=(const PartitionIds& part) {
                _partitions = part._partitions;
                return *this;
        }

        bool operator==(const PartitionIds& p1) const { return p1._partitions == _partitions; }
        bool operator!=(const PartitionIds& p1) const { return p1._partitions != _partitions; }

        PartitionIds& operator&=(const PartitionIds& p1) {
                _partitions &= p1._partitions;
                return *this;
        }
        PartitionIds& operator|=(const PartitionIds& p1) {
                _partitions |= p1._partitions;
                return *this;
        }

        inline void setToPart(unsigned part);
        // make it be in ONLY Partition part
        inline void addToPart(unsigned part);
        // make it be ALSO in Partition part
        inline void removeFromPart(unsigned part);
        // remove the cell from partition "part"
        inline void setToAll(unsigned k);
        // set to all partitions 0 to k-1
        inline bool isInPart(unsigned part) const;
        // tell whether the cell is in partition "part"
        // note: it may be in others, as well.
        inline bool isEmpty() const { return _partitions == 0; }

        bool operator[](unsigned part) const { return isInPart(part); }

        bool intersectsWith(const PartitionIds& pids) const { return (_partitions & pids._partitions); }

        inline unsigned lowestNumPart() const;
        // return the lowest number of partitions which the cell belongs to.

        inline unsigned numberPartitions() const;
        // how many partitions are marked true
        inline unsigned numberPartitions(unsigned nParts) const;
        // return the number of parptitions the cell
        // belongs to whose number is less than k

        unsigned getUnsigned() const { return _partitions; }
        void loadBitsFrom(unsigned bits) { _partitions = bits; }

        friend std::ostream& operator<<(std::ostream& os, const PartitionIds& part);
        // not sure what we'd output..but here to be complete
};

std::ostream& operator<<(std::ostream& os, const PartitionIds& part);

//: A vector to save the partition status of all cells
class Partitioning : public uofm::vector<PartitionIds> {
       public:
        Partitioning() {}
        Partitioning(unsigned size) : uofm::vector<PartitionIds>(size) {}
        Partitioning(unsigned size, PartitionIds p) : uofm::vector<PartitionIds>(size, p) {}
        ~Partitioning() {}
        friend std::ostream& operator<<(std::ostream& os, const Partitioning& part);
        void randomize(unsigned nParts);
        // return the random partitioning result of nParts-way partitioning
};

std::ostream& operator<<(std::ostream& os, const Partitioning& part);

/*___________________PartitionIds inline methods __________________ */

inline void PartitionIds::setToPart(unsigned part) {
#ifdef PARTDEBUG
        abkfatal(part < 32, "partitions can not exceed 32");
#endif
        _partitions = (1 << part);
}

inline void PartitionIds::removeFromPart(unsigned part) {
#ifdef PARTDEBUG
        abkfatal(part < 32, "partitions can not exceed 32");
#endif
        _partitions &= ~(1 << part);
}

inline void PartitionIds::setToAll(unsigned k) {
#ifdef PARTDEBUG
        abkfatal(k <= 32, "partitions can not exceed 32");
#endif

        if (k == 32)
                // this should not be necessary; next branch
                _partitions = 0xffffffff;
        // should work always.  But at least on MSVC++
        // it returns that you're in *no* partitions
        // (instead of *every*)partition.  Reason
        // is unknown.  mro
        else
                _partitions = ~(0xffffffff << k);
}

inline void PartitionIds::addToPart(unsigned part) {
#ifdef PARTDEBUG
        abkfatal(part < 32, "partitions can not exceed 32");
#endif

        _partitions |= (1 << part);
}

inline bool PartitionIds::isInPart(unsigned part) const {
#ifdef PARTDEBUG
        abkfatal(part < 32, "partitions can not exceed 32");
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif

        return _partitions & (1 << part);

#ifdef _MSC_VER
#pragma warning(default : 4800)
#endif
}

inline unsigned PartitionIds::lowestNumPart() const {
        // return the lowest numbered patition marked 'true'
        // return of 0-31 == lowest numbered partition

        if (_partitions & 0x1) return 0;
        if (_partitions & 0x2) return 1;
        if (!_partitions) return 32;
        unsigned bitVec = _partitions >> 2;
        for (unsigned k = 2;; k++, bitVec >>= 1) {
                if (bitVec & 0x1) return k;
        }
}

inline unsigned PartitionIds::numberPartitions() const {  // how many 1's are there in the partitionIds

        unsigned count = 0;
        unsigned bitVec = _partitions;

        if (bitVec) do {
                        count++;
                } while (bitVec &= bitVec - 1);

        return count;
}

inline unsigned PartitionIds::numberPartitions(unsigned nParts) const {
        // how many partitions are marked true
        // anyone got a better name for this?
        unsigned count = 0;
        for (int k = nParts - 1; k >= 0; k--)
                if (_partitions & (1 << k)) count++;
        return count;
}

#ifdef _MSC_VER
#pragma warning(default : 4800)
#endif

#endif
