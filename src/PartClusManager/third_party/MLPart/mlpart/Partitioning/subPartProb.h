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

//! author=" Andrew Caldwell"

#ifndef __SUB_PART_PROB_H__
#define __SUB_PART_PROB_H__

#include <ABKCommon/uofm_alloc.h>
#include <Partitioning/partProb.h>
#include <Placement/layoutBBoxes.h>
#include <iostream>

class Partitioning;
class Placement;

//:  derived from PartitioningProblem
//   (thus is a more specialized class) and allows nontrivial
//   construction of partitioning problems from other partitioning
//   problems. Indeed, the main difference between the two is
//   a pair of [almost identical] new constructors that take placement,
//   hypergraph and bounding boxes of new partitions.
//
//   SubPartitioningProblem is to be used *only* when its specific
//   functionalities/interface are needed, otherwise PartitioningProblem
//   should be used.
class SubPartitioningProblem : public PartitioningProblem {
       private:
        LayoutBBoxes* _ptrPartitionsLB;  // used only if we
                                         // need to allocate memorey
                                         // for _partitionsLB --
                                         // otherwise NULL

       protected:
        const LayoutBBoxes& _partitionsLB;
        const Placement& _placement;
        const HGraphFixed& _hgraphOfNetlist;

        void setBuffer();
        void setHGraph();

       public:
        SubPartitioningProblem(const LayoutBBoxes& partition, const HGraphFixed& hgraphOfNetlist, const Placement& placement);
        // From the information of hgraph and placement of each terminal, get
        // the
        //	partitioning problem of the given partition

        SubPartitioningProblem(const uofm::vector<BBox>& partBBoxes, const HGraphFixed& hgraphOfNetlist, const Placement& placement);
        // This constructor takes longer than the previous one,
        // because the LayoutBBoxes object has to be allocated locally
        // and the vector of BBox es copied in.

        virtual ~SubPartitioningProblem();
        void makeVanilla();  // sets the partitions to
                             // be on a small grid (0,10)(10,10)->etc..
                             // and all terminals to be in either one
                             //'left' or one 'right' padBlock
                             //(essentially, it clears away all actual
                             // locations)
};

#endif
