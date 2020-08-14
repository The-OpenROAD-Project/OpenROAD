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


#ifndef BTREEANNEAL_H
#define BTREEANNEAL_H 

#include "basepacking.h"
#include "mixedpacking.h"
#include "btree.h"
#include "btreeslackeval.h"
#include "btreecompact.h"
#include "baseannealer.h"
#include "debug.h"
#include "allparquet.h"

#include <cfloat>

// --------------------------------------------------------
class BTreeAreaWireAnnealer : public BaseAnnealer
{
public:
   BTreeAreaWireAnnealer(const parquetfp::Command_Line *const params,
                         parquetfp::DB *const db);
   
   BTreeAreaWireAnnealer(MixedBlockInfoType& nBlockinfo,
                         const parquetfp::Command_Line *const params,
                         parquetfp::DB *const db);
   
   inline virtual ~BTreeAreaWireAnnealer();

   virtual bool go();
   bool anneal(); // WARNING: anneal() takes at least TWO blocks
                  //   use packOneBlock() to floorplan ONE block
   virtual bool packOneBlock();
   
   inline const BTree& currSolution() const;
   inline const BTree& bestSolution() const;

   virtual void takePlfromDB();
   virtual void compactSoln(bool minWL, bool fixedOutline,
                            float reqdH, float reqdW);
   
   int makeMove(int& indexOrient,
                parquetfp::ORIENT& newOrient,
                parquetfp::ORIENT& oldOrient); // returns 1,2,3,4,5 or -1
   int makeMoveSlacks();                     // returns 6
   int makeMoveSlacksOrient() { return 10; } // returns 10
   int makeMoveOrient() { return 10; }       // returns 10
   int makeARMove();                         // returns 7
   int makeSoftBlMove(int& index, float& newWidth, float& newHeight);
                                             // returns 11
   inline int makeIndexSoftBlMove(int index,
                                  float& newWidth, float& newHeight);
                                             // returns 11
   
   int makeHPWLMove();                       // returns 12
   int makeARWLMove();                       // returns 13

   int packSoftBlocks(int numIter);          // returns -1
   inline int compactBlocks();               // returns -1        
   
   inline static void sort_slacks(const std::vector<float>& slacks,
                                  std::vector<int>& indices_sorted);
   void GenerateRandomSoln(BTree& soln, int blocknum) const;
   
   inline unsigned getNumObstacles();

 protected:
   MixedBlockInfoType *const _blockinfo_cleaner;
   MixedBlockInfoType& _blockinfo;
   BasePacking _obstacleinfo;
   float _obstacleFrame[2];

 public:
   const MixedBlockInfoType& blockinfo;

 protected:

   BTree in_curr_solution;
   BTree in_next_solution; // sketch-board
   BTree in_best_solution;

   // (blockIndex, [0-7]) --> [0-7], constantly 0 if fixedOrient
   std::vector< std::vector<parquetfp::ORIENT> > _physicalOrient;
   
   BTreeSlackEval *_slackEval;

   void constructor_core();
   inline BTree::MoveType get_move() const;
   inline void perform_swap();
   inline void perform_move();
   inline void perform_rotate();
   inline void perform_rotate(int& blk,
                              parquetfp::ORIENT& newOrient,
                              parquetfp::ORIENT& oldOrient);
   class SlackInfo
   {
   public:
      float slack;
      int index;

      inline bool operator <(const SlackInfo& si) const
         {   return slack < si.slack; }
   };

   void DBfromSoln(const BTree& soln); // update *_db from "soln"
   
   void makeMoveSlacksCore(bool); // used by "makeMoveSlacks" and "makeARMove"
   void locateSearchBlocks(int, std::vector<int>&); // used by HPWL and ARWL moves

   // the following -SoftBl- fcns assumes slacks are ALREADY EVAULATED
   int getSoftBlIndex(bool horizontal) const; // returns the index of the operand
   int getSoftBlNewDimensions(int index,
                              float& newWidth, float& newHeight) const; 
   // used by soft-block moves, returns 11

   BTreeAreaWireAnnealer(MixedBlockInfoType& nBlockinfo);

};
// --------------------------------------------------------

// =========================
//      IMPLEMENTATIONS
// =========================
BTreeAreaWireAnnealer::~BTreeAreaWireAnnealer()
{
   if (_slackEval != NULL)
      delete _slackEval;

   if (_blockinfo_cleaner != NULL)
      delete _blockinfo_cleaner;
}
// --------------------------------------------------------
const BTree& BTreeAreaWireAnnealer::currSolution() const
{   return in_curr_solution; }
// --------------------------------------------------------
const BTree& BTreeAreaWireAnnealer::bestSolution() const
{   return in_best_solution; }
// --------------------------------------------------------
int BTreeAreaWireAnnealer::makeIndexSoftBlMove(int index,
                                               float& newWidth,
                                               float& newHeight)
{
   _slackEval->evaluateSlacks(in_curr_solution);
   return getSoftBlNewDimensions(index, newWidth, newHeight);
}
// --------------------------------------------------------
int BTreeAreaWireAnnealer::compactBlocks()
{
   std::cout << "compactBlocks" << std::endl;
   BTreeCompactor compactor(in_curr_solution);
   compactor.compact();
   in_curr_solution = compactor;
   return MISC;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::sort_slacks(const std::vector<float>& slacks,
                                        std::vector<int>& indices_sorted)
{
   int blocknum = slacks.size();
   std::vector<SlackInfo> slackinfo(blocknum);
   for (int i = 0; i < blocknum; i++)
   {
      slackinfo[i].slack = slacks[i];
      slackinfo[i].index = i;
   }
   sort(slackinfo.begin(), slackinfo.end());

   indices_sorted.resize(blocknum);
   for (int i = 0; i < blocknum; i++)
      indices_sorted[i] = slackinfo[i].index;
}
// --------------------------------------------------------
BTree::MoveType BTreeAreaWireAnnealer::get_move() const
{
   // 0 <= "rand_num" < 1
   float rand_num = rand() / (RAND_MAX + 1.0);
   if (rand_num < 0.3333)
      return BTree::SWAP;
   else if (rand_num < 0.6666)
      return BTree::ROTATE;
   else
      return BTree::MOVE;
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::perform_swap()
{
   int blocknum = blockinfo.currDimensions.blocknum();
   int blkA = int(blocknum * (rand() / (RAND_MAX + 1.0)));
   int blkB = int((blocknum-1) * (rand() / (RAND_MAX + 1.0)));
   blkB = (blkB >= blkA)? blkB+1 : blkB;
   
   in_next_solution = in_curr_solution;
   in_next_solution.swap(blkA, blkB);
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::perform_rotate()
{
   int blocknum = blockinfo.currDimensions.blocknum();
   int blk = int(blocknum * (rand() / (RAND_MAX + 1.0)));

   // may want to do something different here,
   // like search for something that can be rotated
   if(_db->getNodes()->getNode(blk).isOrientFixed()) return;

   int blk_orient = in_curr_solution.tree[blk].orient;
   int new_orient = (blk_orient+1) % 8;
   new_orient = _physicalOrient[blk][new_orient];

   // in_next_solution = in_curr_solution;
   in_next_solution.setTree(in_curr_solution.tree);
   in_next_solution.rotate(blk, new_orient);
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::perform_move()
{
   int blocknum = blockinfo.currDimensions.blocknum();
   int blk = int(blocknum * (rand() / (RAND_MAX + 1.0)));

   int target_rand_num = int((2*blocknum-1) * (rand() / (RAND_MAX + 1.0)));
   int target = target_rand_num / 2;
   target = (target >= blk)? target+1 : target;
   
   int leftChild = target_rand_num % 2;

   // in_next_solution = in_curr_solution;
   in_next_solution.setTree(in_curr_solution.tree);
   in_next_solution.move(blk, target, (leftChild == 0));
}
// --------------------------------------------------------
void BTreeAreaWireAnnealer::perform_rotate(int& blk,
                                           parquetfp::ORIENT& newOrient,
                                           parquetfp::ORIENT& oldOrient)
{
   int blocknum = blockinfo.currDimensions.blocknum();
   blk = int(blocknum * (rand() / (RAND_MAX + 1.0)));
   int blk_orient = in_curr_solution.tree[blk].orient;
   int new_orient = blk_orient;

   if(_db->getNodes()->getNode(blk).isOrientFixed())
   {
     // may want to do something different here,
     // like search for something that can be rotated
   }
   else
   {
     if (_params->minWL)
     {
        while (new_orient == blk_orient)
           new_orient = (blk_orient + rand() % 8) % 8;
     }
     else
        new_orient = (blk_orient+1) % 8;
     new_orient = _physicalOrient[blk][new_orient];

     // in_next_solution = in_curr_solution;
     in_next_solution.setTree(in_curr_solution.tree);
     in_next_solution.rotate(blk, new_orient);
   }

   newOrient = parquetfp::ORIENT(new_orient);
   oldOrient = parquetfp::ORIENT(blk_orient);
}
// ========================================================
inline unsigned BTreeAreaWireAnnealer::getNumObstacles()
{
	return _obstacleinfo.xloc.size();
}
// ========================================================

#endif
