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


#ifndef PLTOBTREE_H
#define PLTOBTREE_H

#include "basepacking.h"
#include "btree.h"

// --------------------------------------------------------
class Pl2BTree
{
public:
   enum AlgoType {HEURISTIC, TCG};

   inline Pl2BTree(const std::vector<float>& n_xloc,
                   const std::vector<float>& n_yloc,
                   const std::vector<float>& n_widths,
                   const std::vector<float>& n_heights,
                   AlgoType algo);

   inline const std::vector<BTree::BTreeNode>& btree() const;
   inline const std::vector<int>& getXX() const;
   inline const std::vector<int>& getYY() const;
   
   static const float Infty;
   static const int Undefined; // = basepacking_h::Dimension::Undefined;
   static const float Epsilon_Accuracy; // = basepacking_h::Dimension::Epsilon_Accuracy;
   
private:
   const std::vector<float>& _xloc;
   const std::vector<float>& _yloc;
   const std::vector<float>& _widths;
   const std::vector<float>& _heights;
   const int _blocknum;
   const float _epsilon;

   std::vector<BTree::BTreeNode> _btree;

   inline void initializeTree();
   inline float get_epsilon() const;

   // heuristic O(n^2) algo, usu. works for compacted packings
   class BuildTreeRecord
   {
   public:
      int parent;
      int treeLocIndex;
      float distance;
      float yDistance;

      float xStart;
      float xEnd;
      float yStart;
      float yEnd;
      bool used;

      bool operator <(const BuildTreeRecord& btr) const;      

      static float _epsilon;
      static float getDistance(const BuildTreeRecord& btr1,
                                const BuildTreeRecord& btr2);
   };
   
   class ValidCriterion // only compare relevant elements
   {
   public:
      inline ValidCriterion(const std::vector<BuildTreeRecord>& new_btr_vec);
      bool operator ()(const BuildTreeRecord& btr1,
                       const BuildTreeRecord& btr2) const;
      
      const std::vector<BuildTreeRecord>& btr_vec;
   };
   
   void heuristic_build_tree();   
   void build_tree_add_block(const BuildTreeRecord& btr);
   
   // tcg-based algo, always work
   int _count;
   std::vector<int> _XX;
   std::vector<int> _YY;
   void TCG_build_tree();

   // DP to find TCG
   void TCG_DP(std::vector< std::vector <bool> >& TCGMatrix); 
   void TCGDfs(std::vector< std::vector <bool> >& TCGMatrix, 
               const std::vector< std::vector <bool> >& adjMatrix,
               int v, 
               std::vector<int>& pre);

   class SPXRelation
   {
   public:
      SPXRelation(const std::vector< std::vector<bool> >& TCGMatrixHorizIP, 
                  const std::vector< std::vector<bool> >& TCGMatrixVertIP)
         : TCGMatrixHoriz(TCGMatrixHorizIP),
           TCGMatrixVert(TCGMatrixVertIP)
         {}

      inline bool operator ()(int i, int j) const;
         
   private:
      const std::vector< std::vector<bool> >& TCGMatrixHoriz;
      const std::vector< std::vector<bool> >& TCGMatrixVert;      
   };

   class SPYRelation
   {
   public:
      SPYRelation(const std::vector< std::vector<bool> >& TCGMatrixHorizIP, 
                  const std::vector< std::vector<bool> >& TCGMatrixVertIP)
         : TCGMatrixHoriz(TCGMatrixHorizIP),
           TCGMatrixVert(TCGMatrixVertIP)
         {}
      
      inline bool operator ()(int i, int j) const;

   private:
      const std::vector< std::vector<bool> >& TCGMatrixHoriz;
      const std::vector< std::vector<bool> >& TCGMatrixVert;      
   };
};
// --------------------------------------------------------

// ===============
// IMPLEMENTATIONS
// ===============
Pl2BTree::Pl2BTree(const std::vector<float>& n_xloc,
                   const std::vector<float>& n_yloc,
                   const std::vector<float>& n_widths,
                   const std::vector<float>& n_heights,
                   Pl2BTree::AlgoType algo)
   : _xloc(n_xloc),
     _yloc(n_yloc),
     _widths(n_widths),
     _heights(n_heights),
     _blocknum(n_xloc.size()),
     _epsilon(get_epsilon()),
     _btree(n_xloc.size()+2),
     _count(Undefined),
     _XX(_blocknum, Undefined),
     _YY(_blocknum, Undefined)
{
   initializeTree();
   switch (algo)
   {
   case HEURISTIC:
      heuristic_build_tree();
      break;

   case TCG:
      TCG_build_tree();
      break;
      
   default:
      std::cout << "ERROR: invalid algorithm specified for Pl2BTree()." << std::endl;
      exit(1);
   }
}
// --------------------------------------------------------
void Pl2BTree::initializeTree()
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
float Pl2BTree::get_epsilon() const
{
   float ep = Infty;
   for (int i = 0; i < _blocknum; i++)
      ep = std::min(ep, std::min(_widths[i], _heights[i]));
   ep /= Epsilon_Accuracy;
   return ep;
}
// --------------------------------------------------------
const std::vector<BTree::BTreeNode>& Pl2BTree::btree() const
{   return _btree; }
// --------------------------------------------------------
const std::vector<int>& Pl2BTree::getXX() const
{   return _XX; }
// --------------------------------------------------------
const std::vector<int>& Pl2BTree::getYY() const
{   return _YY; }
// --------------------------------------------------------
Pl2BTree::ValidCriterion::ValidCriterion(
   const std::vector<BuildTreeRecord>& new_btr_vec)
   : btr_vec(new_btr_vec)
{}
// --------------------------------------------------------
bool Pl2BTree::SPXRelation::operator ()(int i, int j) const
{
   if (TCGMatrixHoriz[i][j])
      return true;
   else if (TCGMatrixHoriz[j][i])
      return false;
   else if (TCGMatrixVert[j][i])
      return true;
   else if (TCGMatrixVert[i][j])
      return false;
   else
      return i < j;
}
// --------------------------------------------------------
bool Pl2BTree::SPYRelation::operator ()(int i, int j) const
{
   if (TCGMatrixHoriz[i][j])
      return true;
   else if (TCGMatrixHoriz[j][i])
      return false;
   else if (TCGMatrixVert[j][i])
      return false;
   else if (TCGMatrixVert[i][j])
      return true;
   else
      return i < j;
}
// --------------------------------------------------------

#endif
