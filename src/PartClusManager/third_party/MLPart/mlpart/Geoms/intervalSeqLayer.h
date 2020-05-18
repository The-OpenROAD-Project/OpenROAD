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

//   97.8.4  Paul Tucker
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
// 990222 mro separated IntervalSeqLayer (which stays in Placement)
//            from the other classes originally from interval.h
//            (which go to Geoms)

#ifndef _INTERVALSEQLAYER_H_
#define _INTERVALSEQLAYER_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <Geoms/interval.h>
#include <iostream>

/******* classes defined in this file **********/

class IntervalSeqLayer;

/******** definitions **********/

//:
class IntervalSeqLayer : public uofm::vector<IntervalSeq> {
       public:
        IntervalSeqLayer() {}
        IntervalSeqLayer(unsigned n) : uofm::vector<IntervalSeq>(n) {}
        IntervalSeqLayer(unsigned n, const IntervalSeq& is) : uofm::vector<IntervalSeq>(n, is) {}
        IntervalSeqLayer(uofm::vector<IntervalSeq>& is) : uofm::vector<IntervalSeq>(is) {}

        //   IntervalSeqLayer& operator=(uofm::vector<IntervalSeq>& is)
        //   {return *this=is;}
};

#endif
