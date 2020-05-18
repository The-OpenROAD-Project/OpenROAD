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

#ifndef _GAIN_LIST__H__
#define _GAIN_LIST__H__

#include "ABKCommon/abkcommon.h"
#include "svGainElmt.h"

class GainList : public SVGainElement {
       public:
        GainList() : SVGainElement() {}
        ~GainList() {}

        bool isEmpty() const { return _next == NULL; }

        SVGainElement& getFirst() const {
                abkassert(_next != NULL, "can't call getFirst on an empty GList");
                return *_next;
        }

        void pop();                      // remove the item at the head
        void push(SVGainElement& elmt);  // add item at the head

        SVGainElement* getLastItem();

        void insertAhead(GainList&);
        void insertBehind(GainList&);

        bool isConsistent();

        friend std::ostream& operator<<(std::ostream& os, const GainList&);
};

//____________________IMPLEMENTATIONS__________________________//

//_______GainList functions_____________//

inline void GainList::pop() {  // the GList never owned the Element memory, so
                               // it does
        // not have to delete any of it.
        abkwarn(_next != NULL, "calling pop on an empty GB list");
        if (_next != NULL) {
                SVGainElement* toRemove = _next;
                _next = toRemove->_next;  // change the next ptr.
                toRemove->_next = NULL;   // disconnect the removed item
                toRemove->_prev = NULL;

                if (_next != NULL) _next->_prev = this;
        }
}

inline void GainList::push(SVGainElement& elmt) {
        elmt._prev = this;
        elmt._next = _next;
        if (_next != NULL) _next->_prev = &elmt;
        _next = &elmt;
}

inline SVGainElement* GainList::getLastItem() {
        SVGainElement* last = _next;
        if (last == NULL) return NULL;
        for (; last->_next != NULL; last = last->_next) {
        }

        return last;
}

inline void GainList::insertAhead(GainList& toAdd) {
        if (toAdd._next == NULL)
                return;
        else if (_next == NULL) {
                _next = toAdd._next;
                _next->_prev = this;
        } else {
                SVGainElement* tmpTop = _next;
                _next = toAdd._next;
                _next->_prev = this;
                SVGainElement* toAddEnd = toAdd.getLastItem();
                tmpTop->_prev = toAddEnd;
                toAddEnd->_next = tmpTop;
        }

        toAdd._next = NULL;
}

inline void GainList::insertBehind(GainList& toAdd) {
        if (toAdd._next == NULL)
                return;
        else if (_next == NULL) {
                _next = toAdd._next;
                _next->_prev = this;
        } else {
                SVGainElement* mainListEnd = getLastItem();
                mainListEnd->_next = toAdd._next;
                mainListEnd->_next->_prev = mainListEnd;
        }
        toAdd._next = NULL;
}

inline bool GainList::isConsistent() {
        SVGainElement* next, *prev;
        prev = this;
        next = _next;
        bool consistent = true;
        for (; next != NULL; prev = next, next = next->_next) {
                if (prev->_next != next || next->_prev != prev) {
                        std::cout << "gain list mismatch" << std::endl;
                        consistent = false;
                }
        }
        return consistent;
}

inline std::ostream& operator<<(std::ostream& os, const GainList& lst) {
        SVGainElement* cur = lst._next;
        if (cur == NULL)
                os << " EMPTY " << std::endl;
        else {
                for (; cur != NULL; cur = cur->_next) os << *cur << " ";
                os << std::endl;
        }
        return os;
}

#endif
