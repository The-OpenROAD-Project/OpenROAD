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


#ifndef PLSPTOBTREE_H
#define PLSPTOBTREE_H

#include "btree.h"

// --------------------------------------------------------
class PlSP2BTree
{
public:
   PlSP2BTree(const std::vector<float>& n_xloc,
              const std::vector<float>& n_yloc,
              const std::vector<float>& n_widths,
              const std::vector<float>& n_heights,
              const std::vector<int>& XX,
              const std::vector<int>& YY);

   PlSP2BTree(const std::vector<float>& n_xloc,
              const std::vector<float>& n_yloc,
              const std::vector<float>& n_widths,
              const std::vector<float>& n_heights,
              const std::vector<unsigned int>& XX,
              const std::vector<unsigned int>& YY);

   inline const std::vector<BTree::BTreeNode>& btree() const;
   inline const std::vector<int>& SP_XX() const;
   inline const std::vector<int>& SP_YY() const;
   inline const std::vector<int>& SP_XXinverse() const;
   inline const std::vector<int>& SP_YYinverse() const;
   
   const std::vector<float>& xloc;
   const std::vector<float>& yloc;
   const std::vector<float>& widths;
   const std::vector<float>& heights;

   inline bool SPleftof(int i, int j) const;
   inline bool SPrightof(int i , int j) const;
   inline bool SPabove(int i, int j) const;
   inline bool SPbelow(int i, int j) const;

   static const float Infty;
   static const int Undefined; // = BTree::Undefined;

private:
   int _blocknum;
   std::vector<int> _XX;
   std::vector<int> _YY;
   std::vector<int> _XXinverse;
   std::vector<int> _YYinverse;
   std::vector<BTree::BTreeNode> _btree;

   void constructor_core();
   void initializeTree();

   void build_tree();
   void build_tree_add_block(int currBlock, int otree_parent);
};
// --------------------------------------------------------

// ===============
// IMPLEMENTATIONS
// ===============
inline const std::vector<BTree::BTreeNode>& PlSP2BTree::btree() const
{   return _btree; }
// --------------------------------------------------------
inline const std::vector<int>& PlSP2BTree::SP_XX() const
{   return _XX; }
// --------------------------------------------------------
inline const std::vector<int>& PlSP2BTree::SP_YY() const
{   return _YY; }
// --------------------------------------------------------
inline const std::vector<int>& PlSP2BTree::SP_XXinverse() const
{   return _XXinverse; }
// --------------------------------------------------------
inline const std::vector<int>& PlSP2BTree::SP_YYinverse() const
{   return _YYinverse; }
// --------------------------------------------------------
inline bool PlSP2BTree::SPleftof(int i, int j) const
{   return (_XXinverse[i] < _XXinverse[j] &&
            _YYinverse[i] < _YYinverse[j]); }
// --------------------------------------------------------
inline bool PlSP2BTree::SPrightof(int i, int j) const
{   return SPleftof(j, i); }
// --------------------------------------------------------
inline bool PlSP2BTree::SPabove(int i, int j) const
{   return (_XXinverse[i] < _XXinverse[j] &&
            _YYinverse[i] > _YYinverse[j]); }
// --------------------------------------------------------
inline bool PlSP2BTree::SPbelow(int i, int j) const
{   return SPabove(j, i); }
// --------------------------------------------------------

#endif
