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


#include "btree.h"

#include <iostream>
#include <string>
#include <cmath>
#include <climits>
#include <algorithm>
#include <cfloat>

using namespace basepacking_h;
using std::ofstream;
using std::string;
using std::max;
using std::min;
using std::cout;
using std::endl;
using std::vector;



const int BTree::Undefined = Dimension::Undefined;
// ========================================================
BTree::BTree(const HardBlockInfoType& blockinfo)
   : tree(in_tree),
     contour(in_contour),
     NUM_BLOCKS(blockinfo.blocknum()),
     in_blockinfo(blockinfo),
     in_tree(blockinfo.blocknum()+2),
     in_contour(blockinfo.blocknum()+2),
     
     in_xloc(blockinfo.blocknum()+2, Undefined),
     in_yloc(blockinfo.blocknum()+2, Undefined),
     in_width(blockinfo.blocknum()+2, Undefined),
     in_height(blockinfo.blocknum()+2, Undefined),
     
     in_blockArea(blockinfo.blockArea()),
     in_totalArea(0),
     in_totalWidth(0),
     in_totalHeight(0),

     TOLERANCE(0),

	 pack_origin(PACK_BOTTOM)
{
   int vec_size = NUM_BLOCKS+2;
   for (int i = 0; i < vec_size; i++)
   {
      in_tree[i].parent = Undefined;
      in_tree[i].left = Undefined;
      in_tree[i].right = Undefined;
      in_tree[i].block_index = i;
      in_tree[i].orient = Undefined;

      in_contour[i].next = Undefined;
      in_contour[i].prev = Undefined;
      in_contour[i].begin = Undefined;
      in_contour[i].end = Undefined;
      in_contour[i].CTL = Undefined;
   }

   in_contour[NUM_BLOCKS].next = NUM_BLOCKS+1;
   in_contour[NUM_BLOCKS].prev = Undefined;
   in_contour[NUM_BLOCKS].begin = 0;
   in_contour[NUM_BLOCKS].end = 0;
   in_contour[NUM_BLOCKS].CTL = Dimension::Infty;

   in_xloc[NUM_BLOCKS] = 0;
   in_yloc[NUM_BLOCKS] = 0;
   in_width[NUM_BLOCKS] = 0;
   in_height[NUM_BLOCKS] = Dimension::Infty;
   
   in_contour[NUM_BLOCKS+1].next = Undefined;
   in_contour[NUM_BLOCKS+1].prev = NUM_BLOCKS;
   in_contour[NUM_BLOCKS+1].begin = 0;
   in_contour[NUM_BLOCKS+1].end = Dimension::Infty;
   in_contour[NUM_BLOCKS+1].CTL = 0;

   in_xloc[NUM_BLOCKS+1] = 0;
   in_yloc[NUM_BLOCKS+1] = 0;
   in_width[NUM_BLOCKS+1] = Dimension::Infty;
   in_height[NUM_BLOCKS+1] = 0;
}
// --------------------------------------------------------
BTree::BTree(const HardBlockInfoType& blockinfo,
             float nTolerance)
   : tree(in_tree),
     contour(in_contour),
     NUM_BLOCKS(blockinfo.blocknum()),
     in_blockinfo(blockinfo),
     in_tree(blockinfo.blocknum()+2),
     in_contour(blockinfo.blocknum()+2),
     
     in_xloc(blockinfo.blocknum()+2, Undefined),
     in_yloc(blockinfo.blocknum()+2, Undefined),
     in_width(blockinfo.blocknum()+2, Undefined),
     in_height(blockinfo.blocknum()+2, Undefined),
     
     in_blockArea(blockinfo.blockArea()),
     in_totalArea(0),
     in_totalWidth(0),
     in_totalHeight(0),

     TOLERANCE(nTolerance),

	 pack_origin(PACK_BOTTOM)
{
   int vec_size = NUM_BLOCKS+2;
   for (int i = 0; i < vec_size; i++)
   {
      in_tree[i].parent = Undefined;
      in_tree[i].left = Undefined;
      in_tree[i].right = Undefined;
      in_tree[i].block_index = i;
      in_tree[i].orient = Undefined;

      in_contour[i].next = Undefined;
      in_contour[i].prev = Undefined;
      in_contour[i].begin = Undefined;
      in_contour[i].end = Undefined;
      in_contour[i].CTL = Undefined;
   }

   in_contour[NUM_BLOCKS].next = NUM_BLOCKS+1;
   in_contour[NUM_BLOCKS].prev = Undefined;
   in_contour[NUM_BLOCKS].begin = 0;
   in_contour[NUM_BLOCKS].end = 0;
   in_contour[NUM_BLOCKS].CTL = Dimension::Infty;

   in_xloc[NUM_BLOCKS] = 0;
   in_yloc[NUM_BLOCKS] = 0;
   in_width[NUM_BLOCKS] = 0;
   in_height[NUM_BLOCKS] = Dimension::Infty;
   
   in_contour[NUM_BLOCKS+1].next = Undefined;
   in_contour[NUM_BLOCKS+1].prev = NUM_BLOCKS;
   in_contour[NUM_BLOCKS+1].begin = 0;
   in_contour[NUM_BLOCKS+1].end = Dimension::Infty;
   in_contour[NUM_BLOCKS+1].CTL = 0;

   in_xloc[NUM_BLOCKS+1] = 0;
   in_yloc[NUM_BLOCKS+1] = 0;
   in_width[NUM_BLOCKS+1] = Dimension::Infty;
   in_height[NUM_BLOCKS+1] = 0;
}
// --------------------------------------------------------
void BTree::evaluate(const vector<BTreeNode>& ntree)
{
   if (ntree.size() != in_tree.size())
   {
      cout << "ERROR: size of btree's doesn't match." << endl;
      exit(1);
   }

   in_tree = ntree;
   contour_evaluate();
}  
// --------------------------------------------------------
void BTree::evaluate(const vector<int>& tree_bits,
                     const vector<int>& perm,
                     const vector<int>& orient)
{
   if (int(perm.size()) != NUM_BLOCKS)
   {
      cout << "ERROR: the permutation length doesn't match with "
           << "size of the tree." << endl;
      exit(1);
   }
   bits2tree(tree_bits, perm, orient, in_tree);
//   OutputBTree(cout, in_tree);
   contour_evaluate();
}
// --------------------------------------------------------
void BTree::bits2tree(const vector<int>& tree_bits,
                       const vector<int>& perm,
                       const vector<int>& orient,
                       vector<BTreeNode>& ntree)
{
   int perm_size = perm.size();
   ntree.resize(perm_size+2);
   clean_tree(ntree);

   int treePtr = perm_size;
   int bitsPtr = 0;

   int lastAct = -1;
   for (int i = 0; i < perm_size; i++)
   {
      int currAct = tree_bits[bitsPtr];
      while (currAct == 1)
      {
         // move up a level/sibling
         if (lastAct == 1)
            treePtr = ntree[treePtr].parent;

         // move among siblings
         while (ntree[treePtr].right != Undefined)
            treePtr = ntree[treePtr].parent;
         bitsPtr++;
         lastAct = 1;
         currAct = tree_bits[bitsPtr];
      }

      if (lastAct != 1)
         ntree[treePtr].left = perm[i];
      else // lastAct == 1
         ntree[treePtr].right = perm[i];
      
      ntree[perm[i]].parent = treePtr;
      ntree[perm[i]].block_index = perm[i];
      ntree[perm[i]].orient = orient[i];

      treePtr = perm[i];
      lastAct = 0;
      bitsPtr++;
   }         
}
// --------------------------------------------------------
void BTree::swap(int indexOne,
                 int indexTwo)
{
   int indexOne_left = in_tree[indexOne].left;
   int indexOne_right = in_tree[indexOne].right;
   int indexOne_parent = in_tree[indexOne].parent;

   int indexTwo_left = in_tree[indexTwo].left;
   int indexTwo_right = in_tree[indexTwo].right;
   int indexTwo_parent = in_tree[indexTwo].parent;

   if (indexOne == indexTwo_parent)
      swap_parent_child(indexOne, (indexTwo == in_tree[indexOne].left));
   else if (indexTwo == indexOne_parent)
      swap_parent_child(indexTwo, (indexOne == in_tree[indexTwo].left));
   else
   {
      // update around indexOne
      in_tree[indexOne].parent = indexTwo_parent;
      in_tree[indexOne].left = indexTwo_left;
      in_tree[indexOne].right = indexTwo_right;

      if (indexOne == in_tree[indexOne_parent].left)
         in_tree[indexOne_parent].left = indexTwo;
      else
         in_tree[indexOne_parent].right = indexTwo;

      if (indexOne_left != Undefined)
         in_tree[indexOne_left].parent = indexTwo;
      
      if (indexOne_right != Undefined)
         in_tree[indexOne_right].parent = indexTwo;
      
      // update around indexTwo
      in_tree[indexTwo].parent = indexOne_parent;
      in_tree[indexTwo].left = indexOne_left;
      in_tree[indexTwo].right = indexOne_right;
      
      if (indexTwo == in_tree[indexTwo_parent].left)
         in_tree[indexTwo_parent].left = indexOne;
      else
         in_tree[indexTwo_parent].right = indexOne;
      
      if (indexTwo_left != Undefined)
         in_tree[indexTwo_left].parent = indexOne;
      
      if (indexTwo_right != Undefined)
         in_tree[indexTwo_right].parent = indexOne;
   }
   contour_evaluate();   
}
// --------------------------------------------------------
void BTree::swap_parent_child(int parent,
                              bool isLeft)
{
   int parent_parent = in_tree[parent].parent;
   int parent_left = in_tree[parent].left;
   int parent_right = in_tree[parent].right;

   int child = (isLeft)? in_tree[parent].left : in_tree[parent].right;
   int child_left = in_tree[child].left;
   int child_right = in_tree[child].right;

   if (isLeft)
   {
      in_tree[parent].parent = child;
      in_tree[parent].left = child_left;
      in_tree[parent].right = child_right;

      if (parent == in_tree[parent_parent].left)
         in_tree[parent_parent].left = child;
      else
         in_tree[parent_parent].right = child;

      if (parent_right != Undefined)
         in_tree[parent_right].parent = child;

      in_tree[child].parent = parent_parent;
      in_tree[child].left = parent;
      in_tree[child].right = parent_right;

      if (child_left != Undefined)
         in_tree[child_left].parent = parent;

      if (child_right != Undefined)
         in_tree[child_right].parent = parent;
   }
   else
   {
      in_tree[parent].parent = child;
      in_tree[parent].left = child_left;
      in_tree[parent].right = child_right;

      if (parent == in_tree[parent_parent].left)
         in_tree[parent_parent].left = child;
      else
         in_tree[parent_parent].right = child;

      if (parent_left != Undefined)
         in_tree[parent_left].parent = child;

      in_tree[child].parent = parent_parent;
      in_tree[child].left = parent_left;
      in_tree[child].right = parent;

      if (child_left != Undefined)
         in_tree[child_left].parent = parent;

      if (child_right != Undefined)
         in_tree[child_right].parent = parent;
   }
}
// --------------------------------------------------------
void BTree::move(int index,
                 int target,
                 bool leftChild)
{
   int index_parent = in_tree[index].parent;
   int index_left = in_tree[index].left;
   int index_right = in_tree[index].right;

   // remove "index" from the tree
   if ((index_left != Undefined) && (index_right != Undefined))
      remove_left_up_right_down(index);
   else if (index_left != Undefined)
   {
      in_tree[index_left].parent = index_parent;
      if (index == in_tree[index_parent].left)
         in_tree[index_parent].left = index_left;
      else
         in_tree[index_parent].right = index_left;
   }
   else if (index_right != Undefined)
   {
      in_tree[index_right].parent = index_parent;
      if (index == in_tree[index_parent].left)
         in_tree[index_parent].left = index_right;
      else
         in_tree[index_parent].right = index_right;
   }
   else
   {
      if (index == in_tree[index_parent].left)
         in_tree[index_parent].left = Undefined;
      else
         in_tree[index_parent].right = Undefined;
   }

   int target_left = in_tree[target].left;
   int target_right = in_tree[target].right;
   
   // add "index" to the required location
   if (leftChild)
   {
      in_tree[target].left = index;
      if (target_left != Undefined)
         in_tree[target_left].parent = index;
         
      in_tree[index].parent = target;
      in_tree[index].left = target_left;
      in_tree[index].right = Undefined;
   }
   else
   {
      in_tree[target].right = index;
      if (target_right != Undefined)
         in_tree[target_right].parent = index;

      in_tree[index].parent = target;
      in_tree[index].left = Undefined;
      in_tree[index].right = target_right;
   }
  
//   cout << "Start Move Evaluate!!" << endl; 
   contour_evaluate();
//  cout << "After FINISH xLoc yLoc Check" << endl;
//  for(int i=0; i<NUM_BLOCKS; i++) {
//    cout << i << " " << in_xloc[i] << " " << in_yloc[i] << endl;
//  }
//   cout << "Finish Move Evaluate!!" << endl;
//
}
// --------------------------------------------------------
void BTree::remove_left_up_right_down(int index)
{
   int index_parent = in_tree[index].parent;
   int index_left = in_tree[index].left;
   int index_right = in_tree[index].right;

   in_tree[index_left].parent = index_parent;
   if (index == in_tree[index_parent].left)
      in_tree[index_parent].left = index_left;
   else
      in_tree[index_parent].right = index_left;

   int ptr = index_left;
   while (in_tree[ptr].right != Undefined)
      ptr = in_tree[ptr].right;

   in_tree[ptr].right = index_right;
   in_tree[index_right].parent = ptr;
}
// --------------------------------------------------------
void BTree::transform_bbox_wrt_pack_origin(BTree::PackOrigin packOrigin, 
    float frameWidth, float frameHeight,
    float &xMin, float &yMin, float &xMax, float &yMax)
// <aaronnn> parquet may choose to build trees from various origins 
// - this function is to make sure that objects that have coordinates based on
// the bin's bottom left will be updated appropriately
{
	float originalXMin = xMin;
	float originalXMax = xMax;
	float originalYMin = yMin;
	float originalYMax = yMax;
	float originalWidth = xMax - xMin;
	float originalHeight = yMax - yMin;

//  cout << "bbox change function" << endl;
//  cout << "packOrigin: " << packOrigin << endl;
//  cout << "fw: " << frameWidth << ", fh: " << frameHeight << endl;
	if (packOrigin == BTree::PACK_LEFT) {
		// orthogonal to default - bottom left origin and grow upward and right
		xMin = originalYMin;
		yMin = originalXMin;
		float width = originalHeight;
		float height = originalWidth;
		xMax = xMin + width;
		yMax = yMin + height;
	} else if (packOrigin == BTree::PACK_TOP) {
		// 'reverse' of default - mirror in y direction
		yMin = frameHeight - originalYMax;
		yMax = yMin + originalHeight;
	} else if (packOrigin == BTree::PACK_RIGHT) {
		// 'reverse' of orth. to def. - rotate frame 90degrees counter clockwise
		xMin = originalYMin;
		yMin = frameWidth - originalXMax;
		float width = originalHeight;
		float height = originalWidth;
		xMax = xMin + width;
		yMax = yMin + height;
	} else {
		// do nothing
	}
}
// --------------------------------------------------------
void BTree::contour_evaluate() // assume the tree is set
{
	clean_contour(in_contour);

	// x- and y- shifts for new block to avoid obstacles
	new_block_x_shift = 0;
	new_block_y_shift = 0;

	int tree_prev = NUM_BLOCKS;
	int tree_curr = in_tree[NUM_BLOCKS].left; // start with first block
//  cout << "ContourEvaluate Executed!" << endl;
//  cout << "InObstacleFrame: " << in_obstacleframe[0] << ", " << in_obstacleframe[1] << endl;
	while (tree_curr != NUM_BLOCKS) // until reach the root again
	{
//		cout << "curr: " << tree_curr << " / " << in_tree[tree_curr].left 
//      << " / " << in_tree[tree_curr].right << " // P: " << in_tree[tree_curr].parent << endl ;
		if (tree_prev == in_tree[tree_curr].parent)
		{
			unsigned obstacleID = UINT_MAX;
			float obstacle_xMin, obstacle_xMax, obstacle_yMin, obstacle_yMax;
			float new_xMin, new_xMax, new_yMin, new_yMax;

			if (contour_new_block_intersects_obstacle(tree_curr, obstacleID,
			    new_xMin, new_xMax, new_yMin, new_yMax,
			    obstacle_xMin, obstacle_xMax, obstacle_yMin, obstacle_yMax)) {
				// 'add obstacle' and then resume building the tree from here

//        printf( "New (%f %f) - (%f %f)\n", new_xMin, new_yMin, new_xMax, new_yMax);
//        printf( "Obs (%f %f) - (%f %f)\n", obstacle_xMin, obstacle_yMin, obstacle_xMax, obstacle_yMax); 
				int tree_parent = in_tree[tree_curr].parent;
				//int parent_index = in_tree[tree_parent].block_index;
				//int parent_theta = in_tree[tree_parent].orient;
				//float parent_height = in_blockinfo[parent_index].height[parent_theta];
				float block_height = new_yMax - new_yMin;
				float block_width = new_xMax - new_xMin;

				if ((tree_curr == in_tree[tree_parent].left
				    && obstacle_yMax + block_height > in_contour[tree_parent].CTL + (block_height * 0.5)
				    && (obstacle_xMax + block_width) < in_obstacleframe[0])
				    || (tree_curr == in_tree[tree_parent].right
				    && (obstacle_yMax + block_height) > in_obstacleframe[1])) {
					// left child & shifting up makes it too high
					// or right child & shifting up makes it too high;
					// shift the starting location of the block right in x 
					new_block_x_shift += obstacle_xMax - new_xMin;
					new_block_y_shift = 0;
				} else {
					// shift the block in y
					new_block_y_shift += obstacle_yMax - new_yMin;
				}
//        cout << (tree_curr == in_tree[tree_parent].left? "LeftChild" : "RightChild") << endl;
//        cout << "newBlockShift: (" << new_block_x_shift << ", " << new_block_y_shift << ")" << endl << endl;;
			} else {
//        printf( "%d %s (%f %f) - (%f %f)\n", 
//            tree_curr, in_blockinfo.block_names[in_tree[tree_curr].block_index].c_str(),  
//            new_xMin, new_yMin, new_xMax, new_yMax);
				contour_add_block(tree_curr);

				// <aaronnn> reset x- y- obstacle shift
				new_block_x_shift = 0;
				new_block_y_shift = 0;

				tree_prev = tree_curr;
				if (in_tree[tree_curr].left != Undefined)
					tree_curr = in_tree[tree_curr].left;
				else if (in_tree[tree_curr].right != Undefined)
					tree_curr = in_tree[tree_curr].right;
				else
					tree_curr = in_tree[tree_curr].parent;
			}
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
  
//  cout << "contourTraverse" << endl;

  in_totalContourArea = 0;
	while (contour_ptr != NUM_BLOCKS+1)
	{
		in_totalHeight = max(in_totalHeight, in_contour[contour_ptr].CTL);
//    cout << "Node: " << contour_ptr << endl;
//    cout << "Prev: " << in_contour[contour_ptr].prev << endl;
//    cout << "Next: " << in_contour[contour_ptr].next << endl;
//    cout << "Begin: " << in_contour[contour_ptr].begin << endl;
//    cout << "End: " << in_contour[contour_ptr].end << endl;
//    cout << "CTL: " << in_contour[contour_ptr].CTL << endl << endl;
    
    // Calculate contour area
//    cout << "real " << in_contour[contour_ptr].begin << " " << in_contour[contour_ptr].end << " : " << in_contour[contour_ptr].CTL << " " << 
//      (in_contour[contour_ptr].end - in_contour[contour_ptr].begin)*in_contour[contour_ptr].CTL << endl;
    in_totalContourArea += (in_contour[contour_ptr].end - in_contour[contour_ptr].begin)*in_contour[contour_ptr].CTL;
    
    // go to next pointer
		contour_ptr = in_contour[contour_ptr].next;
	}
//  cout << "realFinal: " << in_totalContourArea << endl;

//  cout << "xLoc yLoc Check" << endl;
//  for(int i=0; i<NUM_BLOCKS; i++) {
//    cout << i << " " << in_xloc[i] << " " << in_yloc[i] << endl;
//  }

	in_totalArea = in_totalWidth * in_totalHeight;
}

//#define FLT_EPSILON numeric_limits< float >::epsilon()
inline float get_snap_coordinates(float input, float snapVal) {
  // snap Value is zero, then simply return input values.
  if( std::abs( snapVal ) <= FLT_EPSILON ) {
    return input;
  }

  // otherwise, calculate the coordinates
  float calVal = input / snapVal; 
  int roundVal = calVal;
  if( std::abs( (float)roundVal - calVal ) <= FLT_EPSILON ) {
    return input;
  }
  else {
    return (roundVal+1)*snapVal;
  }
}

//#define FLT_EPSILON numeric_limits< float >::epsilon()
inline float get_snap_coordinates_gap(float input, float snapVal) {
  // snap Value is zero, then simply return input values.
  if( std::abs( snapVal ) <= FLT_EPSILON ) {
    return 0;
  }

  // otherwise, calculate the coordinates
  float calVal = input / snapVal; 
  int roundVal = calVal;
  if( std::abs( (float)roundVal - calVal ) <= FLT_EPSILON ) {
    return 0;
  }
  else {
    return (roundVal+1)*snapVal - input;
  }
}
// --------------------------------------------------------
void BTree::find_new_block_location(const int tree_ptr, 
    float &out_x, float &out_y, int &contour_prev, int &contour_ptr)
{
  // three values
  // 1) new_block_x_shift : obstacle shift
  // 2) halo_block_x_shift : halo consideration
  // 3) snap_x_gap : macro snapping to specified grid.
  //
  // Hence, we have to generate/update below variables:
  // 1) new_block_contour_begin
  // 2) new_block_contour_end
  // 3) contour_prev_ptr
	int tree_parent = in_tree[tree_ptr].parent;
	contour_prev = Undefined;
	contour_ptr = Undefined;
	
  int block = in_tree[tree_ptr].block_index;
	int theta = in_tree[tree_ptr].orient;

	float new_block_contour_begin;
  float halo_block_x_shift = 0.0f, halo_block_y_shift = 0.0f;
	if (tree_ptr == in_tree[tree_parent].left)
	{
		// to the right of parent, start x where parent ends
		new_block_contour_begin = in_contour[tree_parent].end;
		// use block that's right of parent's contour for y
		contour_ptr = in_contour[tree_parent].next;

    if (tree_parent != NUM_BLOCKS) {
      halo_block_x_shift += in_blockinfo[block].haloX[theta];
    }
	}
	else
	{
    int parent_block = in_tree[tree_parent].block_index;
		// above parent, use parent's x
		new_block_contour_begin = in_contour[tree_parent].begin;

    // additional HALO consideration
    if( new_block_contour_begin != 0 ) {
//      new_block_contour_begin += in_blockinfo[parent_block].haloX[theta];
      halo_block_x_shift += in_blockinfo[block].haloX[theta];
    } 
		// use parent's contour for y
		contour_ptr = tree_parent;

    if( tree_ptr == in_tree[tree_parent].right ) {
      halo_block_y_shift += in_blockinfo[block].haloY[theta]; 
    }
	}

  // Get obstacle-aware snap gap 
  float snap_gap_x = get_snap_coordinates_gap( 
      new_block_contour_begin + new_block_x_shift, 
      in_blockinfo[block].snapX[theta] );

	new_block_contour_begin += new_block_x_shift; // considering obstacles
	new_block_contour_begin += snap_gap_x; // considering snapping

	contour_prev = in_contour[contour_ptr].prev; // begins of cPtr/tPtr match

//  cout << "ctrPtr: " << contour_ptr << ", ctrPrev: " << contour_prev << endl;

//  cout << block << " snapX: " << new_block_contour_begin;
  // macro snapping  
//  new_block_contour_begin = 
//    snap_coordinates( new_block_contour_begin, in_blockinfo[block].snapX[theta] ) ;

//  cout << "new_block_contour_begin: " << new_block_contour_begin << endl;

	float new_block_contour_end = new_block_contour_begin
	    + in_blockinfo[block].width[theta] + 2*in_blockinfo[block].haloX[theta];
//  cout << block << " newBlockContourEnd: " << new_block_contour_end << endl;
	float maxCTL = in_contour[contour_ptr].CTL;
	float contour_ptr_end = 
    (contour_ptr == tree_ptr)? new_block_contour_end : in_contour[contour_ptr].end;
//  cout << block << " curContourPtrEnd: " << contour_ptr_end << endl;

	while (contour_ptr_end <=	new_block_contour_end + TOLERANCE) {
//      cout << "contourPtr: " << contour_ptr << endl;
//      cout << "ctrMapEnd: " << contour_ptr_end << endl;
//      cout << "newBlockContourEnd: " << new_block_contour_end<< endl;
		maxCTL = max(maxCTL, in_contour[contour_ptr].CTL);
		contour_ptr = in_contour[contour_ptr].next;
		contour_ptr_end = 
      (contour_ptr == tree_ptr)? new_block_contour_end : in_contour[contour_ptr].end;
    
//      cout << "nextContourPtr: " << contour_ptr << endl;
//      cout << "nextContourPtrEnd: " << contour_ptr_end << endl << endl;
	}

  // contour_prev location Update!!!!  
  if( snap_gap_x + new_block_x_shift > FLT_EPSILON ) { 
//    cout << "contour_prev update from :" << contour_prev << endl;
    int contour_ptr_tmp = contour_prev;
    float contour_ptr_end_sec = 
      (contour_ptr_tmp == tree_ptr)? new_block_contour_end : in_contour[contour_ptr_tmp].begin;

//    cout << "contour_ptr_end_sec: " <<contour_ptr_end_sec<< endl;
//    cout << "tree_ptr: " << tree_ptr << endl;
//    cout << "new_block_contour_end: " << new_block_contour_end<< endl;

    while( contour_ptr_end_sec <= new_block_contour_begin ) {
//      cout << contour_ptr_tmp << " -> " << in_contour[contour_ptr_tmp].next << endl;
      contour_ptr_tmp = in_contour[contour_ptr_tmp].next;
      contour_ptr_end_sec = 
        (contour_ptr_tmp == tree_ptr)? new_block_contour_end : in_contour[contour_ptr_tmp].end;
      contour_prev = in_contour[contour_ptr_tmp].prev;
//      cout << "contour_prev: " << contour_prev << endl;
//      cout << "ineq: " << contour_ptr_end_sec << " " << new_block_contour_begin << endl << endl;
//      contour_prev = contour_ptr_tmp;
    }

    int prev_next = in_contour[contour_prev].next;

//    cout << "prev->next: " << in_contour[contour_prev].next << endl;
//    cout << "ctrMap(prev->next).end: " << in_contour[in_contour[contour_prev].next].end << endl;
//    cout << "ctrMap(prev->next).begin: " << in_contour[in_contour[contour_prev].next].begin<< endl;

    if( prev_next != NUM_BLOCKS+1 
        && in_contour[prev_next].end > new_block_contour_begin 
        && in_contour[prev_next].begin < new_block_contour_begin ) {
       contour_prev = in_contour[contour_prev].next;
    }
    
//    cout << "updated contour_ptr_end_sec: " <<contour_ptr_end_sec << endl;
//    cout << "contour_prev updated into " << contour_prev << endl;
  }

//	contour_prev = in_contour[contour_ptr].prev; // begins of cPtr/tPtr match

//  if( block == 1 || block == 10 || block == 2) { 
    int tmpCursor = NUM_BLOCKS;
//    cout << "InnerContourTraverse" << endl;
//    while (tmpCursor != NUM_BLOCKS+1)
//    {
//      cout << "Node: " << tmpCursor << endl;
//      cout << "Prev: " << in_contour[tmpCursor].prev << endl;
//      cout << "Next: " << in_contour[tmpCursor].next << endl;
//      cout << "Begin: " << in_contour[tmpCursor].begin << endl;
//      cout << "End: " << in_contour[tmpCursor].end << endl;
//      cout << "CTL: " << in_contour[tmpCursor].CTL << endl << endl;
//      tmpCursor = in_contour[tmpCursor].next;
//    }
//  }

	float contour_ptr_begin = 
    (contour_ptr == tree_ptr)? new_block_contour_begin : in_contour[contour_ptr].begin;

	if (contour_ptr_begin + TOLERANCE < new_block_contour_end)
		maxCTL = max(maxCTL, in_contour[contour_ptr].CTL);

  // left-child upper halo consideration
  if( maxCTL != 0 && tree_ptr == in_tree[tree_parent].left ) { 
    halo_block_y_shift += in_blockinfo[block].haloY[theta]; 
  }

	// get location where new block sho1uld be added
	out_x = new_block_contour_begin + halo_block_x_shift;
//	out_x = snap_coordinates( new_block_contour_begin, in_blockinfo[block].snapX[theta] ) ;
	out_y = get_snap_coordinates( maxCTL + new_block_y_shift + halo_block_y_shift, in_blockinfo[block].snapY[theta] ) ; // XXX
}
// --------------------------------------------------------
void BTree::contour_add_block(const int tree_ptr)
{
   int contour_ptr = Undefined;
   int contour_prev = Undefined;
   float new_xloc, new_yloc;
   
   find_new_block_location(tree_ptr, new_xloc, new_yloc, contour_prev, contour_ptr);
//   cout << tree_ptr << ": newLoc (" << new_xloc << ", " << new_yloc << ") - ctr_prev: " << contour_prev << " contour_ptr: " << contour_ptr << endl;


   int block = in_tree[tree_ptr].block_index;
   int theta = in_tree[tree_ptr].orient;

   in_xloc[tree_ptr] = new_xloc;
   in_yloc[tree_ptr] = new_yloc;
   in_width[tree_ptr] = in_blockinfo[block].width[theta];
   in_height[tree_ptr] = in_blockinfo[block].height[theta];
//   printf("Update!: %d: (%f %f)-(%f %f)\n", tree_ptr, new_xloc, new_yloc, 
//       new_xloc + in_width[tree_ptr], 
//       new_yloc + in_height[tree_ptr]);
     
//   in_contour[tree_ptr].begin = in_xloc[tree_ptr] - new_block_x_shift;

   // x_halo control
   int tree_parent = in_tree[tree_ptr].parent;
   if( tree_parent == NUM_BLOCKS+1 ) {
     in_contour[tree_ptr].begin = in_xloc[tree_ptr];
   }
   else {
     in_contour[tree_ptr].begin = in_xloc[tree_ptr] - in_blockinfo[block].haloX[theta];
   }

   in_contour[tree_ptr].end = in_xloc[tree_ptr] + in_width[tree_ptr] 
     + in_blockinfo[block].haloX[theta];
   in_contour[tree_ptr].CTL =  in_yloc[tree_ptr] + in_height[tree_ptr] 
     + in_blockinfo[block].haloY[theta];
   in_contour[tree_ptr].next = contour_ptr;
   in_contour[tree_ptr].prev = contour_prev;

//     cout << tree_ptr << " ctrMap Update" << endl;
//     cout << "tree_ptr: " << tree_ptr << endl;
//     cout << "begin: " << in_contour[tree_ptr].begin << endl;
//     cout << "end: " << in_contour[tree_ptr].end << endl;
//     cout << "CTL: " << in_contour[tree_ptr].CTL << endl;
//     cout << "prev: " << in_contour[tree_ptr].prev << endl;
//     cout << "next: " << in_contour[tree_ptr].next << endl << endl;


   in_contour[contour_ptr].prev = tree_ptr;
   in_contour[contour_prev].next = tree_ptr;
   in_contour[contour_ptr].begin = in_xloc[tree_ptr] + in_width[tree_ptr] + in_blockinfo[block].haloX[theta];
   //in_contour[contour_ptr].begin = in_xloc[tree_ptr] + in_width[tree_ptr];
   in_contour[tree_ptr].begin = max(in_contour[contour_prev].end, in_contour[tree_ptr].begin);
//   in_contour[tree_ptr].begin = max(0.0f, in_contour[tree_ptr].begin);
     
//   cout << "contour_ptr: " << contour_ptr << endl;
//   cout << "contour_ptr's begin: " << in_contour[contour_ptr].begin << endl ;
//   cout << "tree_ptr's begin: " << in_contour[tree_ptr].begin << endl << endl;
}
// --------------------------------------------------------
bool BTree::contour_new_block_intersects_obstacle(const int tree_ptr, unsigned &obstacleID,
    float &new_xMin, float &new_xMax, float &new_yMin, float &new_yMax,
    float &obstacle_xMin, float &obstacle_xMax, float &obstacle_yMin, float &obstacle_yMax)
// <aaronnn> check if adding this new block intersects an obstacle,
//           return obstacleID if it does
// TODO: smarter way of storing/searching obstacles. 
// Currently complexity is O(foreach add_block * foreach unseen_obstacle)
{ 
	if (getNumObstacles() == 0)
		return false; // don't even bother
	
	obstacleID = Undefined;
	int contour_ptr = Undefined;
	int contour_prev = Undefined;

	find_new_block_location(tree_ptr, new_xMin, new_yMin, contour_prev, contour_ptr);
//  cout << tree_ptr << ": contNewLoc (" << new_xMin << ", " << new_yMin << ")" << endl;

	int block = in_tree[tree_ptr].block_index;
	int theta = in_tree[tree_ptr].orient;

	// get the rest of the bbox of new block, if the new block were added
	new_xMax = new_xMin + in_blockinfo[block].width[theta];
	new_yMax = new_yMin + in_blockinfo[block].height[theta];

	// check if adding this new block will create a contour that 
	// intersects with an obstacle
	for (unsigned i=0; i < getNumObstacles(); i++) {
		//if (seen_obstacles[i]) 
		//	continue;

		obstacle_xMin = in_obstacles.xloc[i];
		obstacle_yMin = in_obstacles.yloc[i];
		obstacle_xMax = obstacle_xMin + in_obstacles.width[i];
		obstacle_yMax = obstacle_yMin + in_obstacles.height[i];

		transform_bbox_wrt_pack_origin(pack_origin, 
		    in_obstacleframe[0], in_obstacleframe[1], 
		    obstacle_xMin, obstacle_yMin, obstacle_xMax, obstacle_yMax);

//    cout << "Obs " << i << " (" << obstacle_xMin << ", " << obstacle_yMin << ") - (" <<
//      obstacle_xMax << ", " << obstacle_yMax << ")" << endl;

		if ((new_xMax <= obstacle_xMin) || (new_xMin >= obstacle_xMax)
				|| (new_yMax <= obstacle_yMin) || (new_yMin >= obstacle_yMax))
			continue;
    
//    cout << "Obs " << i << " is overlapped" << endl; 

		obstacleID = i;
		//cout << "ANDBG block " << tree_ptr << " intersects " << obstacleID << endl;
		//cout << "ANDBG block (" << new_xMin << ", " << new_yMin << ") - (" << new_xMax << ", " << new_yMax << ")" << endl;
		//cout <<"       intersects (" << obstacle_xMin << ", " << obstacle_yMin << ") - (" << obstacle_xMax << ", " << obstacle_yMax << ")" << endl;
		return true;
	}

	return false;
}
// --------------------------------------------------------
void BTree::save_bbb(const string& filename) const
{
   ofstream outfile;
   outfile.open(filename.c_str());
   if (!outfile.good())
   {
      cout << "ERROR: cannot open file" << filename << endl;
      exit(1);
   }

   outfile.setf(std::ios::fixed);
   outfile.precision(3);

   outfile << in_totalWidth << endl;
   outfile << in_totalHeight << endl;
   outfile << NUM_BLOCKS << endl;
   for (int i = 0; i < NUM_BLOCKS; i++)
      outfile << in_width[i] << " " << in_height[i] << endl;
   outfile << endl;

   for (int i = 0; i < NUM_BLOCKS; i++)
      outfile << in_xloc[i] << " " << in_yloc[i] << endl;
   outfile << endl;
}

void BTree::save_dot(const std::string& fileName) const {
  std::filebuf fb;
  fb.open( fileName.c_str(), std::ios::out );
  std::ostream outs (&fb);

  outs << "digraph BST {" << std::endl;
  outs << "  graph [ordering=\"out\"];" << std::endl;

  int nullPtrCnt = 0; 
  for(int i=0; i< NUM_BLOCKS; i++) {
    outs << "  \"" << i << "\" -> ";
    if( in_tree[i].left != Undefined) {
      outs << "\"" << in_tree[i].left << "\";" << std::endl;
    }
    else {
      outs << "null" << std::to_string(nullPtrCnt) << std::endl;
      outs << "  null" << std::to_string(nullPtrCnt++) << " [shape=point];" << std::endl;
    } 

    outs << "  \"" << i << "\" -> ";
    if( in_tree[i].right!= Undefined) {
      outs << "\"" << in_tree[i].right<< "\";" << std::endl;
    }
    else {
      outs << "null" << std::to_string(nullPtrCnt) << std::endl;
      outs << "  null" << std::to_string(nullPtrCnt++) << " [shape=point];" << std::endl;
    } 
  } 
  outs << "}" << std::endl;
  fb.close();
}

void BTree::save_plot(const std::string& fileName) const {
  using std::endl;
  std::filebuf fb;
  fb.open( fileName.c_str(), std::ios::out );
  std::ostream outs (&fb);
  
  outs<<"#Use this file as a script for gnuplot"<<endl;
  outs<<"#(See http://www.gnuplot.info/ for details)"<<endl;
  outs << "set nokey"<<endl;

  outs << "set size ratio -1" << endl;
  outs << "set title ' " << fileName << endl << endl;
  
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

// --------------------------------------------------------
void BTree::addObstacles(BasePacking &obstacles, float obstacleFrame[2])
// <aaronnn> make this btree aware of obstacles
{
	in_obstacles.xloc = obstacles.xloc;
	in_obstacles.yloc = obstacles.yloc;
	in_obstacles.width = obstacles.width;
	in_obstacles.height = obstacles.height;

	in_obstacleframe[0] = obstacleFrame[0];
	in_obstacleframe[1] = obstacleFrame[1];
}

// --------------------------------------------------------
