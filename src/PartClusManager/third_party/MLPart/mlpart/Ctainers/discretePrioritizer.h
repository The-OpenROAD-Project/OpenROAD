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

//////////////////////////////////////////////////////////////////////
//
// created by Mike Oliver on 05/05/99
// discretePrioritizer.h: interface for the DiscretePrioritizer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DISCRETEPRIORITIZER_H__INCLUDED_)
#define _DISCRETEPRIORITIZER_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>

class DiscretePriorityElement {
       public:
        unsigned priority;
        unsigned index;
        DiscretePriorityElement *prev;
        DiscretePriorityElement *next;
        bool enqueued;
};

class DiscretePrioritizer {
       protected:
        DiscretePriorityElement *_elements;
        unsigned _numElts;  // including dequeued elements
        uofm::vector<DiscretePriorityElement *> _heads;
        uofm::vector<DiscretePriorityElement *> _tails;
        unsigned _maxPriority;
        unsigned _minPriority;
        bool _populated;

        mutable DiscretePriorityElement *_curElt;

        // recompute max,min priority
        void _scanForMinMax(unsigned oldPrior, unsigned newPrior);

       public:
        DiscretePrioritizer();  // must call _populate() after ctruction
        DiscretePrioritizer(const uofm::vector<unsigned> &priorities);
        void populate(const uofm::vector<unsigned> &priorities);
        virtual ~DiscretePrioritizer();

        // these two functions are used to change an element's
        // priority and requeue it all in one shot
        void changePriorHead(unsigned idx, unsigned newPriority);
        void changePriorTail(unsigned idx, unsigned newPriority);

        void enqueueHead(unsigned idx, unsigned newPriority);
        void enqueueTail(unsigned idx, unsigned newPriority);

        // remove from the prioritizer
        void dequeue(unsigned idx);

        void dequeueAll();

        unsigned getPriority(unsigned idx) const {
                abkassert(_populated, "not populated");
                return _elements[idx].priority;
        }

        unsigned isEnqueued(unsigned idx) const {
                abkassert(_populated, "not populated");
                return _elements[idx].enqueued;
        }

        unsigned top() const {
                abkassert(_populated, "not populated");
                if (_maxPriority == unsigned(-1)) return UINT_MAX;
                return _heads[_maxPriority]->index;
                // return _heads[_maxPriority]-_elements;
        }

        unsigned bottom() const {
                abkassert(_populated, "not populated");
                if (_maxPriority == unsigned(-1)) return UINT_MAX;
                return _heads[_minPriority]->index;
                // return _heads[_minPriority]-_elements;
        }

        unsigned headTop();
        unsigned tailTop();
        unsigned headBottom();
        unsigned tailBottom();

        unsigned head(unsigned priority);
        unsigned tail(unsigned priority);

        unsigned next();
        unsigned prev();

        unsigned getMaxPriority() const { return _maxPriority; }
        unsigned getMinPriority() const { return _minPriority; }

        friend std::ostream &operator<<(std::ostream &, const DiscretePrioritizer &);
};

#endif  // !defined(_DISCRETEPRIORITIZER_H__INCLUDED_)
