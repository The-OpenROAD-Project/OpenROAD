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


#ifndef BTREECOMPACT_H
#define BTREECOMPACT_H

#include "btree.h"

#include <algorithm>

// --------------------------------------------------------
class BTreeCompactor : public BTree
{
public:
   inline BTreeCompactor(const BTree& orig_tree);
   inline void operator =(const BTree& new_tree);
   inline void slimAssign(const BTree& new_tree); // suffices if evaluation
                                                  // follows
   
   inline int compact();
   void build_orth_tree();
   
   const std::vector<BTreeNode>& orth_tree;

   void out_orth_dot(const std::string& file) const;
   void out_orth_plot(const std::string& file) const;


private:
   std::vector<BTreeNode> in_orth_tree; // [NUM_BLOCKS] ~ in_tree[NUM_BLOCKS+1]
                                   // [NUM_BLOCKS+1] ~ in_tree[NUM_BLOCKS]

   void build_orth_tree_add_block(int treePtr);

   // (a) swap NUM_BLK vs. NUM_BLK+1
   // (b) fix parent of BL-block
   // used by build_orth_tree() only!!!
   inline static void fix_orth_tree(std::vector<BTreeNode>& orth_tree);
};
// --------------------------------------------------------

// =========================
//      IMPLEMENTATIONS
// =========================
BTreeCompactor::BTreeCompactor(const BTree& orig_tree)
   : BTree(orig_tree),
     orth_tree(in_orth_tree),
     in_orth_tree(orig_tree.tree.size())
{
   int vec_size = in_orth_tree.size();
   for (int i = 0; i < vec_size; i++)
   {
      in_orth_tree[i].parent = Undefined;
      in_orth_tree[i].left = Undefined;
      in_orth_tree[i].right = Undefined;
      in_orth_tree[i].block_index = orig_tree.tree[i].block_index;
      in_orth_tree[i].orient =
         OrientedPacking::flip(OrientedPacking::ORIENT(orig_tree.tree[i].orient));
   }
}
// --------------------------------------------------------
inline void BTreeCompactor::operator =(const BTree& new_tree)
{
   this->BTree::operator =(new_tree);
   for (unsigned int i = 0; i < in_orth_tree.size(); i++)
   {
      in_orth_tree[i].parent = Undefined;
      in_orth_tree[i].left = Undefined;
      in_orth_tree[i].right = Undefined;
      in_orth_tree[i].block_index = new_tree.tree[i].block_index;
      in_orth_tree[i].orient =
         OrientedPacking::flip(OrientedPacking::ORIENT(new_tree.tree[i].orient));
   }      
}
// --------------------------------------------------------
inline void BTreeCompactor::slimAssign(const BTree& new_tree)
{
   in_tree = new_tree.tree;
   in_blockArea = new_tree.blockArea();
   for (unsigned int i = 0; i < in_orth_tree.size(); i++)
   {
      in_orth_tree[i].parent = Undefined;
      in_orth_tree[i].left = Undefined;
      in_orth_tree[i].right = Undefined;
      in_orth_tree[i].block_index = new_tree.tree[i].block_index;
      in_orth_tree[i].orient =
         OrientedPacking::flip(OrientedPacking::ORIENT(new_tree.tree[i].orient));
   }      
}
// --------------------------------------------------------
inline int BTreeCompactor::compact()
{
	if (getNumObstacles()) 
	{
		// <aaronnn> if obstacles are present, don't allow compaction
		return 0;
	}

   std::vector<float> orig_xloc(in_xloc);
   std::vector<float> orig_yloc(in_yloc);

   build_orth_tree();
   swap_ranges(in_tree.begin(), in_tree.end(),
              in_orth_tree.begin());

 
//   save_dot("compact-1_orig.dot");
//   save_plot("compact-1_orig.plot");
//   out_orth_dot("compact-1_orth.dot");
//   out_orth_plot("compact-1_orth.plot"); 


//    cout << "-----here1: after build_orth_tree()-----" << endl;
//    cout << "in_tree: " << endl;
//    OutputBTree(cout, in_tree);
//    cout << endl << endl;
//    cout << "in_orth_tree:" << endl;
//    OutputBTree(cout, in_orth_tree);
//    cout << endl << endl;

   build_orth_tree();
   swap_ranges(in_tree.begin(), in_tree.end(),
              in_orth_tree.begin());
   
//   save_dot("compact-2_orig.dot");
//   save_plot("compact-2_orig.plot");
//   out_orth_dot("compact-2_orth.dot");
//   out_orth_plot("compact-2_orth.plot"); 
//    cout << "-----here2: after build_orth_tree()-----" << endl;
//    cout << "in_tree: " << endl;
//    OutputBTree(cout, in_tree);
//    cout << endl << endl;
//    cout << "in_orth_tree: " << endl;
//    OutputBTree(cout, in_orth_tree);
//    cout << endl << endl;
//    cout << "-----flipped packing-----" << endl;
//    OutputPacking(cout, BTreeOrientedPacking(*this));

//    ofstream outfile;
//    outfile.open("dummy_flipped");
//    Save_bbb(outfile, BTreeOrientedPacking(*this));
//    outfile.close();

   build_orth_tree();
   
//   save_dot("compact-3_orig.dot");
//   save_plot("compact-3_orig.plot");
//   out_orth_dot("compact-3_orth.dot");
//   out_orth_plot("compact-3_orth.plot"); 

//     exit(1);

   int count = 0;
   for (int i = 0; i < NUM_BLOCKS; i++)
   {
      if ((orig_xloc[i] != in_xloc[i]) ||
          (orig_yloc[i] != in_yloc[i]))
         count ++;
   }
   return count;
}
// --------------------------------------------------------
inline void BTreeCompactor::fix_orth_tree(std::vector<BTree::BTreeNode>& orth_tree)
{   
   // fix the tree s.t. in_tree[NUM_BLOCKS] corresponds to the root
   const int NUM_BLOCKS = orth_tree.size() - 2;
   int bottomLeftBlock = orth_tree[NUM_BLOCKS+1].left;
   
   orth_tree[bottomLeftBlock].parent = NUM_BLOCKS;
   std::iter_swap(&(orth_tree[NUM_BLOCKS]), &(orth_tree[NUM_BLOCKS+1]));
}
// --------------------------------------------------------
#endif
