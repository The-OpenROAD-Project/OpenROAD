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

/*
     97.8.8  Paul Tucker

     Implementation of larger member functions for
     classes related to Interval.
*/

// 970813 mro added double() around _ranIntWidth in four places.
// 970820 pat make ranIntSeqs begin with dark space
// 970824 ilm renamed all Interval -> Interval, same for other classes
// 970825 aec changed insert(0.. to insert(begin.. in mergeInterval
// 970826 aec rewrote the functionality fo IntervalSeq::Merge
// 970921 pat corrected bug in blankInterval
// 970923 pat added IntervalSeq::getLength()
// 971023 pat added IntervalSeq::canonicalize and IntervalSeq::merge
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "interval.h"
#include "ABKCommon/abkcommon.h"
#include "Ctainers/rbtree.h"
#include <algorithm>
#include <ABKCommon/uofm_alloc.h>

using std::ostream;
using std::endl;
using std::flush;
using uofm::vector;

Interval::Interval(const RBInterval& i) : low(i.low), high(i.high) {}

Interval::operator RBInterval() {
        RBInterval rbint(this->low, this->high);
        return rbint;
}

ostream& operator<<(ostream& out, const Interval& arg) {
        out << "[" << arg.low << "," << arg.high << ")";
        return out;
}

ostream& operator<<(ostream& out, const IntervalSeq& arg) {
        unsigned i;

        out << " " << arg.size() << " interval(s)  :  " << flush;
        for (i = 0; i < arg.size(); i++) out << arg[i] << "  ";
        out << endl;
        return out;
}

double IntervalSeq::getLength() const
    /*
       Calculate the sum of all interval lengths within
       the sequence.
    */
{
        double sum = 0;
        unsigned i;

        for (i = 0; i < size(); i++) sum += begin()[i].getLength();

        return sum;
}

void IntervalSeq::blankInterval(const Interval& I)
    /*
       Subtract the argument interval from the sequence by
       erasing its range from any existing interval.  This may
       result in an existing interval being split in two.
    */
{
        unsigned i = 0;

        while ((i < size()) && (!(*this)[i].doesOverlap(I))) i++;

        while ((i < size()) && ((*this)[i].doesOverlap(I))) {
                if (((*this)[i].low < I.low) && ((*this)[i].high > I.high)) {
                        insert(begin() + i + 1, Interval(I.high, (*this)[i].high));
                        (*this)[i].high = I.low;
                        i++;
                } else if ((*this)[i].low < I.low) {
                        (*this)[i].high = I.low;
                        i++;
                } else if ((*this)[i].high > I.high) {
                        (*this)[i].low = I.high;
                        i++;
                } else
                        erase(begin() + i);
        }
}

void RandomIntervalSeqGenerator::set(IntervalSeq& IS)
    /*
       Update the arugment interval sequence with a pseudo-random
       sequence of values.
    */
{
        double x = _leftEdge;

        IS.erase(IS.begin(), IS.end());

        x += (_darkBias * double(_ranIntWidth));

        while (x + _minIntWidth < _maxSeqWidth) {
                double w = _ranIntWidth;
                while ((w < _minIntWidth) || (x + w >= _maxSeqWidth)) w = double(_ranIntWidth);
                IS.push_back(Interval(x, x + w));
                x += w + (_darkBias * double(_ranIntWidth));
        }
}

void RandomIntervalSeqGenerator::setInt(IntervalSeq& IS) {
        double x = _leftEdge;

        IS.erase(IS.begin(), IS.end());

        x += (int)(_darkBias * double(_ranIntWidth));

        while ((x + (int)_minIntWidth) < ((int)_maxSeqWidth)) {
                unsigned w = unsigned(_ranIntWidth);
                while ((w < _minIntWidth) || (w + x >= _maxSeqWidth)) w = unsigned(_ranIntWidth);
                IS.push_back(Interval(x, x + w));
                x += w + (int)(_darkBias * double(_ranIntWidth));
        }
}

void RandomIntervalSeqGenerator::set(IntervalSeq& IS, double maxWidth) {
        double x = _leftEdge;

        IS.erase(IS.begin(), IS.end());

        x += (_darkBias * double(_ranIntWidth));

        while (x + _minIntWidth < maxWidth) {
                double w = double(_ranIntWidth);
                while ((w < _minIntWidth) || (w + x >= maxWidth)) w = double(_ranIntWidth);
                IS.push_back(Interval(x, x + w));
                x += w + (_darkBias * double(_ranIntWidth));
        }
}

void RandomIntervalSeqGenerator::setInt(IntervalSeq& IS, unsigned maxWidth) {
        double x = _leftEdge;

        IS.erase(IS.begin(), IS.end());

        x += (int)(_darkBias * double(_ranIntWidth));

        while ((x + (int)_minIntWidth) < (int)maxWidth) {
                unsigned w = unsigned(_ranIntWidth);
                while ((w < _minIntWidth) || (w + x >= maxWidth)) w = unsigned(_ranIntWidth);
                IS.push_back(Interval(x, x + w));
                x += w + (int)(_darkBias * double(_ranIntWidth));
        }
}

struct compareByLow {
        bool operator()(const Interval i1, const Interval i2) const {
                if (i1.low < i2.low)
                        return true;
                else if (i1.low > i2.low)
                        return false;
                else if (i1.high < i2.high)
                        return true;
                else if (i1.high > i2.high)
                        return false;
                else  // they're identical
                        return (&i1 < &i2);
        }
};

void IntervalSeq::canonicalize()
    // void canonicalize(IntervalSeq& IS)
    /*
       The IntervalSeq may contain overlapping intervals,
       in any order.
       This function restores it to a sorted sequence of non-overlapping
       intervals, with the same coverage.
    */
{
        if (size() > 1) {
                std::sort(begin(), end(), compareByLow());
                unsigned i, j;
                IntervalSeq IStemp;
                Interval newI;

                for (i = 0; i < size(); i = j) {
                        newI = begin()[i];

                        for (j = i + 1; (j < size()) && (begin()[j].low <= newI.high); j++) {
                                if (begin()[j].high > newI.high) newI.high = begin()[j].high;
                        }

                        if (newI.low < newI.high) IStemp.push_back(newI);
                }
                (*this) = IStemp;
        }
}

void IntervalSeq::merge(const IntervalSeq& IS)
    /*
       This IntervalSeqs is updated to be the union with the argument,
       in cannonical form.
    */
{
        for (unsigned i = 0; i < IS.size(); ++i) push_back(IS[i]);
        canonicalize();
}

void IntervalSeq::complement() {
        vector<Interval> origSeq = (*this);
        (*this).erase((*this).begin(), (*this).end());

        double leftEdge = -DBL_MAX;
        for (unsigned i = 0; i < origSeq.size(); i++) {
                if (origSeq[i].low != leftEdge) (*this).push_back(Interval(leftEdge, origSeq[i].low));
                leftEdge = origSeq[i].high;
        }

        if (leftEdge != DBL_MAX) (*this).push_back(Interval(leftEdge, DBL_MAX));
}

void IntervalSeq::intersect(IntervalSeq& ivl) {
        complement();
        ivl.complement();
        merge(ivl);
        complement();
        ivl.complement();  // put it back
}
