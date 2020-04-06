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

//! author=" 97.8.4  Paul Tucker "

// CHANGES
// 970817 ilm uninlined stream I/O and moved into Interval.cxx
//                      this does not trigger CC +O5 optimizer bug any more
// 970818 pat corrected index checking on operator[] def
// 970824 ilm renamed all Interval -> Interval, same for other classes
// 970826 aec changed out of range error msg. to be more specific
//		changed bool overlaps to doesOverlap
// 970826 ilm removed IntervalSeq::Merge
// 970826 ilm changed (double) to static_cast<double>()
// 970917 ilm added getLength
// 970919 pat corrected overlap and doesOverlap
// 970923 pat added getLength() to IntervalSeq
// 971023 pat added canonicalize and merge to IntervalSeq
// 971113 mro unified seeded, unseeded ctors in RandomIntervalGenerator,
//            RandomIntervalSeqGenerator (default value UINT_MAX means
//            use clock seed)
// 980217 ilm added Interval::getMiddle()
// 990221 mro moved to Geoms, without IntervalSeqLayer (which remains
//            in Placement

#ifndef _INTERVALS_H_
#define _INTERVALS_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <iostream>

/******* classes defined in this file **********/

class Interval;
class IntervalSeq;

class RandomIntervalGenerator;
class RandomIntervalSeqGenerator;

/******** definitions **********/

class RBInterval;  // to allow type conversions

//:  An interval is a half-open [low,high)
//   interval on a line, in double coordinates.
class Interval {
       public:
        double low;
        double high;

        Interval() {};
        Interval(double s, double e) : low(s), high(e) {};
        Interval(const RBInterval& i);
        operator RBInterval();

        /*
           There is some ambiguity as to what the operators + and -
           should mean when applied to pairs of intervals.  When
           applied to an interval and a scalar, the coordinate shift
           operation is performed.
        */

        double getLength() const { return high - low; }
        double getMiddle() const { return 0.5 * (high + low); }

        Interval operator-(const double arg) const { return Interval(low - arg, high - arg); }

        Interval& operator-=(const double arg) {
                low -= arg;
                high -= arg;
                return *this;
        }

        Interval operator+(const double arg) const { return Interval(low + arg, high + arg); }

        Interval& operator+=(const double arg) {
                low += arg;
                high += arg;
                return *this;
        }

        bool operator==(const Interval& arg) const { return (low == arg.low) && (high == arg.high); }

        bool operator!=(const Interval& arg) const { return (low != arg.low) || (high != arg.high); }

        bool operator<(const Interval& arg) const { return high < arg.low; }
        // Comparison operators:
        bool operator>(const Interval& arg) const { return low > arg.high; }

        bool doesOverlap(const Interval& arg) const;
        double overlap(const Interval& arg) const;

        friend std::ostream& operator<<(std::ostream& out, const Interval& arg);
        friend std::istream& operator>>(std::istream& in, Interval& arg);
};

inline bool Interval::doesOverlap(const Interval& arg) const {
        if (low <= arg.low)
                return (arg.low < high);
        else
                return (low < arg.high);
}

inline double Interval::overlap(const Interval& arg) const {
        if (low <= arg.low) {
                if (arg.low < high)
                        return (std::min(high, arg.high) - arg.low);
                else
                        return 0;
        } else  // arg.low < low
        {
                if (low < arg.high)
                        return (std::min(high, arg.high) - low);
                else
                        return 0;
        }
}

std::ostream& operator<<(std::ostream& out, const Interval& arg);
std::istream& operator>>(std::istream& in, Interval& arg);

class IntervalSeq : public uofm::vector<Interval> {
        /*
           An IntervalSeq is a sequence of intervals.

           It is expected that intervals will always be ordered.
           Coordinates can be either non-decreasing, or non-increasing,
           with vector index, but they should remain monotone.
           (I.e. if x[i].low < x[i].high, then x[i].high < x[i+1].low)
           However, this is not enforced.
        */
       public:
        IntervalSeq() {}

        IntervalSeq(unsigned numInts) : uofm::vector<Interval>(numInts) {}

        Interval& operator[](unsigned index) {
                abkassert(index < size(), "Interval[]: Index out of range");
                return *(this->begin() + index);
        }

        const Interval& operator[](unsigned index) const {
                abkassert(index < size(), "Interval[]: Index out of range");
                return *(this->begin() + index);
        }

        void blankInterval(const Interval& I);

        double getLength() const;
        void canonicalize();
        void merge(const IntervalSeq& IS);

        void complement();                 // change the interval into its complement
        void intersect(IntervalSeq& ivl);  // this interval becomes
        // the intersection of the two

        friend std::ostream& operator<<(std::ostream& out, const IntervalSeq& arg);
        friend std::istream& operator>>(std::istream& in, IntervalSeq& arg);
};

std::ostream& operator<<(std::ostream& out, const IntervalSeq& arg);
std::istream& operator>>(std::istream& in, IntervalSeq& arg);

class RandomIntervalGenerator {
       protected:
        RandomDouble _rng;

       public:
        RandomIntervalGenerator(const double low, const double high) : _rng(low, high) {}

        ~RandomIntervalGenerator() {}

        // void seed() {_rng.seed();}
        // void seed(unsigned seedN) {_rng.seed(seedN);}
        // void CPUseed(double seedN=0) {_rng.CPUseed(seedN);}

        void set(Interval& I) {
                double s = _rng;
                double e = _rng;

                if (s <= e) {
                        I.low = s;
                        I.high = e;
                } else {
                        I.low = e;
                        I.high = s;
                }
        }

        void setInt(Interval& I) {
                int s = static_cast<int>(fabs(_rng.operator double()));
                int e = static_cast<int>(fabs(_rng.operator double()));

                while (e == s) e = static_cast<int>(fabs(_rng.operator double()));

                if (s < e) {
                        I.low = s;
                        I.high = e;
                } else {
                        I.low = e;
                        I.high = s;
                }
        }
};

class RandomIntervalSeqGenerator {
       protected:
        RandomDouble _ranIntWidth;
        double _leftEdge, _minIntWidth;
        double _minSeqWidth, _maxSeqWidth, _darkBias;

       public:
        RandomIntervalSeqGenerator(double leftEdge, double minIntWidth, double maxIntWidth, double minSeqWidth, double maxSeqWidth, double darkBias) : _ranIntWidth(minIntWidth, maxIntWidth), _leftEdge(leftEdge), _minIntWidth(minIntWidth), _minSeqWidth(minSeqWidth), _maxSeqWidth(maxSeqWidth), _darkBias(darkBias) {}

        RandomIntervalSeqGenerator(double leftEdge, double minIntWidth, double maxIntWidth, double minSeqWidth, double maxSeqWidth, double darkBias, unsigned seed) : _ranIntWidth(minIntWidth, maxIntWidth), _leftEdge(leftEdge), _minIntWidth(minIntWidth), _minSeqWidth(minSeqWidth), _maxSeqWidth(maxSeqWidth), _darkBias(darkBias) { (void)seed; }
        ~RandomIntervalSeqGenerator() {}

        // void seed() {_ranIntWidth.seed();}
        // void seed(unsigned seedN) {_ranIntWidth.seed(seedN);}
        // void CPUseed(double seedN=0) {_ranIntWidth.CPUseed(seedN);}

        void set(IntervalSeq& IS);
        void setInt(IntervalSeq& IS);
        void set(IntervalSeq& IS, double maxWidth);
        void setInt(IntervalSeq& IS, unsigned maxWidth);
};

#endif
