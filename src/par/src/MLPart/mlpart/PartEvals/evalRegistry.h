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

// created by Andrew Caldwell and Igor Markov on 05/17/98

// This should be a parameter to partitioner, so that differently
// templated move managers can be instantiated.

// This class assumes there's only package with evaluators

#include <iostream>

class PartitioningProblem;
class Partitioning;

#ifndef _EVALREG_H_
#define _EVALREG_H_

class PartEvalType {
       public:
        enum Type {
                NetCutWBits,
                NetCutWConfigIds,
                NetCutWNetVec,
                BBox1Dim,
                /*
                  BBox2Dim, ModifiedNetCut,
                  BBox1DimWCheng, BBox2DimWCheng, BBox2DimWRSMT,
                  HBBox, HBBoxWCheng, HBBoxWRSMT,
                  HBBox0, HBBox0wCheng, HBBox0wRSMT,
*/ StrayNodes,
                NetCut2way,
                StrayNodes2way,
                NetCut2wayWWeights
        };
        // single-cell move stucture
        // is the default for names,
        // so is "with net tallies"
       protected:
        Type _type;
        static char strbuf[];

       public:
        PartEvalType(Type type = NetCutWNetVec) : _type(type) {};
        PartEvalType(int argc, const char* argv[]);  // catches -eval <name>

        operator Type() const { return _type; }

        operator const char*() const;
};

std::ostream& operator<<(std::ostream& os, const PartEvalType& pet);

#endif
