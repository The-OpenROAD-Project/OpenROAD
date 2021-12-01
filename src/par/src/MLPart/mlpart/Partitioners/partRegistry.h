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

//! author=" Andrew Caldwell and Igor Markov"

// This should be a parameter listing partitioners available to,
// e.g. multilevel. May need to be moved elsewhere when we have
// multiple partitioner packages, but ML is not a good place.

#ifndef _PARTREG_H_
#define _PARTREG_H_

//: The typr of partitioners:
// FM, SA, RandomGreedy, Greedy, AGreed, HMetis
// Should be used as a parameter for partitioners
class PartitionerType {
       public:
        enum Type {
                FM,
                SA,
                RandomGreedy,
                Greedy,
                AGreed,
                HMetis
        };
        // , Small, Dereplicating};

       protected:
        Type _type;
        static char strbuf[];

       public:
        PartitionerType(Type type = FM) : _type(type) {}
        PartitionerType(int argc, const char* argv[]);  // catches -part
        operator Type() const { return _type; }
        operator const char*() const;
};

std::ostream& operator<<(std::ostream& os, const PartitionerType& pt);

#endif
