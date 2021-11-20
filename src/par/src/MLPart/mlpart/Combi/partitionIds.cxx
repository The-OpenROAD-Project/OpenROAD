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

// taken from Partitioning/partitionData.cxx
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <Combi/partitionIds.h>
#include <ABKCommon/abkio.h>
#include <ABKCommon/abkrand.h>

using std::ostream;
using std::endl;
using std::vector;

ostream& operator<<(ostream& os, const PartitionIds& part) {
        os << "[ ";
        for (unsigned k = 0; k < 32; k++)
                if (part.isInPart(k)) os << k << " ";
        os << " ]" << endl;

        return os;
}

ostream& operator<<(ostream& os, const Partitioning& part) {
        os << "Begining of the partitioning" << endl;
        os << static_cast<vector<PartitionIds> >(part);
        return os;
}

void Partitioning::randomize(unsigned nParts) {
        RandomRawUnsigned ru;
        for (vector<PartitionIds>::iterator pit = begin(); pit != end(); ++pit) {
                pit->setToPart(ru % nParts);
        }
}
