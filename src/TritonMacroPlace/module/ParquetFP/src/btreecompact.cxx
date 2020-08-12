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


#include "btreecompact.h"

#include <vector>

// --------------------------------------------------------
void BTreeCompactor::build_orth_tree()
{
   clean_contour(in_contour);
   clean_tree(in_orth_tree);
   
   int tree_prev = NUM_BLOCKS;
   int tree_curr = in_tree[NUM_BLOCKS].left; // start with first block
   while (tree_curr != NUM_BLOCKS) // until reach the root again
   {
      if (tree_prev == in_tree[tree_curr].parent)
      {
         build_orth_tree_add_block(tree_curr);
         tree_prev = tree_curr;
         if (in_tree[tree_curr].left != Undefined)
            tree_curr = in_tree[tree_curr].left;
         else if (in_tree[tree_curr].right != Undefined)
            tree_curr = in_tree[tree_curr].right;
         else
            tree_curr = in_tree[tree_curr].parent;
      }
      else if (tree_prev == in_tree[tree_curr].left)
      {
         tree_prev = tree_curr;
         if (in_tree[tree_curr].right != Undefined)
            tree_curr = in_tree[tree_curr].right;
         else
            tree_curr = in_tree[tree_curr].parent;
      }
      else
      {
         tree_prev = tree_curr;
         tree_curr = in_tree[tree_curr].parent;
      }
   }
   in_totalWidth = in_contour[NUM_BLOCKS+1].begin;

   int contour_ptr = in_contour[NUM_BLOCKS].next;
   in_totalHeight = 0;
   in_totalContourArea = 0;
   while (contour_ptr != NUM_BLOCKS+1)
   {
      in_totalHeight = std::max(in_totalHeight, in_contour[contour_ptr].CTL);
      in_totalContourArea += (in_contour[contour_ptr].end - in_contour[contour_ptr].begin)*in_contour[contour_ptr].CTL;
      contour_ptr = in_contour[contour_ptr].next;
   }
   in_totalArea = in_totalWidth * in_totalHeight;

   fix_orth_tree(in_orth_tree);
}
// --------------------------------------------------------
void BTreeCompactor::build_orth_tree_add_block(int tree_ptr)
{
   int tree_parent = in_tree[tree_ptr].parent;
   int orth_tree_parent = NUM_BLOCKS;
   
   float maxCTL = -1;
   int contour_ptr = Undefined;
   int contour_prev = Undefined;
   
   if (tree_ptr == in_tree[in_tree[tree_ptr].parent].left)
   {
      in_contour[tree_ptr].begin = in_contour[tree_parent].end;
      contour_ptr = in_contour[tree_parent].next;
   }
   else
   {
      in_contour[tree_ptr].begin = in_contour[tree_parent].begin;
      contour_ptr = tree_parent;
   }
   contour_prev = in_contour[contour_ptr].prev; // begins of cPtr/tPtr match
   orth_tree_parent = contour_ptr;       // initialize necessary
   maxCTL = in_contour[contour_ptr].CTL; // since width/height may be 0

   int block = in_tree[tree_ptr].block_index;
   int theta = in_tree[tree_ptr].orient;
   in_contour[tree_ptr].end =
      in_contour[tree_ptr].begin + in_blockinfo[block].width[theta];

   while (in_contour[contour_ptr].end <=
          in_contour[tree_ptr].end + TOLERANCE)
   {
      if (in_contour[contour_ptr].CTL > maxCTL)
      {
         maxCTL = in_contour[contour_ptr].CTL;
         orth_tree_parent = contour_ptr;
      }
      contour_ptr = in_contour[contour_ptr].next;
   }

   if (in_contour[contour_ptr].begin + TOLERANCE <
       in_contour[tree_ptr].end)
   {
      if (in_contour[contour_ptr].CTL > maxCTL)
      {
         maxCTL = in_contour[contour_ptr].CTL;
         orth_tree_parent = contour_ptr;
      }
   }
   
   in_xloc[tree_ptr] = in_contour[tree_ptr].begin;
   in_yloc[tree_ptr] = maxCTL;
   in_width[tree_ptr] = in_blockinfo[block].width[theta];
   in_height[tree_ptr] = in_blockinfo[block].height[theta];
   
//   printf("UpdateOrtho: %d: (%f %f)-(%f %f)\n", tree_ptr, in_xloc[tree_ptr], in_yloc[tree_ptr], 
//       in_xloc[tree_ptr]+ in_width[tree_ptr], 
//       in_yloc[tree_ptr]+ in_height[tree_ptr]);
       

   in_contour[tree_ptr].CTL =  maxCTL + in_blockinfo[block].height[theta];
   in_contour[tree_ptr].next = contour_ptr;
   in_contour[contour_ptr].prev = tree_ptr;
   in_contour[contour_ptr].begin = in_contour[tree_ptr].end;

   in_contour[tree_ptr].prev = contour_prev;
   in_contour[contour_prev].next = tree_ptr;
   in_contour[tree_ptr].begin = in_contour[contour_prev].end;

   // edit "in_orth_tree", the orthogonal tree
   if (in_orth_tree[orth_tree_parent].left == Undefined)
   {
      // left-child of the parent
      in_orth_tree[orth_tree_parent].left = tree_ptr;
      in_orth_tree[tree_ptr].parent = orth_tree_parent;
   }
   else
   {
      // sibling fo the left-child of the parent
      int orth_tree_ptr = in_orth_tree[orth_tree_parent].left;
      while (in_orth_tree[orth_tree_ptr].right != Undefined)
         orth_tree_ptr = in_orth_tree[orth_tree_ptr].right;

      in_orth_tree[orth_tree_ptr].right = tree_ptr;
      in_orth_tree[tree_ptr].parent = orth_tree_ptr;
   }
}
// --------------------------------------------------------

void BTreeCompactor::out_orth_dot(const std::string& file) const {
  std::filebuf fb;
  fb.open( file.c_str(), std::ios::out );
  std::ostream outs (&fb);

  outs << "digraph BST {" << std::endl;
  outs << "  graph [ordering=\"out\"];" << std::endl;

  int nullPtrCnt = 0; 
  for(int i=0; i< NUM_BLOCKS; i++) {
    outs << "  \"" << i << "\" -> ";
    if( in_orth_tree[i].left != Undefined) {
      outs << "\"" << in_orth_tree[i].left << "\";" << std::endl;
    }
    else {
      outs << "null" << std::to_string(nullPtrCnt) << std::endl;
      outs << "  null" << std::to_string(nullPtrCnt++) << " [shape=point];" << std::endl;
    } 

    outs << "  \"" << i << "\" -> ";
    if( in_orth_tree[i].right!= Undefined) {
      outs << "\"" << in_orth_tree[i].right<< "\";" << std::endl;
    }
    else {
      outs << "null" << std::to_string(nullPtrCnt) << std::endl;
      outs << "  null" << std::to_string(nullPtrCnt++) << " [shape=point];" << std::endl;
    } 
  } 
  outs << "}" << std::endl;
  fb.close();
}
void BTreeCompactor::out_orth_plot(const std::string& file) const {
  using std::endl;
  std::filebuf fb;
  fb.open( file.c_str(), std::ios::out );
  std::ostream outs (&fb);
  
  outs<<"#Use this file as a script for gnuplot"<<endl;
  outs<<"#(See http://www.gnuplot.info/ for details)"<<endl;
  outs << "set nokey"<<endl;

  outs << "set size ratio -1" << endl;
  outs << "set title ' " << file << endl << endl;
  
  outs <<"set terminal png size 1024,768" << endl;

  outs << "set xrange[" << 0 << ":" << in_totalWidth << "]" << endl;
  outs << "set yrange[" << 0 << ":" << in_totalHeight << "]" << endl;

  int objCnt = 0;
  float x = 0, y = 0, w = 0, h = 0;
  for(int i=0; i<NUM_BLOCKS; i++) {
    x = in_xloc[i];
    y = in_yloc[i];
    w = in_width[i];
    h = in_height[i];
    outs << "set object " << ++objCnt
      << " rect from " << x << "," << y << " to " << x+w << "," << y+h 
      << " fc rgb \"gold\"" << endl;
  }
  outs << "plot '-' w l" << endl;
  outs << "EOF"<<endl<<endl; 
  outs << "pause -1 'Press any key' "<<endl;
  fb.close();

}
