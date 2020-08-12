/**************************************************************************
***    
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
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
#ifndef BTREECOMPACTSSTREE_H
#define BTREECOMPACTSSTREE_H

#include "btreecompact.h"
#include "btreefromsstree.h"
#include "basepacking.h"

#include <string>
#include <cfloat>
#include <algorithm>

const float DEFAULT_SIDE_ACCURACY = 10000000;
// --------------------------------------------------------
float BTreeCompactSlice(const SoftPacking& spk,
                         const string& outfilename);
inline float getTolerance(const HardBlockInfoType& blockinfo);
// --------------------------------------------------------

// ========================
//      IMPLEMENTATION
// ========================
inline float getTolerance(const HardBlockInfoType& blockinfo)
{
   float min_side = FLT_MAX;
   for (int i = 0; i < blockinfo.blocknum(); i++)
      min_side = min(min_side,
                     min(blockinfo[i].width[0], blockinfo[i].height[0]));

   return min_side / DEFAULT_SIDE_ACCURACY;
}
// --------------------------------------------------------

#endif
