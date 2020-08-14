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


#include "pltobtree.h"
#include "plsptobtree.h"
#include "debug.h"

#include <cmath>
#include <vector>
#include <list>
#include <algorithm>
#include <cfloat>

using std::cout;
using std::endl;
using std::vector;

const float Pl2BTree::Infty = basepacking_h::Dimension::Infty;
const int Pl2BTree::Undefined = basepacking_h::Dimension::Undefined;
const float Pl2BTree::Epsilon_Accuracy = basepacking_h::Dimension::Epsilon_Accuracy;
float Pl2BTree::BuildTreeRecord::_epsilon = 0;
// --------------------------------------------------------
void Pl2BTree::heuristic_build_tree()
{
   BuildTreeRecord::_epsilon = _epsilon;

   // -----initialize the list-----
   vector<BuildTreeRecord> treeRecord(_blocknum+1);
   for (int i = 0; i < _blocknum; i++)
   {
      treeRecord[i].parent = _blocknum;      // set as the LEFT-edge initially
      treeRecord[i].treeLocIndex = i;
      treeRecord[i].distance = _xloc[i]; // set as its xloc initially

      treeRecord[i].xStart = _xloc[i];
      treeRecord[i].xEnd = _xloc[i] + _widths[i];
      treeRecord[i].yStart = _yloc[i];
      treeRecord[i].yEnd = _yloc[i] + _heights[i];
      treeRecord[i].used = false;      
   }
   treeRecord[_blocknum].parent = Undefined;
   treeRecord[_blocknum].treeLocIndex = _blocknum;
   treeRecord[_blocknum].distance = 0;

   treeRecord[_blocknum].xStart = 0;
   treeRecord[_blocknum].xEnd = 0;
   treeRecord[_blocknum].yStart = 0;
   treeRecord[_blocknum].yEnd = Infty;
   treeRecord[_blocknum].used = true;

   // determine the real (otree) parent of each node, maybe unchanged
   for (int i = 0; i < _blocknum; i++)
   {
      float minDistance = treeRecord[i].distance;
      for (int j = 0; j < _blocknum; j++)
      {
         if (i != j)
         {
            // if overlap in y-span occurs (compact to the left)
            if (treeRecord[i].yEnd > treeRecord[j].yStart + _epsilon &&
                treeRecord[j].yEnd > treeRecord[i].yStart + _epsilon)
            {
               float tempDistance =
                  BuildTreeRecord::getDistance(treeRecord[i],
                                               treeRecord[j]);

               // break ties
               if (tempDistance < minDistance)
               {
                  treeRecord[i].parent = j;
                  minDistance = tempDistance;
               }
            }
         }
      }
   }
   
   for (int treeNodeCount = 0; treeNodeCount < _blocknum; treeNodeCount++)
   {
      // -----add a block to the B*-Tree-----
      vector<BuildTreeRecord>::iterator min_ptr =
         min_element(treeRecord.begin(),
                     treeRecord.end(),
                     ValidCriterion(treeRecord));

      build_tree_add_block(*min_ptr);
      min_ptr->used = true;

      // -----update the rest of the list-----
      int currBlock = min_ptr->treeLocIndex;      
      for (int tempBlock = 0; tempBlock < _blocknum; tempBlock++)
         if (!treeRecord[tempBlock].used)
         {
            // determine the distance of removed block from this block
            float tempDistance =
               BuildTreeRecord::getDistance(treeRecord[tempBlock],
                                            treeRecord[currBlock]);
            
            // determine whether to update and update if necessary
            treeRecord[tempBlock].distance =
               std::min(treeRecord[tempBlock].distance, tempDistance);
         }

//       OutputBTree(cout, _btree);
//       cin.get();
   } // end while (!treeRecord.empty)
}
// --------------------------------------------------------
void Pl2BTree::build_tree_add_block(const Pl2BTree::BuildTreeRecord& btr)
{
//    printf("block %d added with block %d as its otree-parent\n",
//           btr.treeLocIndex, btr.parent);
   
   int currBlock = btr.treeLocIndex;
   int otree_parent = btr.parent;
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
             (_yloc[tree_curr] < _yloc[currBlock]))
      {
         tree_prev = tree_curr;
         tree_curr = _btree[tree_curr].right;
      }

//       printf("tree_curr: %d tree_prev: %d\n", tree_curr, tree_prev);
//       printf("YLOC: tree_curr: %.2lf currBlock: %.2lf tree_prev: %.2lf\n",
//              (tree_curr != Undefined)? _yloc[tree_curr] : -1,
//              _yloc[currBlock], _yloc[tree_prev]);
      
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
float Pl2BTree::BuildTreeRecord::getDistance(const BuildTreeRecord& btr1, 
                                              const BuildTreeRecord& btr2) 
{
   float tempDistance = btr1.xStart - btr2.xEnd;// tempXlocStart - currXlocEnd;

   if (btr1.yEnd < btr2.yStart + _epsilon || // only count if y-spans overlap
       btr2.yEnd < btr1.yStart + _epsilon)
      tempDistance = Infty;
   
   else if (tempDistance < -1*_epsilon) // horizontal overlap? not necessary
   {
      if (btr2.xStart < btr1.xEnd - _epsilon) // horizontal overlap
      {
         if ((btr1.yEnd > btr2.yStart + _epsilon) &&
             (btr1.yStart < btr2.yEnd - _epsilon))
            tempDistance = 0;      // real overlap, push it horizontally
         else
            tempDistance = Infty;  // no overlap, treat it as no-edge
      }
      else
         tempDistance = Infty;
   }
   
   return std::max(tempDistance, float(0.0));
}
// --------------------------------------------------------
bool Pl2BTree::BuildTreeRecord::operator <(const BuildTreeRecord& btr) const
{
   float difference = distance - btr.distance;
   if (std::abs(difference) < _epsilon)
   {
      if (xEnd < btr.xStart + _epsilon) // break ties by x-span (L > R)
         return true;
      else if (btr.xEnd < xStart + _epsilon)
         return false;
         
      if (yEnd < btr.yStart + _epsilon) // break ties by y-span (T > B)
         return true;
      else if (btr.yEnd < yStart + _epsilon)
         return false;
      else
         return false;                  // two blocks overlap, whatever
   }
   else
      return difference < 0;
}
// --------------------------------------------------------
bool Pl2BTree::ValidCriterion::operator ()(const BuildTreeRecord& btr1,
                                           const BuildTreeRecord& btr2) const
{
   if (btr1.used)
      return false;
   
   else if (btr2.used) // btr1 is not used
      return true;
      
   else if (!btr_vec[btr1.parent].used)
      return false;

   else if (!btr_vec[btr2.parent].used) // parent of btr1 is used
      return true;
   
   else
      return btr1 < btr2;
}
// --------------------------------------------------------
void Pl2BTree::TCG_build_tree()
{
   vector< vector<bool> > TCGMatrixVert(_blocknum, vector<bool>(_blocknum, false));
   vector< vector<bool> > TCGMatrixHoriz(TCGMatrixVert);

   //set up the immediate constraints
   for(int i = 0; i < _blocknum; ++i)
   {
      for(int j = 0; j <= i; ++j)
      {
         float horizOverlap = 0;
         float vertOverlap = 0;
         unsigned vertOverlapDir = 0;
         unsigned horizOverlapDir = 0;
                  
         if (i == j)
         {
            TCGMatrixHoriz[i][j] = true;
            TCGMatrixVert[i][j] = true;
            continue;
         }

         // i != j
         TCGMatrixHoriz[i][j] = false;
         TCGMatrixVert[i][j] = false;
         TCGMatrixHoriz[j][i] = false;
         TCGMatrixVert[j][i] = false;
         
         float iXStart = _xloc[i];
         float iXEnd = _xloc[i] + _widths[i];
         float jXStart = _xloc[j];
	 float jXEnd = _xloc[j] + _widths[j];

	 float iYStart = _yloc[i];
	 float iYEnd = _yloc[i] + _heights[i];
	 float jYStart = _yloc[j];
	 float jYEnd = _yloc[j] + _heights[j];

         // horizontal constraint
         if (jYStart < iYEnd - _epsilon && iYStart < jYEnd - _epsilon)
         {         
            if (jYStart < iYStart && jYEnd < iYEnd)
            {
               vertOverlap = jYEnd - iYStart; // lower overlap (j lower i)
               vertOverlapDir = 0; 
            }
            else if (jYStart > iYStart) // (jYEnd <= iYEnd)
            {
               vertOverlap = jYEnd - jYStart; // inner overlap (j inner i)
               if (iYEnd-jYEnd > jYStart-iYStart)
                  vertOverlapDir = 0;
               else
                  vertOverlapDir = 1;
            }
            else if (jYEnd > iYEnd) // (jYStart <= iYstart)
            {
               vertOverlap = iYEnd - jYStart; // upper overlap (j upper i)
               vertOverlapDir = 1;
            }
            else // (jYStart <= iYStart && jYEnd > iYEnd)
            {
               vertOverlap = iYEnd - iYStart; // outer overlap (j outer i)
               if(jYEnd-iYEnd > iYStart-jYStart)
                  vertOverlapDir = 1;
               else
                  vertOverlapDir = 0;
            }
         }
         else
	    TCGMatrixHoriz[i][j] = false;  // no overlap         

         // vertical constraint
         if (jXStart < iXEnd - _epsilon && iXStart < jXEnd - _epsilon)
         {            
            if (jXStart < iXStart && jXEnd < iXEnd)
            {
               horizOverlap = jXEnd - iXStart; // left overlap (j left i)
               horizOverlapDir = 0; 
            }
            else if (jXEnd < iXEnd) // (jXStart >= iXStart) 
            {
               horizOverlap = jXEnd - jXStart; // inner overlap (j inner i)
               if (iXEnd-jXEnd > jXStart-iXStart)
                  horizOverlapDir = 0;
               else
                  horizOverlapDir = 1;
            }
            else if (jXStart > iXStart) // (jXEnd >= iXEnd)
            {
               horizOverlap = iXEnd - jXStart;  // right overlap (j right i)
               horizOverlapDir = 1;
            }
            else //  (jXStart < iXStart) && (jXEnd >= iXEnd) 
            {
               horizOverlap = iXEnd - iXStart;  // outer overlap (j out i)
               if (jXEnd-iXEnd  > iXStart-jXStart )
                  horizOverlapDir = 1;
               else
                  horizOverlapDir = 0;
            }
         }
         else
            TCGMatrixVert[i][j] = false;    // no overlap

         // determine edge in TCG's
         if (vertOverlap != 0 && horizOverlap == 0)
         {
            if (iXStart <= jXStart)
               TCGMatrixHoriz[i][j] = true;
            else
               TCGMatrixHoriz[j][i] = true;
         }
         else if (horizOverlap != 0 && vertOverlap == 0)
         {
            if (iYStart <= jYStart)
               TCGMatrixVert[i][j] = true;
            else
               TCGMatrixVert[j][i] = true;
         }
         // overlapping
         else if (horizOverlap != 0 && vertOverlap != 0)
         {
            if (vertOverlap >= horizOverlap)
            {
               if (horizOverlapDir == 1)
                  TCGMatrixHoriz[i][j] = true;
               else
                  TCGMatrixHoriz[j][i] = true;
            }
            else
            {
               if (vertOverlapDir == 1)
                  TCGMatrixVert[i][j] = true;
               else
                  TCGMatrixVert[j][i] = true;
            }
         }
      }
   }
    
  //floyd marshal to find transitive closure
  //TCG_FM(TCGMatrixHoriz, TCGMatrixVert);

  // dynamic programming DFS algo to find transitive closure
  TCG_DP(TCGMatrixHoriz);
  TCG_DP(TCGMatrixVert);

  // find ties and break them
  for(int i = 0; i < _blocknum; ++i)
  {
     for(int j = 0; j < _blocknum; ++j)
     {
        if (i == j)
           continue;
        
        if (TCGMatrixHoriz[i][j] && TCGMatrixHoriz[j][i])
        {
           cout << "ERROR in TCG 1 "<<i<<"\t"<<j<<endl;
        }
        
        if (TCGMatrixVert[i][j] && TCGMatrixVert[j][i])
        {
           cout<<"ERROR in TCG 2 "<<i<<"\t"<<j<<endl;

        }

        unsigned ctr = 0;
        if(TCGMatrixHoriz[i][j])
           ++ctr;
        if(TCGMatrixHoriz[j][i])
           ++ctr;
        if(TCGMatrixVert[i][j])
           ++ctr;
        if(TCGMatrixVert[j][i])
           ++ctr;

        if(ctr > 1)
        {
           unsigned dir = rand()%2;
           if(dir == 0) // H constraint
           {
              TCGMatrixVert[i][j] = false;
              TCGMatrixVert[j][i] = false;
           }
           else // V constraint
           {
              TCGMatrixHoriz[i][j] = false;
              TCGMatrixHoriz[j][i] = false;
           }
        }
	  
        // no constraint between the blocks
        if (!TCGMatrixHoriz[i][j] && !TCGMatrixHoriz[j][i] &&
            !TCGMatrixVert[i][j] && !TCGMatrixVert[j][i])
        {
           TCGMatrixHoriz[i][j] = (_xloc[i] < _xloc[j]);
        }
     }
  }
  
  // get the sequence pair now
  for(int i = 0; i < _blocknum; ++i)
  {
     _XX[i] = i;
     _YY[i] = i;
  }  
  sort(_XX.begin(), _XX.end(), SPXRelation(TCGMatrixHoriz, TCGMatrixVert));
  sort(_YY.begin(), _YY.end(), SPYRelation(TCGMatrixHoriz, TCGMatrixVert));

  PlSP2BTree plsp2btree(_xloc, _yloc, _widths, _heights, _XX, _YY);
  _btree = plsp2btree.btree();
}
// --------------------------------------------------------
void Pl2BTree::TCG_DP(vector< vector <bool> >& TCGMatrix)
{
   vector< vector <bool> > adjMatrix(TCGMatrix);
   vector<int> pre(_blocknum, Undefined);
   
   for(int i = 0; i < _blocknum; ++i)
      fill(TCGMatrix[i].begin(), TCGMatrix[i].end(), false);
   
   _count = 0;
   for(int i = 0; i < _blocknum; ++i)
      if(pre[i] == -1)
         TCGDfs(TCGMatrix, adjMatrix, i, pre);
}
// --------------------------------------------------------
void Pl2BTree::TCGDfs(vector< vector <bool> >& TCGMatrix, 
                      const vector< vector <bool> >& adjMatrix,
                      int v,
                      vector<int>& pre)
{
  pre[v] = _count;
  _count++;
  for (int u = 0; u < _blocknum; ++u)
  {
     if (adjMatrix[v][u])
     {
        TCGMatrix[v][u] = true;
        if (pre[u] > pre[v]) // taken care of already
           continue;
         
        if (pre[u] == Undefined)
           TCGDfs(TCGMatrix, adjMatrix, u, pre);
         
        for (int i = 0; i < _blocknum; ++i)
           if (TCGMatrix[u][i])
              TCGMatrix[v][i] = true;
     }
  }
}
// --------------------------------------------------------
