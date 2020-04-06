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

//! author="Sep 22, 1997 by Igor Markov"
//! CHANGES="freqDistr.h 971006 ilm inlined methods defined in this file 1.5.1
// 980126 ilm -- added an optional argument to the FreqDistribution ctors
// setting
// the number of subranges for ASCII output"
//! CONTACTS="Igor ABK"

/*
 CHANGES
 971006 ilm inlined methods defined in this file
 1.5.1 980126 ilm -- added an optional argument to the FreqDistribution
                    ctors setting the number of subranges for ASCII output.
*/

#ifndef _FREQDISTR_H_
#define _FREQDISTR_H_

#include "trivStats.h"
#include <ABKCommon/abkcommon.h>
#include <Combi/permut.h>

#include <iostream>
#include <algorithm>
#include <ABKCommon/uofm_alloc.h>

class CumulativeFrequencyDistribution;
typedef CumulativeFrequencyDistribution FreqDistr;

static const unsigned StatsDefaultSubRanges = 10;

/* ---------------------------- INTERFACES ------------------------------*/

//: Computes cumulative frequency distribution,
//	pretty-prints it, saves plots for it and allows to several types of
// queries.
class CumulativeFrequencyDistribution {
       public:
        enum Type {
                Quantities,
                Magnitudes
        };
        unsigned numSubranges;
        // for ASCII output. Default is StatsDefaultSubRanges

       protected:
        Type _type;
        uofm::vector<double> _data;
        // original data sorted
        uofm::vector<double> _percBelow;
        Permutation _sortedIdx;
        // to look up % below by orig. index
        /*Permutation    _sortedIdxInverse; */

        void computePercBelow();
        unsigned getNumSubranges() const;

       public:
        CumulativeFrequencyDistribution(const uofm::vector<double>&, unsigned nSubRanges = StatsDefaultSubRanges, Type type = Quantities);
        // the argument can be any (not necessarily sorted) vector
        CumulativeFrequencyDistribution(const uofm::vector<unsigned>&, unsigned nSubRanges = StatsDefaultSubRanges, Type type = Quantities);

        unsigned getSize() const { return _data.size(); }
        inline double getPercentBelow(unsigned k) const;
        inline double getPercentBelow(double val) const;
        inline double getValByPercentBelow(double percBelow) const;
        void saveXYPlot(const char* xyFileName) const;
        // Save to xyFileName file by plot format.

        void printEqualSubranges(std::ostream&) const;
        // the below two are called by operator <<
        void printEquipotentSubranges(std::ostream&) const;
};
std::ostream& operator<<(std::ostream&, const CumulativeFrequencyDistribution& stats);
// print out to standard output

/* ---------------------------- IMPLEMENTATIONS ------------------------------
 */

inline double CumulativeFrequencyDistribution::getPercentBelow(unsigned k) const {
        abkassert(k < getSize(), " Index out of range for \'getPercentBelow\'");
        return _percBelow[_sortedIdx[k]];
}

inline double CumulativeFrequencyDistribution::getPercentBelow(double val) const {
        unsigned idx = std::upper_bound(_data.begin(), _data.end(), val) - _data.begin();
        return _percBelow[idx];
}

inline double CumulativeFrequencyDistribution::getValByPercentBelow(double percBelow) const {
        unsigned idx = std::upper_bound(_percBelow.begin(), _percBelow.end(), percBelow) - _percBelow.begin();
        idx = std::min(idx, getSize() - 1);
        return _data[idx];
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "970922, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
