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

#ifndef ORDERING
#define ORDERING

#include <vector>
#include <algorithm>

class ordering {
        std::vector<int> index_to_loc;
        std::vector<int> loc_to_index;

       public:
        ordering(int size) : index_to_loc(size), loc_to_index(size) {}
        unsigned size(void) const { return index_to_loc.size(); }
        int loc(int loc) const { return loc_to_index[loc]; }
        int idx(int idx) const { return index_to_loc[idx]; }
        void iotafy(void) {
                for (unsigned i = 0; i < index_to_loc.size(); i++) {
                        loc_to_index[i] = i;
                        index_to_loc[i] = i;
                }
        }

        void set(int loc, int idx) {
                abkassert(loc >= 0 && idx >= 0 && unsigned(loc) < index_to_loc.size() && unsigned(idx) < index_to_loc.size(), "Dave's error message goes here");
                index_to_loc[idx] = loc;
                loc_to_index[loc] = idx;
        }
};

#endif
