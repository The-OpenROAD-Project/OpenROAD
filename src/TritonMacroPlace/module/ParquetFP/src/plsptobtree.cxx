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


#include "plsptobtree.h"
#include "btree.h"
#include <cmath>
#include <cfloat>
#include <algorithm>

using std::vector;

const float PlSP2BTree::Infty = basepacking_h::Dimension::Infty;
const int PlSP2BTree::Undefined = BTree::Undefined;
// --------------------------------------------------------
PlSP2BTree::PlSP2BTree(const vector<float>& n_xloc,
                       const vector<float>& n_yloc,
                       const vector<float>& n_widths,
                       const vector<float>& n_heights,
                       const vector<int>& XX,
                       const vector<int>& YY)
   : xloc(n_xloc),
     yloc(n_yloc),
     widths(n_widths),
     heights(n_heights),
     _blocknum(n_xloc.size()),
     _XX(XX),
     _YY(YY),
     _XXinverse(_blocknum),
     _YYinverse(_blocknum),
     _btree(_blocknum+2)
{
   constructor_core();
}
// --------------------------------------------------------
PlSP2BTree::PlSP2BTree(const vector<float>& n_xloc,
                       const vector<float>& n_yloc,
                       const vector<float>& n_widths,
                       const vector<float>& n_heights,
                       const vector<unsigned int>& XX,
                       const vector<unsigned int>& YY)
   : xloc(n_xloc),
     yloc(n_yloc),
     widths(n_widths),
     heights(n_heights),
     _blocknum(n_xloc.size()),
     _XX(_blocknum),
     _YY(_blocknum),
     _XXinverse(_blocknum),
     _YYinverse(_blocknum),
     _btree(_blocknum+2)
{
   for (int i = 0; i < _blocknum; i++)
   {
      _XX[i] = int(XX[i]);
      _YY[i] = int(YY[i]);
   }
   constructor_core();
}
// --------------------------------------------------------
void PlSP2BTree::build_tree()
{
   for (int i = 0; i < _blocknum; i++)
   {
      int currBlock = _YY[i];
      int otree_parent = _blocknum; // left-edge initially

      float currXStart = xloc[currBlock];
      float minDistance = currXStart;
      int parentMaxIndex = _blocknum;
      for (int j = i-1; j >= 0; j--)
      {
         int tempBlock = _YY[j];
         if (SPleftof(tempBlock, currBlock))
         {
            if (_XXinverse[tempBlock] < parentMaxIndex)
            {
               float tempDistance =
                  currXStart - xloc[tempBlock] - widths[tempBlock];
               if (tempDistance < minDistance)
               {
                  minDistance = tempDistance;
                  otree_parent = tempBlock;
               }
            }
         }
         else // SPbelow(tempBlock, currBlock)
            parentMaxIndex = std::min(parentMaxIndex, _XXinverse[tempBlock]);
      }
      build_tree_add_block(currBlock, otree_parent);
   }
}
// --------------------------------------------------------
void PlSP2BTree::build_tree_add_block(int currBlock,
                                      int otree_parent)
{
//    printf("block %d added with block %d as its otree-parent\n",
//           currBlock, otree_parent);
   
   if (_btree[otree_parent].left == Undefined)
   {
      _btree[otree_parent].left = currBlock;
      _btree[currBlock].parent = otree_parent;
   }
   else
   {
      int tree_prev = otree_parent;
      int tree_curr = _btree[otree_parent].left;
      while ((tree_curr != Undefined) &&
             (yloc[tree_curr] < yloc[currBlock]))
      {
         tree_prev = tree_curr;
         tree_curr = _btree[tree_curr].right;
      }

//       printf("tree_curr: %d tree_prev: %d\n", tree_curr, tree_prev);
//       printf("YLOC: tree_curr: %.2lf currBlock: %.2lf tree_prev: %.2lf\n",
//              (tree_curr != Undefined)? yloc[tree_curr] : -1,
//              yloc[currBlock], yloc[tree_prev]);
      
      if ((tree_curr != Undefined) &&
          (tree_curr == _btree[tree_prev].left))
         _btree[tree_prev].left = currBlock;
      else
         _btree[tree_prev].right = currBlock;
      _btree[currBlock].parent = tree_prev;

      _btree[currBlock].right = tree_curr; // possibly Undefined
      if (tree_curr != Undefined)
         _btree[tree_curr].parent = currBlock;
   }
}
// --------------------------------------------------------
void PlSP2BTree::constructor_core()
{
   initializeTree();
   for (int i = 0; i < _blocknum; i++)
   {
      _XXinverse[_XX[i]] = i;
      _YYinverse[_YY[i]] = i;
   }
   build_tree();
}
// --------------------------------------------------------
void PlSP2BTree::initializeTree()
{
   int vec_size = int(_btree.size());
   for (int i = 0; i < vec_size; i++)
   {
      _btree[i].parent = _blocknum;
      _btree[i].left = Undefined;
      _btree[i].right = Undefined;
      _btree[i].block_index = i;
      _btree[i].orient = 0;
   }

   _btree[_blocknum].parent = Undefined;
   _btree[_blocknum].left = Undefined;
   _btree[_blocknum].right = Undefined;
   _btree[_blocknum].block_index = _blocknum;
   _btree[_blocknum].orient = Undefined;

   _btree[_blocknum+1].parent = _blocknum;
   _btree[_blocknum+1].left = Undefined;
   _btree[_blocknum+1].right = Undefined;
   _btree[_blocknum+1].block_index = _blocknum+1;
   _btree[_blocknum+1].orient = Undefined;
}
// --------------------------------------------------------

   

