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


#ifndef BTREEFROMSSTREE_H
#define BTREEFROMSSTREE_H

//#include "datastrfrontsoftst.h"

#include "basepacking.h"
#include "btree.h"

#include <string>
#include <vector>

// --------------------------------------------------------
class SoftPackingHardBlockInfoType : public HardBlockInfoType
{
public:
   SoftPackingHardBlockInfoType(const SoftPacking& spk);
};
// --------------------------------------------------------
class BTreeFromSoftPacking : public BTree
{
public:
   BTreeFromSoftPacking(const HardBlockInfoType& nBlockinfo,
                        const SoftPacking& spk);
   BTreeFromSoftPacking(const HardBlockInfoType& nBlockinfo,
                        const SoftPacking& spk,
                        float nTolerance);   

protected:
   class SymbolicNodeType
   {
   public:
      SymbolicNodeType(int s, int BL, int TL, int R, float w)
         : sign(s), BL_block(BL), TL_block(TL), R_block(R), width(w) {}
      
      int sign;
      int BL_block;
      int TL_block;
      int R_block;
      float width;      
   };
   vector<SymbolicNodeType> in_buffer;
   void EvaluateTree(const SoftPacking& spk);
};
// --------------------------------------------------------

#endif
