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

// created by Andrew Caldwell and Igor Markov on 09/19/98

#ifndef __BASE_GAIN_BUCKET_M_H__
#define __BASE_GAIN_BUCKET_M_H__

#include "ABKCommon/abkcommon.h"
#include "svGainElmt.h"
#include "gainList.h"
#include <map>
#include <algorithm>

class BucketArrayM : public std::map<unsigned, GainList> {
       protected:
        iterator _highestNonEmpty;
        iterator _highestValid;
        SVGainElement* _lastMoveSuggested;
        int _gainOffset;
        int _maxBucketIdx;

       public:
        BucketArrayM() : std::map<unsigned, GainList>(), _highestNonEmpty(end()), _highestValid(end()), _lastMoveSuggested(NULL), _maxBucketIdx(-1) {}
        ~BucketArrayM() {}
        SVGainElement* getHighest();  // NUL => no more valid moves
        void resetAfterGainUpdate();  // must be called after the
        // gain updates are done
        void removeAll();  // remove all elements
        bool invalidateBucket();
        bool invalidateElement();
        int getHighestValidGain() {
                if (_highestValid == end())
                        return -1;
                else
                        return (_highestValid->first);
        }
        void setupForClip();
        bool isConsistent();
        friend std::ostream& operator<<(std::ostream& os, const BucketArrayM& buckets);
        void setMaxGain(unsigned maxG);
        void push(SVGainElement& elmt);
};

//____________________IMPLEMENTATIONS__________________________//

//________BucketArray functions________//

inline SVGainElement* BucketArrayM::getHighest() {  // assumes _highestValid
                                                    // indexes a bucket with
                                                    // some valid
        // move remaining or -1, and that lastMoveSuggested is either
        // an element in that bucket, or NULL if _highestValie == -1
        abkassert(_maxBucketIdx != -1, "must call setMaxGain before using the BucketArray");

        return _lastMoveSuggested;
}

inline void BucketArrayM::resetAfterGainUpdate() {
        // bring the _highestNonEmpty index down to the first non-empty
        // bucket (it may be too high as a result of incremental gain updates)
        // and clear any invalidations

        abkassert(_maxBucketIdx != -1, "must call setMaxGain before using the BucketArray");

        while (_highestNonEmpty != end() && _highestNonEmpty->second.isEmpty())
                if (_highestNonEmpty == begin())
                        _highestNonEmpty = end();
                else
                        --_highestNonEmpty;

        _highestValid = _highestNonEmpty;
        if (_highestValid != end())  // there is something left in this array
                _lastMoveSuggested = _highestValid->second._next;
        else
                _lastMoveSuggested = NULL;
}

inline bool BucketArrayM::invalidateBucket() {
        abkassert(_maxBucketIdx != -1, "must call setMaxGain before using the BucketArray");

        // invalidate _highestValid..
        if (_highestValid == begin())
                _highestValid = end();
        else
                --_highestValid;
        while (_highestValid != end() && _highestValid->second.isEmpty())
                if (_highestValid == begin())
                        _highestValid = end();
                else
                        --_highestValid;

        if (_highestValid != end())  // there is something left in this array
                _lastMoveSuggested = _highestValid->second._next;
        else
                _lastMoveSuggested = NULL;

        return true;
}

inline bool BucketArrayM::invalidateElement() {
        abkassert(_maxBucketIdx != -1, "must call setMaxGain before using the BucketArray");

        abkassert(_lastMoveSuggested != NULL, "can't invalidate a NULL move");
        _lastMoveSuggested = _lastMoveSuggested->_next;

        if (_lastMoveSuggested == NULL)  // last one in this bucket
                return invalidateBucket();
        else
                return false;
}

inline void BucketArrayM::removeAll() {
        for (iterator g = begin(); g != end(); ++g) {
                SVGainElement* elmt = g->second._next;
                while (elmt != NULL) {
                        elmt->disconnect();
                        elmt = g->second._next;
                }
        }

        _highestNonEmpty = begin();
        resetAfterGainUpdate();
}
inline bool BucketArrayM::isConsistent() {
        bool consistent = true;
        for (iterator g = begin(); g != end(); ++g)
                if (!g->second.isConsistent()) consistent = false;

        return consistent;
}

// NOTE: gain offset is the offset that gives use the bucket
// coresponding to gain ==0.  Note that this is a vector
// of size 2*_gainOffset+1.
inline void BucketArrayM::setupForClip() {  // unchanged to map
        int g;
        for (g = _gainOffset - 1; g >= 0; g--) (*this)[_gainOffset].insertBehind((*this)[g]);
        for (g = _gainOffset + 1; g < static_cast<int>(this->size()); g++) (*this)[_gainOffset].insertAhead((*this)[g]);

        SVGainElement* e;
        for (e = (*this)[_gainOffset]._next; e != NULL; e = e->_next) {
                e->_gainOffset = e->_gain;
                e->_gain = 0;
        }
}

inline std::ostream& operator<<(std::ostream& os, const BucketArrayM& bucket) {
        os << " BucketArray:" << std::endl;
        os << "  MaxGain:  " << bucket._gainOffset << std::endl;

        for (std::map<unsigned, GainList>::const_reverse_iterator g = bucket.rbegin(); g != bucket.rend(); --g) os << (g->first) << ") " << (g->second);

        os << std::endl;
        return os;
}

inline void BucketArrayM::setMaxGain(unsigned maxG) {
        _gainOffset = maxG;
        _maxBucketIdx = 2 * _gainOffset;
        // this->insert(end(), _maxBucketIdx+1-size(), GainList());
        // no longer need to fill with empty ones, map container takes care of
        // this now
        _highestNonEmpty = _highestValid = begin();
}

inline void BucketArrayM::push(SVGainElement& elmt) {
        abkassert(_maxBucketIdx != -1, "must call setMaxGain before using the BucketArray");

        const unsigned bucket = std::max(std::min(_maxBucketIdx, elmt._gain + _gainOffset), 0);
        (*this)[bucket].push(elmt);
        if (_highestNonEmpty == end()) {
                _highestNonEmpty = find(bucket);
        } else {
                const unsigned a = _highestNonEmpty->first;
                _highestNonEmpty = find(std::max(a, bucket));
        }
}

#endif
