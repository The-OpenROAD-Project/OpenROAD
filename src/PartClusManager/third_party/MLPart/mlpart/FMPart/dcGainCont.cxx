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

#include "dcGainCont.h"

using std::ostream;
using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

ostream& operator<<(ostream& os, const DestCentricGainContainerT& gc) {
        if (gc.getMaxGain() == -1)
                cout << "setMaxGain has not been called. "
                     << "Can't print DestCentricGainContainerT";

        os << "  MaxGain:  " << gc.getMaxGain() << endl;

        for (unsigned k = 0; k < gc.getNumPrioritizers(); k++) {
                os << "Moves To Partition " << k << endl;
                if (gc._params.mapBasedGC) {
                        for (BucketArrayM::const_reverse_iterator it = gc.getPrioritizerM(k).rbegin(); it != gc.getPrioritizerM(k).rend(); ++it) {
                                SVGainElement* cur = static_cast<SVGainElement* const>(it->second._next);
                                if (cur == NULL)
                                        ;
                                // os << setw(4) << (it->first) -
                                // gc.getMaxGain() <<") ";
                                // os<<" EMPTY "<<endl;
                                else {
                                        os << setw(4) << static_cast<int>(it->first) - gc.getMaxGain() << ") ";
                                        for (; cur != NULL; cur = static_cast<SVGainElement* const>(cur->_next)) {

                                                os << "[#:" << gc.getRepository().getElementId(*cur);
                                                os << " g:" << cur->getGain() << " + ";
                                                os << cur->getRealGain() - cur->getGain() << "]";
                                        }
                                        os << endl;
                                }
                        }
                } else if (gc._params.hashBasedGC) {
                        for (BucketArrayHL::const_reverse_iterator it = gc.getPrioritizerHL(k).rbegin(); it != gc.getPrioritizerHL(k).rend(); ++it) {
                                SVGainElement* cur = static_cast<SVGainElement* const>((*it)._next);
                                if (cur == NULL)
                                        ;
                                // os << setw(4) << (it.getWeight()) -
                                // gc.getMaxGain() <<") ";
                                // os<<" EMPTY "<<endl;
                                else {
                                        os << setw(4) << static_cast<int>(it.getWeight()) - gc.getMaxGain() << ") ";
                                        for (; cur != NULL; cur = static_cast<SVGainElement* const>(cur->_next)) {

                                                os << "[#:" << gc.getRepository().getElementId(*cur);
                                                os << " g:" << cur->getGain() << " + ";
                                                os << cur->getRealGain() - cur->getGain() << "]";
                                        }
                                        os << endl;
                                }
                        }
                } else {
                        for (int g = gc.getPrioritizer(k).size() - 1; g >= 0; g--) {
                                SVGainElement* cur = static_cast<SVGainElement* const>(gc.getPrioritizer(k)[g]._next);
                                if (cur == NULL)
                                        ;
                                // os << setw(4) << g - gc.getMaxGain() <<") ";
                                // os<<" EMPTY "<<endl;
                                else {
                                        os << setw(4) << g - gc.getMaxGain() << ") ";
                                        for (; cur != NULL; cur = static_cast<SVGainElement* const>(cur->_next)) {

                                                os << "[#:" << gc.getRepository().getElementId(*cur);
                                                os << " g:" << cur->getGain() << " + ";
                                                os << cur->getRealGain() - cur->getGain() << "]";
                                        }
                                        os << endl;
                                }
                        }
                }
        }
        return os;
}

void DestCentricGainContainerT::reinitialize() {
        if (_params.mapBasedGC) {
                // disconnect all the elements
                for (unsigned k = 0; k < _prioritizer2.size(); k++)
                        for (unsigned n = 0; n < _numNodes; n++) getElement(n, k).disconnect();

                // null the GainList in each GainBucket
                for (unsigned k = 0; k < _prioritizer2.size(); k++) _prioritizer2[k].removeAll();
        } else if (_params.hashBasedGC) {
                // disconnect all the elements
                for (unsigned k = 0; k < _prioritizer3.size(); k++)
                        for (unsigned n = 0; n < _numNodes; n++) getElement(n, k).disconnect();

                // null the GainList in each GainBucket
                for (unsigned k = 0; k < _prioritizer3.size(); k++) _prioritizer3[k].removeAll();
        } else {
                // disconnect all the elements
                for (unsigned k = 0; k < _prioritizer1.size(); k++)
                        for (unsigned n = 0; n < _numNodes; n++) getElement(n, k).disconnect();

                // null the GainList in each GainBucket
                for (unsigned k = 0; k < _prioritizer1.size(); k++) _prioritizer1[k].removeAll();
        }
        _highestArray = 0;
}

void DestCentricGainContainerT::setupClip() {
        if (_params.mapBasedGC) {
                for (unsigned k = 0; k < _prioritizer2.size(); k++) _prioritizer2[k].setupForClip();
        } else if (_params.hashBasedGC) {
                for (unsigned k = 0; k < _prioritizer3.size(); k++) _prioritizer3[k].setupForClip();
        } else {
                for (unsigned k = 0; k < _prioritizer1.size(); k++) _prioritizer1[k].setupForClip();
        }
}

void DestCentricGainContainerT::setMaxGain(unsigned maxG) {
        _maxGain = maxG;
        if (_params.mapBasedGC) {
                for (vector<BucketArrayM>::iterator it = _prioritizer2.begin(); it != _prioritizer2.end(); ++it) it->setMaxGain(maxG);
        } else if (_params.hashBasedGC) {
                for (vector<BucketArrayHL>::iterator it = _prioritizer3.begin(); it != _prioritizer3.end(); ++it) it->setMaxGain(maxG);
        } else {
                for (vector<BucketArray>::iterator it = _prioritizer1.begin(); it != _prioritizer1.end(); ++it) it->setMaxGain(maxG);
        }
}

bool DestCentricGainContainerT::isConsistent() {
        // return true; //uncomment this to run debug mode significantly
        // faster...
        bool consistent = true;
        if (_params.mapBasedGC) {
                for (unsigned k = 0; k < _prioritizer2.size(); k++) {
                        if (!_prioritizer2[k].isConsistent()) {
                                consistent = false;
                                cout << "BucketArrayM " << k << " is not consistent" << endl;
                        }
                }
        } else if (_params.hashBasedGC) {
                for (unsigned k = 0; k < _prioritizer3.size(); k++) {
                        if (!_prioritizer3[k].isConsistent()) {
                                consistent = false;
                                cout << "BucketArrayHL " << k << " is not consistent" << endl;
                        }
                }
        } else {
                for (unsigned k = 0; k < _prioritizer1.size(); k++) {
                        if (!_prioritizer1[k].isConsistent()) {
                                consistent = false;
                                cout << "BucketArray " << k << " is not consistent" << endl;
                        }
                }
        }

        return consistent;
}
