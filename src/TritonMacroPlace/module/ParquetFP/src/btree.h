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


#ifndef BTREE_H
#define BTREE_H

#include "basepacking.h"

#include <string>
#include <fstream>
#include <cmath>

// --------------------------------------------------------
class BTree
{
public:
   BTree(const HardBlockInfoType& blockinfo);
   BTree(const HardBlockInfoType& blockinfo, float nTolerance);
   inline BTree(const BTree& newBTree);
   inline bool operator =(const BTree& newBTree); // true if succeeds

   void addObstacles(BasePacking &obstacles, float obstacleFrame[2]); // <aaronnn> give btree obstacles
   inline unsigned getNumObstacles();
   // <aaronnn> change the frame of reference for packing (used when comparing with fixed objects like obstacles)
   enum PackOrigin {PACK_BOTTOM, PACK_LEFT, PACK_RIGHT, PACK_TOP};
   void set_pack_origin(PackOrigin new_pack_origin) { pack_origin = new_pack_origin; } 

   class BTreeNode;
   class ContourNode;
   const std::vector<BTreeNode>& tree;
   const std::vector<ContourNode>& contour;

   inline const std::vector<float>& xloc() const;
   inline const std::vector<float>& yloc() const;
   inline float xloc(int index) const;
   inline float yloc(int index) const;
   inline float width(int index) const;
   inline float height(int index) const;
   
   inline float blockArea() const;
   inline float totalArea() const;
   inline float totalWidth() const;
   inline float totalHeight() const;
   inline float totalContourArea() const;
   inline float getDistance(float x, float y) const;

   inline void setTree(const std::vector<BTreeNode>& ntree);
   void evaluate(const std::vector<BTreeNode>& ntree); 
   void evaluate(const std::vector<int>& tree_bits,    // assume the lengths of
                 const std::vector<int>& perm,         // these 3 are compatible 
                 const std::vector<int>& orient);      // with size of old tree
   static void bits2tree(const std::vector<int>& tree_bits, // assume lengths of 
                         const std::vector<int>& perm,      // these 3 compatible
                         const std::vector<int>& orient,
                         std::vector<BTreeNode>& ntree);
   inline static void clean_tree(std::vector<BTreeNode>& otree);
   inline void clean_contour(std::vector<ContourNode>& oContour);

   // perturb the tree, evaluate contour from scratch
   enum MoveType {SWAP, ROTATE, MOVE};
   
   void swap(int indexOne, int indexTwo); 
   inline void rotate(int index, int newOrient);
   void move(int index, int target, bool leftChild); // target = 0..n

   const int NUM_BLOCKS;
   static const int Undefined; // = basepacking_h::Dimension::Undefined;

   class BTreeNode
   {
   public:
      int parent;
      int left;
      int right;
      
      int block_index;
      int orient;
   };

   class ContourNode
   {
   public:
      int next;
      int prev;
      
      float begin;
      float end;
      float CTL;
   };

   // -----output functions-----
   void save_bbb(const std::string& filename) const;
   void save_dot(const std::string& filename) const;
   void save_plot(const std::string& filename) const;
   
protected:
   const HardBlockInfoType& in_blockinfo;
   std::vector<BTreeNode> in_tree;
   std::vector<ContourNode> in_contour;

   // <aaronnn> obstacle handling
   BasePacking in_obstacles;
   float in_obstacleframe[2]; // 0=width, 1=height
   std::vector<bool> seen_obstacles; // track obstacles that have been consumed during contour_evaluate
   float new_block_x_shift; // x shift for avoiding obstacles when adding new block 
   float new_block_y_shift; // y shift for avoiding obstacles when adding new block
   void transform_bbox_wrt_pack_origin(BTree::PackOrigin packOrigin, 
      float frameWidth, float frameHeight, float &xMin, float &yMin, float &xMax, float &yMax);

   // blah[i] refers the attribute of in_tree[i].
   std::vector<float> in_xloc;
   std::vector<float> in_yloc;
   std::vector<float> in_width;
   std::vector<float> in_height;

   float in_blockArea;
   float in_totalArea;
   float in_totalWidth;
   float in_totalHeight;
   float in_totalContourArea;

   const float TOLERANCE;

   PackOrigin pack_origin; // frame of reference for btree's direction of growth

   void contour_evaluate();
   void contour_add_block(int treePtr);
   void find_new_block_location(const int tree_ptr, float &out_x, float &out_y, int&, int&);
   // <aaronnn> handle obstacles during contour_evaluate
   bool contour_new_block_intersects_obstacle(const int tree_ptr, unsigned &obstacleID,
      float &new_xMin, float &new_xMax, float &new_yMin, float &new_yMax,
      float &obstacle_xMin, float &obstacle_xMax, float &obstacle_yMin, float &obstacle_yMax);
   void contour_add_obstacle(int tree_ptr, const int obstacleID);

   void swap_parent_child(int parent, bool isLeft);
   void remove_left_up_right_down(int index);
};   
// --------------------------------------------------------
class BTreeOrientedPacking : public OrientedPacking
{
public:
   inline BTreeOrientedPacking() {}
   inline BTreeOrientedPacking(const BTree& bt);

   inline void operator =(const BTree& bt);
};
// --------------------------------------------------------

// =========================
//      Implementations
// =========================
inline BTree::BTree(const BTree& newBtree)
   : tree(in_tree),
     contour(in_contour),
     NUM_BLOCKS(newBtree.NUM_BLOCKS),
     in_blockinfo(newBtree.in_blockinfo),
     in_tree(newBtree.in_tree),
     in_contour(newBtree.in_contour),
     
     in_xloc(newBtree.in_xloc),
     in_yloc(newBtree.in_yloc),
     in_width(newBtree.in_width),
     in_height(newBtree.in_height),
     
     in_blockArea(newBtree.in_blockArea),
     in_totalArea(newBtree.in_totalArea),
     in_totalWidth(newBtree.in_totalWidth),
     in_totalHeight(newBtree.in_totalHeight),

     TOLERANCE(newBtree.TOLERANCE),

	 pack_origin(newBtree.pack_origin)
{
     in_obstacles.xloc = newBtree.in_obstacles.xloc;
     in_obstacles.yloc = newBtree.in_obstacles.yloc;
     in_obstacles.width = newBtree.in_obstacles.width;
     in_obstacles.height = newBtree.in_obstacles.height;
	 in_obstacleframe[0] = newBtree.in_obstacleframe[0];
	 in_obstacleframe[1] = newBtree.in_obstacleframe[1];
}
// --------------------------------------------------------
inline bool BTree::operator =(const BTree& newBtree)
{
   if (NUM_BLOCKS == newBtree.NUM_BLOCKS)
   {
      in_tree = newBtree.in_tree;
      in_contour = newBtree.in_contour;

      in_xloc = newBtree.in_xloc;
      in_yloc = newBtree.in_yloc;
      in_width = newBtree.in_width;
      in_height = newBtree.in_height;

      in_blockArea = newBtree.in_blockArea;
      in_totalArea = newBtree.in_totalArea;
      in_totalWidth = newBtree.in_totalWidth;
      in_totalHeight = newBtree.in_totalHeight;

	  in_obstacles.xloc = newBtree.in_obstacles.xloc;
	  in_obstacles.yloc = newBtree.in_obstacles.yloc;
	  in_obstacles.width = newBtree.in_obstacles.width;
	  in_obstacles.height = newBtree.in_obstacles.height;
	  in_obstacleframe[0] = newBtree.in_obstacleframe[0];
	  in_obstacleframe[1] = newBtree.in_obstacleframe[1];

      pack_origin = newBtree.pack_origin;
      
      return true;
   }
   else
      return false;
}
// --------------------------------------------------------
inline const std::vector<float>& BTree::xloc() const
{  return in_xloc; }
// --------------------------------------------------------
inline const std::vector<float>& BTree::yloc() const
{  return in_yloc; }
// --------------------------------------------------------
inline float BTree::xloc(int index) const
{  return in_xloc[index]; }
// --------------------------------------------------------
inline float BTree::yloc(int index) const
{  return in_yloc[index]; }
// --------------------------------------------------------
inline float BTree::width(int index) const
{  return in_width[index]; }
// --------------------------------------------------------
inline float BTree::height(int index) const
{  return in_height[index]; }
// --------------------------------------------------------
inline float BTree::blockArea() const
{  return in_blockArea; }
// --------------------------------------------------------
inline float BTree::totalArea() const
{  return in_totalArea; }
// --------------------------------------------------------
inline float BTree::totalWidth() const
{  return in_totalWidth; }
// --------------------------------------------------------
inline float BTree::totalHeight() const
{  return in_totalHeight; }
// --------------------------------------------------------
inline float BTree::totalContourArea() const
{  return in_totalContourArea; }
// --------------------------------------------------------
inline float BTree::getDistance(float x, float y) const
{  
  float retVal = 0.0f;
  for(int i=0; i<NUM_BLOCKS; i++) {
//    float urx = in_xloc[i] + in_width[i];
//    float ury = in_yloc[i] + in_height[i];
    float urx = in_xloc[i] + in_width[i] / 2.0;
    float ury = in_yloc[i] + in_height[i] / 2.0;
    retVal += sqrt((x - urx) * (x - urx) * (y - ury) * (y - ury));
  }
  return retVal;
}
// --------------------------------------------------------
inline void BTree::setTree(const std::vector<BTreeNode>& ntree)
{   in_tree = ntree; }
// --------------------------------------------------------
inline void BTree::clean_tree(std::vector<BTreeNode>& otree)
{
   int vec_size = otree.size();
   for (int i = 0; i < vec_size; i++)
   {
      otree[i].parent = Undefined;
      otree[i].left = Undefined;
      otree[i].right = Undefined;
   }
   otree[vec_size-2].block_index = vec_size-2;
   otree[vec_size-1].block_index = vec_size-1;

   otree[vec_size-2].orient = Undefined;
   otree[vec_size-1].orient = Undefined;
}
// --------------------------------------------------------
inline void BTree::clean_contour(std::vector<ContourNode>& oContour)
{
   int vec_size = oContour.size();
   int Ledge = vec_size-2;
   int Bedge = vec_size-1;

   oContour[Ledge].next = Bedge;
   oContour[Ledge].prev = Undefined;
   oContour[Ledge].begin = 0;
   oContour[Ledge].end = 0;
   oContour[Ledge].CTL = basepacking_h::Dimension::Infty;

   oContour[Bedge].next = Undefined;
   oContour[Bedge].prev = Ledge;
   oContour[Bedge].begin = 0;
   oContour[Bedge].end = basepacking_h::Dimension::Infty;
   oContour[Bedge].CTL = 0;

   // reset obstacles (so we consider all of them again)
   if (seen_obstacles.size() != getNumObstacles())
      seen_obstacles.resize(getNumObstacles());
   fill(seen_obstacles.begin(), seen_obstacles.end(), false);
}
// --------------------------------------------------------
inline void BTree::rotate(int index,
                          int newOrient)
{
   in_tree[index].orient = newOrient;
   contour_evaluate();
}
// ========================================================
inline unsigned BTree::getNumObstacles() 
{
	return in_obstacles.xloc.size();
}
// ========================================================
inline BTreeOrientedPacking::BTreeOrientedPacking(const BTree& bt)
{
   int blocknum = bt.NUM_BLOCKS;

   xloc.resize(blocknum);
   yloc.resize(blocknum);
   width.resize(blocknum);
   height.resize(blocknum);
   orient.resize(blocknum);
   for (int i = 0; i < blocknum; i++)
   {
      xloc[i] = bt.xloc(i);
      yloc[i] = bt.yloc(i);
      width[i] = bt.width(i);
      height[i] = bt.height(i);
      orient[i] = OrientedPacking::ORIENT(bt.tree[i].orient);
   }
}
// --------------------------------------------------------
inline void BTreeOrientedPacking::operator =(const BTree& bt)
{
   int blocknum = bt.NUM_BLOCKS;

   xloc.resize(blocknum);
   yloc.resize(blocknum);
   width.resize(blocknum);
   height.resize(blocknum);
   orient.resize(blocknum);
   for (int i = 0; i < blocknum; i++)
   {
      xloc[i] = bt.xloc(i);
      yloc[i] = bt.yloc(i);
      width[i] = bt.width(i);
      height[i] = bt.height(i);
      orient[i] = OrientedPacking::ORIENT(bt.tree[i].orient);
   }
}  
// ========================================================

#endif
