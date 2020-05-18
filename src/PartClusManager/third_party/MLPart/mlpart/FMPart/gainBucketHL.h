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

#ifndef __BASE_GAIN_BUCKET_HASHLIST_H__
#define __BASE_GAIN_BUCKET_HASHLIST_H__

#include "ABKCommon/abkcommon.h"
#include "ABKCommon/sgi_hash_map.h"
#include "svGainElmt.h"
#include "gainList.h"
#include <algorithm>
#include <set>

// I am going to be using a hash_map, so I will not inherit from vector
// however, in the code only operator[]() and .size() are implemented.

class BucketArrayHL  // : public vector<GainList>
    {
       protected:
        struct hash_elementT {
               public:
                std::set<int>::iterator weight_list_element_it;
                GainList gain_list;
        };

        std::set<int>::iterator _highestNonEmpty;
        std::set<int>::iterator _highestValid;
        SVGainElement* _lastMoveSuggested;
        int _gainOffset;
        int _maxBucketIdx;

        hash_map<int, hash_elementT, hash<int>, equal_to<int> > _bucketArray;
        std::set<int> _weightList;

       public:
        BucketArrayHL() : _highestNonEmpty(_weightList.end()), _highestValid(_weightList.end()), _lastMoveSuggested(NULL), _maxBucketIdx(-1) {}

        ~BucketArrayHL() {}

        //  virtual void setMaxGain(unsigned maxG)=0;

        //  virtual void push(SVGainElement& elmt)=0;

        // implement operator[]() and .size() to support the desired interface
        // also need {begin(), end(), rbegin(), rend()}x{const, nonconst}
        // added const operator[]() for the const printing function
        const GainList& operator[](int weight) const {
                hash_map<int, hash_elementT, hash<int>, equal_to<int> >::const_iterator array_iterator = _bucketArray.find(weight);
                if (array_iterator != _bucketArray.end())
                        return array_iterator->second.gain_list;
                else {
                        abkfatal(0,
                                 "Called a const function when it wasnt a "
                                 "const operation");
                }
        }

        GainList& operator[](int weight) {
                hash_map<int, hash_elementT, hash<int>, equal_to<int> >::iterator array_iterator = _bucketArray.find(weight);
                if (array_iterator != _bucketArray.end())
                        return array_iterator->second.gain_list;
                else {
                        // use insert always to save a lookup, insert will do
                        // nothing if the element is there
                        std::pair<std::set<int>::iterator, bool> insert_result = _weightList.insert(weight);
                        hash_elementT e;
                        e.weight_list_element_it = insert_result.first;
                        std::pair<hash_map<int, hash_elementT, hash<int>, equal_to<int> >::iterator, bool> ht_insert_result = _bucketArray.insert(std::make_pair(weight, e));
                        // return e.gain_list;
                        return ht_insert_result.first->second.gain_list;
                }
        }
        size_t size(void) { return _bucketArray.size(); }

        SVGainElement* getHighest();  // NULL => no more valid moves

        void resetAfterGainUpdate();  // must be called after the
        // gain updates are done
        void removeAll();  // remove all elements

        bool invalidateBucket();
        bool invalidateElement();

        int getHighestValidGain() {
                if (_highestValid == _weightList.end()) return -1;
                return *_highestValid;
        }

        void setupForClip();

        bool isConsistent();

        friend std::ostream& operator<<(std::ostream& os, const BucketArrayHL& buckets);

        void setMaxGain(unsigned maxG);
        void push(SVGainElement& elmt);

        class iterator : public std::set<int>::iterator {
               public:
                iterator(BucketArrayHL& list, const std::set<int>::iterator& it) : std::set<int>::iterator(it), _list(list) {}
                GainList& operator*(void) { return _list[std::set<int>::iterator::operator*()]; }
                int getWeight(void) const { return this->std::set<int>::iterator::operator*(); }
                // GainList* operator->(void) { return
                // &_list[std::set<int>::iterator::operator*()]; }

               private:
                BucketArrayHL& _list;
        };

        class reverse_iterator : public std::set<int>::reverse_iterator {
               public:
                reverse_iterator(BucketArrayHL& list, const std::set<int>::reverse_iterator& it) : std::set<int>::reverse_iterator(it), _list(list) {}
                GainList& operator*(void) { return _list[std::set<int>::reverse_iterator::operator*()]; }
                int getWeight(void) const { return this->std::set<int>::reverse_iterator::operator*(); }
                // GainList* operator->(void) { return
                // &_list[std::set<int>::reverse_iterator::operator*()]; }

               private:
                BucketArrayHL& _list;
        };

        class const_iterator : public std::set<int>::const_iterator {
               public:
                const_iterator(const BucketArrayHL& list, std::set<int>::const_iterator it) : std::set<int>::const_iterator(it), _list(list) {}
                const GainList& operator*(void) { return _list[std::set<int>::const_iterator::operator*()]; }
                int getWeight(void) const { return this->std::set<int>::const_iterator::operator*(); }
                // const GainList* operator->(void) const { return
                // &_list[std::set<int>::const_iterator::operator*()]; }

               private:
                const BucketArrayHL& _list;
        };

        class const_reverse_iterator : public std::set<int>::const_reverse_iterator {
               public:
                const_reverse_iterator(const BucketArrayHL& list, std::set<int>::const_reverse_iterator it) : std::set<int>::const_reverse_iterator(it), _list(list) {}
                const GainList& operator*(void) { return _list[std::set<int>::const_reverse_iterator::operator*()]; }
                int getWeight(void) const { return this->std::set<int>::const_reverse_iterator::operator*(); }
                // const GainList* operator->(void) const { return
                // &_list[std::set<int>::const_reverse_iterator::operator*()]; }

               private:
                const BucketArrayHL& _list;
        };

        // set<int>::iterator begin(void) { return _weightList.begin(); }
        // set<int>::iterator end(void) { return _weightList.end(); }

        iterator begin(void) { return iterator(*this, _weightList.begin()); }
        iterator end(void) { return iterator(*this, _weightList.end()); }
        reverse_iterator rbegin(void) { return reverse_iterator(*this, _weightList.rbegin()); }
        reverse_iterator rend(void) { return reverse_iterator(*this, _weightList.rend()); }
        const_iterator begin(void) const { return const_iterator(*this, _weightList.begin()); }
        const_iterator end(void) const { return const_iterator(*this, _weightList.end()); }
        const_reverse_iterator rbegin(void) const { return const_reverse_iterator(*this, _weightList.rbegin()); }
        const_reverse_iterator rend(void) const { return const_reverse_iterator(*this, _weightList.rend()); }
};

//____________________IMPLEMENTATIONS__________________________//
//
//________BucketArray functions________//
#endif
