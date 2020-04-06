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

// created by Andrew Caldwell 05/26/98

// CHANGES

// 980917 ilm   templated dcGainCo by GainBucketArray type
// 980918 ilm   added updateGainOffset methods

#ifndef __DEST_CENTRIC_GC_H__
#define __DEST_CENTRIC_GC_H__

#include "gainBucket.h"
#include "gainBucketM.h"
#include "gainBucketHL.h"
#include "nByKRepository.h"
#include "Partitioners/multiStartPart.h"
#include <iostream>

#define DestCentricGainContainer DestCentricGainContainerT

class DestCentricGainContainerT {
       protected:
        NbyKRepository _repository;
        uofm::vector<BucketArray> _prioritizer1;
        uofm::vector<BucketArrayM> _prioritizer2;
        uofm::vector<BucketArrayHL> _prioritizer3;

        // there will be k BucketArrays (this is a k-way destination-
        // centric GainCont)

        unsigned _highestArray;  // the array of buckets currently having
        // the highest valued gain in it.

        unsigned _partitionBias;  // the gainCo will tie-break towards
        // moves which are TO this partition

        int _maxGain;  // max possible gain a move can have
        const unsigned _numNodes;
        const unsigned _numParts;
        PartitionerParams _params;

       public:
        DestCentricGainContainerT(unsigned numNodes, unsigned numParts, const PartitionerParams& params, bool overrideToHashBased = false) : _repository(numNodes, numParts), _prioritizer1(numParts), _prioritizer2(numParts), _prioritizer3(numParts), _highestArray(0), _partitionBias(0), _maxGain(-1), _numNodes(numNodes), _numParts(numParts), _params(params) {
                abkfatal(!(_params.hashBasedGC && _params.noHashBasedGC), "Cannot use -hashBasedGC and -noHashBasedGC together");
                bool canUseHash = !_params.noHashBasedGC;
                bool saveMemory = _params.saveMem;
                if (canUseHash && saveMemory) {
                        _params.hashBasedGC = true;
                } else if (canUseHash && overrideToHashBased)
                        _params.hashBasedGC = true;

                if (_params.verb.getForActions() >= 2)
                        if (_params.hashBasedGC)
                                std::cout << "DestCentricGainContainerT using "
                                             "hash based gain container" << std::endl;
                        else if (_params.mapBasedGC)
                                std::cout << "DestCentricGainContainerT using "
                                             "map based gain container" << std::endl;
                        else
                                std::cout << "DestCentricGainContainerT using "
                                             "array based gain container" << std::endl;
        }

        void setMaxGain(unsigned maxG);

        void reinitialize();
        void setPartitionBias(unsigned part) { _partitionBias = part; }

        void updateGain(SVGainElement& elmt, int deltaGain);
        void updateGain(unsigned node, unsigned part, int deltaGain);
        void updateGain(SVGainElement& elmt, unsigned part, int deltaGain);

        void updateGainOffset(SVGainElement& elmt, int deltaGain);
        void updateGainOffset(unsigned node, unsigned part, int deltaGain);
        void updateGainOffset(SVGainElement& elmt, unsigned part, int deltaGain);

        void moveGainToOffset(SVGainElement& elmt);
        void moveGainToOffset(unsigned node, unsigned part);
        void moveGainToOffset(SVGainElement& elmt, unsigned part);

        void removeElement(SVGainElement& elmt) { elmt.disconnect(); }
        void removeElement(unsigned nodeId, unsigned part) { _repository.getElement(nodeId, part).disconnect(); }

        void removeAllNodesElements(unsigned nodeId) {
                for (unsigned k = 0; k < _numParts; k++) removeElement(getElement(nodeId, k));
        }

        int getMaxGain() const { return _maxGain; }
        unsigned getNumPrioritizers() const { return _prioritizer1.size(); }
        const BucketArrayHL& getPrioritizerHL(unsigned k) const { return _prioritizer3[k]; }
        const BucketArrayM& getPrioritizerM(unsigned k) const { return _prioritizer2[k]; }
        const BucketArray& getPrioritizer(unsigned k) const { return _prioritizer1[k]; }
        const NbyKRepository& getRepository() const { return _repository; }

        void removeAll();  // remove all the elements

        void addElement(SVGainElement& elmt, int gain);
        void addElement(SVGainElement& elmt, unsigned part, int gain);
        void addElement(unsigned nodeId, unsigned part, int gain) { addElement(_repository.getElement(nodeId, part), part, gain); }

        SVGainElement& getElement(unsigned nodeId, unsigned part) { return _repository.getElement(nodeId, part); }
        unsigned getElementId(SVGainElement& elmt) { return _repository.getElementId(elmt); }
        unsigned getElementPartition(SVGainElement& elmt) { return _repository.getElementPartition(elmt); }

        // I know these static casts are a 'Bad Thing' (tm), however,
        // the head of the lists MUST be derived from the base Element,
        // to allow for efficiently removing the item in lists of length
        // 1.  It can't be templated, as you can't derive from a template
        // parameter

        SVGainElement* getHighestGainElement();

        bool invalidateElement(SVGainElement& elmt);
        bool invalidateBucket(SVGainElement& elmt);

        // these are somewhat faster, if the partition is already known..
        bool invalidateElement(unsigned part);
        bool invalidateBucket(unsigned part);

        void resetAfterGainUpdate() {
                if (_params.mapBasedGC) {
                        for (unsigned k = 0; k < _prioritizer2.size(); k++) _prioritizer2[k].resetAfterGainUpdate();
                } else if (_params.hashBasedGC) {
                        for (unsigned k = 0; k < _prioritizer3.size(); k++) _prioritizer3[k].resetAfterGainUpdate();
                } else {
                        for (unsigned k = 0; k < _prioritizer1.size(); k++) _prioritizer1[k].resetAfterGainUpdate();
                }

                setHighestGainArray();
        }

        void setupClip();

        bool isConsistent();
        friend std::ostream& operator<<(std::ostream& os, const DestCentricGainContainerT& gc);

       private:
        void setHighestGainArray() {
                _highestArray = _partitionBias;
                if (_params.mapBasedGC) {
                        int highestGain = _prioritizer2[_highestArray].getHighestValidGain();
                        for (unsigned k = 0; k < _prioritizer2.size(); k++)
                                if (_prioritizer2[k].getHighestValidGain() > highestGain) {
                                        _highestArray = k;
                                        highestGain = _prioritizer2[k].getHighestValidGain();
                                }
                } else if (_params.hashBasedGC) {
                        int highestGain = _prioritizer3[_highestArray].getHighestValidGain();
                        for (unsigned k = 0; k < _prioritizer3.size(); k++)
                                if (_prioritizer3[k].getHighestValidGain() > highestGain) {
                                        _highestArray = k;
                                        highestGain = _prioritizer3[k].getHighestValidGain();
                                }
                } else {

                        int highestGain = _prioritizer1[_highestArray].getHighestValidGain();
                        for (unsigned k = 0; k < _prioritizer1.size(); k++)
                                if (_prioritizer1[k].getHighestValidGain() > highestGain) {
                                        _highestArray = k;
                                        highestGain = _prioritizer1[k].getHighestValidGain();
                                }
                }
        }
};

std::ostream& operator<<(std::ostream& os, const DestCentricGainContainerT& gc);

inline void DestCentricGainContainerT::updateGain(unsigned node, unsigned part, int deltaGain) { updateGain(_repository.getElement(node, part), part, deltaGain); }

inline void DestCentricGainContainerT::updateGain(SVGainElement& elmt, int deltaGain) {
        unsigned part = _repository.getElementPartition(elmt);
        updateGain(elmt, part, deltaGain);
}

inline void DestCentricGainContainerT::updateGain(SVGainElement& elmt, unsigned part, int deltaGain) {
        elmt.disconnect();
        elmt._gain += deltaGain;
        if (_params.mapBasedGC)
                _prioritizer2[part].push(elmt);
        else if (_params.hashBasedGC)
                _prioritizer3[part].push(elmt);
        else
                _prioritizer1[part].push(elmt);
        // std::cout << "*** gain co" << std::endl << (*this) << std::endl;
}

inline void DestCentricGainContainerT::updateGainOffset(unsigned node, unsigned part, int deltaGain) { updateGainOffset(_repository.getElement(node, part), part, deltaGain); }

inline void DestCentricGainContainerT::updateGainOffset(SVGainElement& elmt, int deltaGain) {
        unsigned part = _repository.getElementPartition(elmt);
        updateGainOffset(elmt, part, deltaGain);
}

inline void DestCentricGainContainerT::updateGainOffset(SVGainElement& elmt, unsigned part, int deltaGain) {
        elmt.disconnect();
        elmt._gainOffset += deltaGain;
        if (_params.mapBasedGC)
                _prioritizer2[part].push(elmt);
        else if (_params.hashBasedGC)
                _prioritizer3[part].push(elmt);
        else
                _prioritizer1[part].push(elmt);
}

inline void DestCentricGainContainerT::moveGainToOffset(unsigned node, unsigned part) { moveGainToOffset(_repository.getElement(node, part), part); }

inline void DestCentricGainContainerT::moveGainToOffset(SVGainElement& elmt) {
        unsigned part = _repository.getElementPartition(elmt);
        moveGainToOffset(elmt, part);
}

inline void DestCentricGainContainerT::moveGainToOffset(SVGainElement& elmt, unsigned part) {
        elmt.disconnect();
        elmt._gainOffset += elmt._gain;
        elmt._gain = 0;
        if (_params.mapBasedGC)
                _prioritizer2[part].push(elmt);
        else if (_params.hashBasedGC)
                _prioritizer3[part].push(elmt);
        else
                _prioritizer1[part].push(elmt);
}

inline void DestCentricGainContainerT::addElement(SVGainElement& elmt, int gain) {
#ifdef ABKDEBUG
        if (elmt._next != NULL || elmt._prev != NULL) {
                std::cout << "trying to add a node that is already in the lists: ";
                std::cout << _repository.getElementId(elmt) << std::endl;
                abkassert(0, "done");
        }
#endif

        elmt._gain = gain;
        if (_params.mapBasedGC)
                _prioritizer2[_repository.getElementPartition(elmt)].push(elmt);
        else if (_params.hashBasedGC)
                _prioritizer3[_repository.getElementPartition(elmt)].push(elmt);
        else
                _prioritizer1[_repository.getElementPartition(elmt)].push(elmt);
}

inline void DestCentricGainContainerT::addElement(SVGainElement& elmt, unsigned part, int gain) {
#ifdef ABKDEBUG
        if (elmt._next != NULL || elmt._prev != NULL) {
                std::cout << "trying to add a node that is already in the lists: ";
                std::cout << _repository.getElementId(elmt) << std::endl;
                abkassert(0, "done");
        }
#endif

        abkassert(part < _prioritizer1.size(), "invalid partition in addGE");

        elmt._gain = gain;
        if (_params.mapBasedGC)
                _prioritizer2[part].push(elmt);
        else if (_params.hashBasedGC)
                _prioritizer3[part].push(elmt);
        else
                _prioritizer1[part].push(elmt);
}

inline SVGainElement* DestCentricGainContainerT::getHighestGainElement() {
        if (_params.mapBasedGC)
                return static_cast<SVGainElement*>(_prioritizer2[_highestArray].getHighest());
        else if (_params.hashBasedGC)
                return static_cast<SVGainElement*>(_prioritizer3[_highestArray].getHighest());
        else
                return static_cast<SVGainElement*>(_prioritizer1[_highestArray].getHighest());
}

inline bool DestCentricGainContainerT::invalidateElement(SVGainElement& elmt) {
        bool maxChanged;
        if (_params.mapBasedGC)
                maxChanged = _prioritizer2[_repository.getElementPartition(elmt)].invalidateElement();
        else if (_params.hashBasedGC)
                maxChanged = _prioritizer3[_repository.getElementPartition(elmt)].invalidateElement();
        else
                maxChanged = _prioritizer1[_repository.getElementPartition(elmt)].invalidateElement();
        if (maxChanged) setHighestGainArray();
        return maxChanged;
}

inline bool DestCentricGainContainerT::invalidateBucket(SVGainElement& elmt) {
        if (_params.mapBasedGC)
                _prioritizer2[_repository.getElementPartition(elmt)].invalidateBucket();
        else if (_params.hashBasedGC)
                _prioritizer3[_repository.getElementPartition(elmt)].invalidateBucket();
        else
                _prioritizer1[_repository.getElementPartition(elmt)].invalidateBucket();
        setHighestGainArray();
        return true;
}

// these are somewhat faster, if the partition is already known..
inline bool DestCentricGainContainerT::invalidateElement(unsigned part) {
        bool maxChanged;
        if (_params.mapBasedGC)
                maxChanged = _prioritizer2[part].invalidateElement();
        else if (_params.hashBasedGC)
                maxChanged = _prioritizer3[part].invalidateElement();
        else
                maxChanged = _prioritizer1[part].invalidateElement();
        if (maxChanged) setHighestGainArray();
        return maxChanged;
}

inline bool DestCentricGainContainerT::invalidateBucket(unsigned part) {
        if (_params.mapBasedGC)
                _prioritizer2[part].invalidateBucket();
        else if (_params.hashBasedGC)
                _prioritizer3[part].invalidateBucket();
        else
                _prioritizer1[part].invalidateBucket();
        setHighestGainArray();
        return true;
}

#endif
