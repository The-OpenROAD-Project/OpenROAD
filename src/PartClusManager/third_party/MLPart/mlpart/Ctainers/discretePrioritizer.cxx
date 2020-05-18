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
// discretePrioritizer.cxx: implementation of the DiscretePrioritizer class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "discretePrioritizer.h"

using std::ostream;
using std::endl;
using uofm::vector;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
DiscretePrioritizer::DiscretePrioritizer() : _elements(NULL), _maxPriority(unsigned(-1)), _minPriority(UINT_MAX), _populated(false), _curElt(NULL) {}

DiscretePrioritizer::DiscretePrioritizer(const vector<unsigned> &priorities) : _elements(NULL), _numElts(UINT_MAX), _maxPriority(unsigned(-1)), _minPriority(UINT_MAX), _populated(false), _curElt(NULL) { populate(priorities); }

void DiscretePrioritizer::populate(const vector<unsigned> &priorities) {
        abkfatal(!_populated, "can't populate DiscretePrioritizer twice");
        _populated = true;
        typedef DiscretePriorityElement *pEltType;
        _numElts = priorities.size();
        _elements = new DiscretePriorityElement[_numElts];
        _minPriority = UINT_MAX;
        _maxPriority = UINT_MAX;
        unsigned i;
        for (i = 0; i < priorities.size(); i++) {
                unsigned priority = priorities[i];

                if (priority == UINT_MAX) continue;

                if (priority > _maxPriority || _maxPriority == UINT_MAX) _maxPriority = priority;
                if (priority < _minPriority) _minPriority = priority;
        }
        _heads.clear();
        _heads.insert(_heads.end(), _maxPriority + 1, static_cast<DiscretePriorityElement *>(NULL));

        _tails.clear();
        _tails.insert(_tails.end(), _maxPriority + 1, static_cast<DiscretePriorityElement *>(NULL));

        for (i = 0; i < priorities.size(); i++) {
                DiscretePriorityElement &elt = _elements[i];
                elt.index = i;
                elt.next = elt.prev = NULL;
                elt.priority = UINT_MAX;
                unsigned curPriority = priorities[i];
                if (curPriority != UINT_MAX) {
                        elt.enqueued = true;
                        changePriorHead(i, curPriority);
                } else
                        elt.enqueued = false;
        }
}

DiscretePrioritizer::~DiscretePrioritizer() { delete[] _elements; }

void DiscretePrioritizer::changePriorHead(unsigned idx, unsigned newPriority) {
        if (newPriority + 1 > _heads.size()) {
                abkfatal(_heads.size() == _tails.size(), "Size mismatch");
                _heads.insert(_heads.end(), newPriority + 1 - _heads.size(), static_cast<DiscretePriorityElement *>(NULL));
                _tails.insert(_tails.end(), newPriority + 1 - _tails.size(), static_cast<DiscretePriorityElement *>(NULL));
        }
        if (_maxPriority == UINT_MAX)
                _maxPriority = _minPriority = newPriority;  // empty prioritizer
        else if (newPriority > _maxPriority)
                _maxPriority = newPriority;
        DiscretePriorityElement &elt = _elements[idx];
        abkfatal(elt.enqueued,
                 "Attempt to change priority of element"
                 " not in queue");
        unsigned oldPriority = elt.priority;
        if (elt.prev) elt.prev->next = elt.next;
        if (elt.next) elt.next->prev = elt.prev;
        if (oldPriority != UINT_MAX) {
                if (_heads[oldPriority] == &elt) _heads[oldPriority] = elt.next;
                if (_tails[oldPriority] == &elt) _tails[oldPriority] = elt.prev;
        }

        DiscretePriorityElement *oldHead = _heads[newPriority];
        DiscretePriorityElement *oldTail = _tails[newPriority];

        elt.prev = NULL;
        elt.next = oldHead;
        elt.priority = newPriority;

        _heads[newPriority] = &elt;

        if (oldHead) oldHead->prev = &elt;
        if (oldTail == NULL) _tails[newPriority] = &elt;

        _scanForMinMax(oldPriority, newPriority);

        abkassert(_minPriority <= _maxPriority, "min priority exceeds max");
}

void DiscretePrioritizer::changePriorTail(unsigned idx, unsigned newPriority) {
        if (newPriority + 1 > _heads.size()) {
                abkfatal(_heads.size() == _tails.size(), "Size mismatch");
                _heads.insert(_heads.end(), newPriority + 1 - _heads.size(), static_cast<DiscretePriorityElement *>(NULL));
                _tails.insert(_tails.end(), newPriority + 1 - _tails.size(), static_cast<DiscretePriorityElement *>(NULL));
        }
        if (_maxPriority == UINT_MAX)
                _maxPriority = _minPriority = newPriority;  // empty prioritizer
        else if (newPriority > _maxPriority)
                _maxPriority = newPriority;
        DiscretePriorityElement &elt = _elements[idx];
        abkfatal(elt.enqueued,
                 "Attempt to change priority of element"
                 " not in queue");
        unsigned oldPriority = elt.priority;
        if (elt.next) elt.next->prev = elt.prev;
        if (elt.prev) elt.prev->next = elt.next;
        if (oldPriority != UINT_MAX) {
                if (_heads[oldPriority] == &elt) _heads[oldPriority] = elt.next;
                if (_tails[oldPriority] == &elt) _tails[oldPriority] = elt.prev;
        }

        DiscretePriorityElement *oldTail = _tails[newPriority];
        DiscretePriorityElement *oldHead = _heads[newPriority];

        elt.next = NULL;
        elt.prev = oldTail;
        elt.priority = newPriority;

        _tails[newPriority] = &elt;

        if (oldTail) oldTail->next = &elt;
        if (oldHead == NULL) _heads[newPriority] = &elt;

        _scanForMinMax(oldPriority, newPriority);

        abkassert(_minPriority <= _maxPriority, "min priority exceeds max");
}
void DiscretePrioritizer::enqueueHead(unsigned idx, unsigned newPriority) {
        abkassert(idx < _numElts, "attempt to enqueue element out of range");
        DiscretePriorityElement &elt = _elements[idx];
        abkassert(!elt.enqueued, "attempt to enqueue element already enqueued");
        abkassert(elt.next == NULL, "attempt to enqueue element with non-null next");
        abkassert(elt.prev == NULL, "attempt to enqueue element with non-null prev");
        abkassert(elt.priority == UINT_MAX,
                  "attempt to enqueue element with "
                  "pre-existent priority");
        elt.enqueued = true;
        changePriorHead(idx, newPriority);
}

void DiscretePrioritizer::enqueueTail(unsigned idx, unsigned newPriority) {
        abkassert(idx < _numElts, "attempt to enqueue element out of range");
        DiscretePriorityElement &elt = _elements[idx];
        abkassert(!elt.enqueued, "attempt to enqueue element already enqueued");
        abkassert(elt.next == NULL, "attempt to enqueue element with non-null next");
        abkassert(elt.prev == NULL, "attempt to enqueue element with non-null prev");
        abkassert(elt.priority == UINT_MAX,
                  "attempt to enqueue element with "
                  "pre-existent priority");
        elt.enqueued = true;
        changePriorTail(idx, newPriority);
}

void DiscretePrioritizer::dequeue(unsigned idx) {
        abkassert(_populated, "Can't dequeue from unpopulated prioritizer");
        DiscretePriorityElement &elt = _elements[idx];
        abkassert(elt.enqueued, "Can't dequeue an element that's not enqueued");
        elt.enqueued = false;
        unsigned oldPriority = elt.priority;
        if (elt.next) elt.next->prev = elt.prev;
        if (elt.prev) elt.prev->next = elt.next;
        if (oldPriority != UINT_MAX) {
                if (_heads[oldPriority] == &elt) _heads[oldPriority] = elt.next;
                if (_tails[oldPriority] == &elt) _tails[oldPriority] = elt.prev;
        }
        _scanForMinMax(oldPriority, UINT_MAX);
        elt.next = elt.prev = NULL;
        elt.priority = UINT_MAX;
}

void DiscretePrioritizer::dequeueAll() {
        abkassert(_populated, "Can't dequeue from unpopulated prioritizer");
        unsigned i;
        for (i = 0; i < _numElts; i++) {
                DiscretePriorityElement &elt = _elements[i];
                elt.enqueued = false;
                elt.next = elt.prev = NULL;
                elt.priority = UINT_MAX;
        }
        DiscretePriorityElement *const nullElt = NULL;
        for (i = 0; i != _heads.size(); i++) _heads[i] = nullElt;
        for (i = 0; i != _tails.size(); i++) _heads[i] = nullElt;
        _minPriority = _maxPriority = UINT_MAX;
        _curElt = NULL;
}
void DiscretePrioritizer::_scanForMinMax(unsigned oldPrior, unsigned newPrior) {
        if (_maxPriority == UINT_MAX) {
                abkassert(oldPrior == UINT_MAX,
                          "_scanForMinMax() called supposedly "
                          "reprioritizing element when no elements enqueued");
                _maxPriority = _minPriority = newPrior;
                return;
        }
        if (newPrior > _maxPriority && newPrior != UINT_MAX) {
                _maxPriority = newPrior;
                abkassert(_minPriority < _maxPriority, "Bad priority order");
        }
        if (newPrior < _minPriority) {
                _minPriority = newPrior;
                abkassert(_minPriority < _maxPriority, "Bad priority order, inst 2");
        }
        if (oldPrior == _maxPriority) {
                while (_maxPriority != unsigned(-1) && _heads[_maxPriority] == NULL) --_maxPriority;
                if (_maxPriority == unsigned(-1)) {
                        _minPriority = _maxPriority = UINT_MAX;
                        return;
                }
        }
        if (oldPrior == _minPriority) {
                while (_heads[_minPriority] == NULL) ++_minPriority;
        }

        abkassert(_minPriority <= _maxPriority, "Bad priority order, inst 3");
}

unsigned DiscretePrioritizer::headTop() {
        abkassert(_populated, "not populated");
        if (_maxPriority == unsigned(-1)) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _heads[_maxPriority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::tailTop() {
        abkassert(_populated, "not populated");
        if (_maxPriority == unsigned(-1)) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _tails[_maxPriority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::headBottom() {
        abkassert(_populated, "not populated");
        if (_maxPriority == unsigned(-1)) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _heads[_minPriority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::tailBottom() {
        abkassert(_populated, "not populated");
        if (_maxPriority == unsigned(-1)) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _tails[_minPriority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::head(unsigned priority) {
        abkassert(_populated, "not populated");
        if (priority > _maxPriority || priority < _minPriority) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _heads[priority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::tail(unsigned priority) {
        abkassert(_populated, "not populated");
        if (priority > _maxPriority || priority < _minPriority) {
                _curElt = NULL;
                return UINT_MAX;
        }
        _curElt = _tails[priority];
        return _curElt->index;
}

unsigned DiscretePrioritizer::next() {
        abkassert(_populated, "not populated");
        if (_curElt == NULL) return UINT_MAX;
        _curElt = _curElt->next;
        if (_curElt == NULL) return UINT_MAX;
        return _curElt->index;
}

unsigned DiscretePrioritizer::prev() {
        abkassert(_populated, "not populated");
        if (_curElt == NULL) return UINT_MAX;
        _curElt = _curElt->prev;
        if (_curElt == NULL) return UINT_MAX;
        return _curElt->index;
}

ostream &operator<<(ostream &os, const DiscretePrioritizer &prior) {
        os << "Min priority: " << prior._minPriority << " Max Priority: " << prior._maxPriority << " top: " << prior.top() << endl;
        if (prior.top() == UINT_MAX) {
                os << "Prioritizer empty" << endl;
                return os;
        }
        for (unsigned priority = prior._minPriority; priority <= prior._maxPriority; priority++) {
                DiscretePriorityElement *heads = prior._heads[priority], *tails = prior._tails[priority];
                if (heads == NULL) {
                        abkfatal(prior._tails[priority] == NULL, "Heads null, tails not");
                        continue;
                }
                abkfatal(tails != NULL, "Tails null, heads not");
                os << "Priority " << priority << ": ";
                DiscretePriorityElement *elt = heads, *oldElt;
                unsigned counter = 0;
                for (; elt; oldElt = elt, elt = elt->next) {
                        os << elt - prior._elements << " ";
                        if ((++counter) % 10 == 0) os << endl << "\t";
                }
                os << endl;
                abkfatal(oldElt == tails, "Corrupted linked list in prioritizer");
        }
        abkfatal(prior._minPriority <= prior._maxPriority,
                 "min priority "
                 "exceeds max");

        return os;
}
