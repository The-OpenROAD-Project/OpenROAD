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

#ifndef _TRIVSTATS_H_
#define _TRIVSTATS_H_

#include "ABKCommon/abkcommon.h"
#include <iostream>
#include <ABKCommon/uofm_alloc.h>

class Correlation;
class TrivialStats;
class TrivialStatsWithStdDev;

typedef Correlation Corr;

/* ---------------------------- INTERFACES ------------------------------ */

typedef uofm::vector<double> StatisticalDataDouble;
typedef uofm::vector<unsigned> StatisticalDataUnsigned;

//: Computes the correlation of vectors of same sizes
class Correlation {
        double _corr;

       public:
        Correlation(const uofm::vector<double>&, const uofm::vector<double>&);
        Correlation(const uofm::vector<unsigned>&, const uofm::vector<unsigned>&);
        operator double() const { return _corr; }
};

//: Computes the number, sum, max, avg and min
//	value of data vector as well as pretty print of the datas.
class TrivialStats {
       protected:
        double _max, _min, _avg, _tot, _stdDev;
        unsigned _num;

       public:
        TrivialStats(const uofm::vector<float>&);
        TrivialStats(const uofm::vector<double>&);
        TrivialStats(const uofm::vector<unsigned>&);

        /*TrivialStats(StatisticalDataDouble::const_iterator data_begin,
                       StatisticalDataDouble::const_iterator data_end)
                       { abkfatal(0," TrivialStats ctor not yet implemented"); }
        */
        double getMax() const { return _max; }
        double getMin() const { return _min; }
        double getAvg() const { return _avg; }
        double getTot() const { return _tot; }
        unsigned getNum() const { return _num; }
};

std::ostream& operator<<(std::ostream&, const TrivialStats& stats);

//: Computes the standard deviation of data vector.
class TrivialStatsWithStdDev : public TrivialStats {
       protected:
        double _stdDev;

       public:
        TrivialStatsWithStdDev(const uofm::vector<float>& data);
        TrivialStatsWithStdDev(const uofm::vector<double>& data);
        TrivialStatsWithStdDev(const uofm::vector<unsigned>& data);
        /*TrivialStatsWithStdDev(vector<double>::const_iterator data_begin,
                              vector<double>::const_iterator data_end)
                    { abkfatal(0," TrivialStatsWithStdDev ctor not yet
           implemented");}
        */
        double getStdDev() const { return _stdDev; }
};

std::ostream& operator<<(std::ostream&, const TrivialStatsWithStdDev& stats);

/* ---------------------------- IMPLEMENTATIONS ------------------------------
 */

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "970922, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
