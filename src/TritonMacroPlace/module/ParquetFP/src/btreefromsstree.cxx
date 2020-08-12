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


#include "btreefromsstree.h"

#include <fstream>
#include <vector>
using namespace basepacking_h;

// --------------------------------------------------------
SoftPackingHardBlockInfoType::SoftPackingHardBlockInfoType(
   const SoftPacking& spk)
   : HardBlockInfoType()
{
   int blocknum = spk.xloc.size();
   
   in_blocks.resize(blocknum+2);
   in_block_names.resize(blocknum+2);
   for (int i = 0; i < blocknum; i++)
   {
      set_dimensions(i, spk.width[i], spk.height[i]);

      char temp[100];
      sprintf(temp, "%d", i);
      in_block_names[i] = temp;
   }

   set_dimensions(blocknum, 0, Dimension::Infty);
   in_block_names[blocknum] = "LEFT";

   set_dimensions(blocknum+1, Dimension::Infty, 0);
   in_block_names[blocknum+1] = "BOTTOM";
}
// --------------------------------------------------------
BTreeFromSoftPacking::BTreeFromSoftPacking(const HardBlockInfoType& nBlockinfo,
                                           const SoftPacking& spk)
   : BTree(nBlockinfo)
{
   EvaluateTree(spk);
   evaluate(in_tree);
}
// --------------------------------------------------------
BTreeFromSoftPacking::BTreeFromSoftPacking(const HardBlockInfoType& nBlockinfo,
                                           const SoftPacking& spk,
                                           float nTolerance)
   : BTree(nBlockinfo, nTolerance)
{
   EvaluateTree(spk);
   evaluate(in_tree);
}
// --------------------------------------------------------
void BTreeFromSoftPacking::EvaluateTree(const SoftPacking& spk)
{
   int expr_size = spk.expression.size();
   int blocknum = spk.xloc.size();
   for (int i = 0; i < expr_size; i++)
   {
      int sign = spk.expression[i];
      if (SoftSTree::isOperand(sign))
      {
         in_buffer.push_back(SymbolicNodeType(sign, sign, sign, sign,
                                              spk.width[sign]));
         in_tree[sign].parent = blocknum;
         in_tree[sign].left = Undefined;
         in_tree[sign].right = Undefined;
         in_tree[sign].block_index = sign;
         in_tree[sign].orient = 0;
      }
      else
      {
         SymbolicNodeType TR_cluster(in_buffer.back());
         in_buffer.pop_back();
         
         SymbolicNodeType BL_cluster(in_buffer.back());
         in_buffer.pop_back();

         int new_BL_block = BL_cluster.BL_block;
         int new_TL_block = Undefined;
         int new_R_block = Undefined;
         float new_width = Undefined;
         if (sign == SoftSTree::STAR)
         {
            new_TL_block = BL_cluster.TL_block;
            new_R_block = TR_cluster.R_block;
            new_width = BL_cluster.width + TR_cluster.width;

            in_tree[TR_cluster.BL_block].parent = BL_cluster.R_block;
            in_tree[BL_cluster.R_block].left = TR_cluster.BL_block;
         }
         else if (sign == SoftSTree::PLUS)
         {            
            new_TL_block = TR_cluster.TL_block;
            new_R_block = (BL_cluster.width > TR_cluster.width)?
               BL_cluster.R_block : TR_cluster.R_block;
            new_width = max(BL_cluster.width, TR_cluster.width);
 
            in_tree[TR_cluster.BL_block].parent = BL_cluster.TL_block;
            in_tree[BL_cluster.TL_block].right = TR_cluster.BL_block;
         }
         else
         {
            cout << "ERROR in BTreeFromSoftPacking::EvaluateTree()" << endl;
            exit(1);
         }
         in_buffer.push_back(SymbolicNodeType(sign,
                                              new_BL_block, new_TL_block,
                                              new_R_block, new_width));
      }
   }
   in_tree[blocknum].left = spk.expression[0];
}
// --------------------------------------------------------

             
            
                             

   
   
     
