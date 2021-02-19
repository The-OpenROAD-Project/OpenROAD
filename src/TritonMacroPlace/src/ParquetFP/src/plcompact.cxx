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


#include "plcompact.h"
#include "basepacking.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <cfloat>

using std::cout;
using std::endl;
using std::vector;

const float ShiftLegalizer::Infty = basepacking_h::Dimension::Infty;
const int ShiftLegalizer::Undefined = basepacking_h::Dimension::Undefined;

const float ShiftBlock::Infty = basepacking_h::Dimension::Infty;
const float ShiftBlock::Neg_Infty = -1 * ShiftBlock::Infty;
const int ShiftBlock::Undefined = basepacking_h::Dimension::Undefined;
// --------------------------------------------------------
ShiftLegalizer::ShiftLegalizer(const vector<float>& n_xloc,
                               const vector<float>& n_yloc,
                               const vector<float>& n_widths,
                               const vector<float>& n_heights,
                               float left_bound,
                               float right_bound,
                               float top_bound,
                               float bottom_bound)
   : _blocknum(n_xloc.size()),
     _xloc(n_xloc),
     _yloc(n_yloc),
     _widths(n_widths),
     _heights(n_heights),
     _epsilon(ShiftBlock::Infty),
     _leftBound(left_bound),
     _rightBound(right_bound),
     _topBound(top_bound),
     _bottomBound(bottom_bound)
{
   for (int i = 0; i < _blocknum; i++)
      _epsilon = std::min(_epsilon, std::min(_widths[i], _heights[i]));

   _epsilon /= basepacking_h::Dimension::Epsilon_Accuracy;
}
// --------------------------------------------------------
bool ShiftLegalizer::legalizeAll(ShiftLegalizer::AlgoType algo,
                                 const vector<int>& checkBlks,
                                 vector<int>& badBlks)
{
   switch (algo)
   {
   case NAIVE:
      return naiveLegalize(checkBlks, badBlks);
      
   default:
      cout << "ERROR: invalid algo specified in ShiftLegalizer::legalizeAll()"
           << endl;
      exit(1);
      break;
   }
}
// --------------------------------------------------------
bool ShiftLegalizer::naiveLegalize(const vector<int>& checkBlks,
                                   vector<int>& badBlks)
{
   int checkBlkNum = checkBlks.size();
   bool success = true;
   
   for (int i = 0; i < checkBlkNum; i++)
      putBlockIntoCore(i);

   for (int i = 0; i < checkBlkNum; i++)
   {
      bool thisSuccess = legalizeBlock(checkBlks[i]);
      if (!thisSuccess)
         badBlks.push_back(checkBlks[i]);
      success = success && thisSuccess;
   }
   return success;
}
// --------------------------------------------------------
bool ShiftLegalizer::legalizeBlock(int currBlk)
{
   printf("-----Block %d-----\n", currBlk);
   
   // move only if it resolves ALL overlaps
   ShiftBlock shift_block(_xloc, _yloc, _widths, _heights,
                          _leftBound, _rightBound,
                          _topBound, _bottomBound);
   vector<ShiftBlock::ShiftInfo> shiftinfo;
   shift_block(currBlk, shiftinfo);

   bool existsOverlaps = false;
   bool success = false; // only makes sense when "existsOverlaps"
   ShiftRotateDecision decision; // initialized
   for (int i = 0; i < ShiftBlock::DIR_NUM; i++)
   {
      // if overlap occurs
      if (std::abs(shiftinfo[i].shiftRangeMin) > _epsilon)
      {
         bool tempSuccess = shiftDecider(ShiftBlock::Directions(i),
                                         shiftinfo, decision);

         decision.rotate = false;
         success = success || tempSuccess;
         existsOverlaps = true;
      }
   }

   // -----try rotating the block if no move works so far-----
   if (existsOverlaps && !success)
   {
      // temporarily change the info
      std::swap(_widths[currBlk], _heights[currBlk]);
      float orig_xloc = _xloc[currBlk];
      float orig_yloc = _yloc[currBlk];

      // apply the same procedures to find moves
      ShiftBlock shift_block_rotated(_xloc, _yloc, _widths, _heights,
                                     _leftBound, _rightBound,
                                     _topBound, _bottomBound);
      vector<ShiftBlock::ShiftInfo> shiftinfo_rotated;
      shift_block_rotated(currBlk, shiftinfo_rotated);

      existsOverlaps = false;
      for (int i = 0; i < ShiftBlock::DIR_NUM; i++)
      {
         // if overlap occurs
         if (std::abs(shiftinfo_rotated[i].shiftRangeMin) > _epsilon)
         {
            bool tempSuccess = shiftDecider(ShiftBlock::Directions(i),
                                            shiftinfo_rotated, decision);

            decision.rotate = true;
            success = success || tempSuccess;
            existsOverlaps = true;
         }
      }

      // special case when "rotating" helps already
      if (!existsOverlaps)
      {
         decision.rotate = true;
         decision.shiftDir = ShiftBlock::EAST;
         decision.shiftExtent = 0;
         
         success = true;
      }

      // restore the global details even when succeed
      std::swap(_widths[currBlk], _heights[currBlk]);
      _xloc[currBlk] = orig_xloc;
      _yloc[currBlk] = orig_yloc;
   }

   // move towards the direction that needs least fix
   if (decision.shiftDir != Undefined)
   {
      adjustBlock(currBlk, decision);
      return true;
   }
   else if (!existsOverlaps)
      return true;
   else
      return false;
}
// --------------------------------------------------------
bool ShiftLegalizer::shiftDecider(
   ShiftBlock::Directions currDir,
   const vector<ShiftBlock::ShiftInfo>& shiftinfo,
   ShiftRotateDecision& decision) const
{
   if (shiftinfo[currDir].shiftRangeMin <
       shiftinfo[currDir].shiftRangeMax + _epsilon)
   {
      if (shiftinfo[currDir].shiftRangeMin < decision.shiftExtent)
      {
         decision.shiftDir = currDir;
         decision.shiftExtent = shiftinfo[currDir].shiftRangeMin;
      }
      return true;
   }
   else
      return false; // "f" ==> exists unresolvable overlap (not converse)
}
// --------------------------------------------------------
void ShiftLegalizer::adjustBlock(int currBlk,
                                 const ShiftRotateDecision& decision)
{
   if (decision.rotate)
      std::swap(_widths[currBlk], _heights[currBlk]);
   
   switch (decision.shiftDir)
   {
   case ShiftBlock::NORTH:
      _yloc[currBlk] += decision.shiftExtent;
      break;

   case ShiftBlock::EAST:
      _xloc[currBlk] -= decision.shiftExtent;
      break;

   case ShiftBlock::SOUTH:
      _yloc[currBlk] -= decision.shiftExtent;
      break;

   case ShiftBlock::WEST:
      _xloc[currBlk] += decision.shiftExtent;
      break;

   default:
      cout << "ERROR: invalid direction specified in moveblock()"
           << endl;
      exit(1);
   }
}
// --------------------------------------------------------
void ShiftLegalizer::putBlockIntoCore(int currBlk)
{
   _xloc[currBlk] = std::max(_xloc[currBlk], _leftBound);
   _xloc[currBlk] = std::min(_xloc[currBlk], _rightBound-_widths[currBlk]);

   _yloc[currBlk] = std::max(_yloc[currBlk], _bottomBound);
   _yloc[currBlk] = std::min(_yloc[currBlk], _topBound-_heights[currBlk]);
}
// --------------------------------------------------------
ShiftBlock::ShiftBlock(const vector<float>& xloc,
                       const vector<float>& yloc,
                       const vector<float>& widths,
                       const vector<float>& heights,
                       float left_bound,
                       float right_bound,
                       float top_bound,
                       float bottom_bound)
   : _blocknum(xloc.size()),
     _xStart(_blocknum+4),
     _xEnd(_blocknum+4),
     _yStart(_blocknum+4),
     _yEnd(_blocknum+4),
     _epsilon(Infty)
{
   for (int i = 0; i < _blocknum; i++)
   {
      _xStart[i] = xloc[i];
      _xEnd[i] = xloc[i] + widths[i];
      _yStart[i] = yloc[i];
      _yEnd[i] = yloc[i] + heights[i];

      _epsilon = std::min(_epsilon, std::min(widths[i], heights[i]));
   } 
   _epsilon /= basepacking_h::Dimension::Epsilon_Accuracy;  

   // LEFT-edge
   _xStart[_blocknum] = Neg_Infty;
   _xEnd[_blocknum] = left_bound;
   _yStart[_blocknum] = Neg_Infty;
   _yEnd[_blocknum] = Infty;

   // BOTTOM-edge
   _xStart[_blocknum+1] = Neg_Infty;
   _xEnd[_blocknum+1] = Infty;
   _yStart[_blocknum+1] = Neg_Infty;
   _yEnd[_blocknum+1] = bottom_bound;

   // RIGHT-edge
   _xStart[_blocknum+2] = right_bound;
   _xEnd[_blocknum+2] = Infty;
   _yStart[_blocknum+2] = Neg_Infty;
   _yEnd[_blocknum+2] = Infty;
      
   // TOP-edge
   _xStart[_blocknum+3] = Neg_Infty;
   _xEnd[_blocknum+3] = Infty;
   _yStart[_blocknum+3] = top_bound;
   _yEnd[_blocknum+3] = Infty;
}
// --------------------------------------------------------
void ShiftBlock::operator ()(int currBlk,
                             vector<ShiftInfo>& shiftinfo) const
{
   // -----initialize the output-----
   shiftinfo.resize(DIR_NUM);
   for (int i = 0; i < int(DIR_NUM); i++)
   {
      Directions currDir = Directions(i);
      shiftinfo[currDir].shiftRangeMin = 0;
      shiftinfo[currDir].shiftRangeMax = Infty;      
      shiftinfo[currDir].overlapMin = Infty;
      shiftinfo[currDir].overlapMax = 0;
   }

   // -----determine the overlap's and/or shiftRange-----
   for (int tempBlk = 0; tempBlk < _blocknum+4; tempBlk++)
   {
      // printf("-----tempBlk %d-----\n", tempBlk);
      if (tempBlk != currBlk)
      {
         if (_xStart[tempBlk] < _xEnd[currBlk] - _epsilon &&
             _xStart[currBlk] < _xEnd[tempBlk] - _epsilon)
         {
            // horizontal overlap, relevant for dir's N and S
            if (_yStart[tempBlk] < _yEnd[currBlk] - _epsilon &&
                _yStart[currBlk] < _yEnd[tempBlk] - _epsilon)
            {
               // real overlaps, "shiftRange(Min/Max)" must be +ve
               shiftinfo[NORTH].shiftRangeMin =
                  std::max(shiftinfo[NORTH].shiftRangeMin,
                      _yEnd[tempBlk] - _yStart[currBlk]);
               
               shiftinfo[SOUTH].shiftRangeMin =
                  std::max(shiftinfo[SOUTH].shiftRangeMin,
                      _yEnd[currBlk] - _yStart[tempBlk]);

               shiftinfo[EAST].shiftRangeMin =
                  std::max(shiftinfo[EAST].shiftRangeMin,
                      _xEnd[currBlk] - _xStart[tempBlk]);

               shiftinfo[WEST].shiftRangeMin =
                  std::max(shiftinfo[WEST].shiftRangeMin,
                      _xEnd[tempBlk] - _xStart[currBlk]);
               
               // handle "overlap(Min/Max)"
               shiftinfo[NORTH].overlapMin =
                  std::min(shiftinfo[NORTH].overlapMin, _xStart[tempBlk]);
               
               shiftinfo[NORTH].overlapMax =
                  std::max(shiftinfo[NORTH].overlapMax, _xEnd[tempBlk]);

               shiftinfo[SOUTH].overlapMin = shiftinfo[NORTH].overlapMin;
               shiftinfo[SOUTH].overlapMax = shiftinfo[NORTH].overlapMax;

               shiftinfo[EAST].overlapMin =
                  std::min(shiftinfo[EAST].overlapMin, _yStart[tempBlk]);

               shiftinfo[EAST].overlapMax =
                  std::max(shiftinfo[EAST].overlapMax, _yEnd[tempBlk]);

               shiftinfo[WEST].overlapMin = shiftinfo[EAST].overlapMin;
               shiftinfo[WEST].overlapMax = shiftinfo[EAST].overlapMax;
               // cout << "here -1- real overlap" << endl;
            }
            else if (_yStart[tempBlk] < _yEnd[currBlk] - _epsilon) 
            {
               // "tempBlk" below "currBlk"
               shiftinfo[SOUTH].shiftRangeMax =
                  std::min(shiftinfo[SOUTH].shiftRangeMax,
                      std::max(_yStart[currBlk]-_yEnd[tempBlk], float(0.0)));
               // cout << "here -2- tempBlk below currBlk" << endl;
            }
            else
            {
               // "tempBlk" above "currBlk"
               shiftinfo[NORTH].shiftRangeMax =
                  std::min(shiftinfo[NORTH].shiftRangeMax,
                      std::max(_yStart[tempBlk]-_yEnd[currBlk], float(0.0)));
               // cout << "here -3- tempBlk above currBlk" << endl;
            }
         }
         else if (_yStart[tempBlk] < _yEnd[currBlk] - _epsilon &&
                  _yStart[currBlk] < _yEnd[tempBlk] - _epsilon)
         {
            // vertical overlap, but not horizontal
            if (_xStart[tempBlk] < _xEnd[currBlk] - _epsilon)
            {
               // "tempBlk" left of "currBlk"
               shiftinfo[EAST].shiftRangeMax =
                  std::min(shiftinfo[EAST].shiftRangeMax,
                      std::max(_xStart[currBlk]-_xEnd[tempBlk], float(0.0)));
               // cout << "here -4- tempBlk left of currBlk" << endl;
            }
            else
            {
               // "tempBlk" right of "currBlk"
               shiftinfo[WEST].shiftRangeMax =
                  std::min(shiftinfo[WEST].shiftRangeMax,
                      std::max(_xStart[tempBlk]-_xEnd[currBlk], float(0.0)));
               // cout << "here -5- tempBlk right of currBlk" << endl;
            }
         }
      }
   }
}
// --------------------------------------------------------
void OutputShiftInfo(std::ostream& outs,
                     const vector<ShiftBlock::ShiftInfo>& shiftinfo)
{
   outs << "NORTH: " << endl;
   outs << "shiftRangeMin: "
        << shiftinfo[ShiftBlock::NORTH].shiftRangeMin << endl;
   outs << "shiftRangeMax: "
        << shiftinfo[ShiftBlock::NORTH].shiftRangeMax << endl;
   outs << "overlapMin: "
        << shiftinfo[ShiftBlock::NORTH].overlapMin << endl;
   outs << "overlapMax: "
        << shiftinfo[ShiftBlock::NORTH].overlapMax << endl;
   outs << endl;
   
   outs << "EAST: " << endl;
   outs << "shiftRangeMin: "
        << shiftinfo[ShiftBlock::EAST].shiftRangeMin << endl;
   outs << "shiftRangeMax: "
        << shiftinfo[ShiftBlock::EAST].shiftRangeMax << endl;
   outs << "overlapMin: "
        << shiftinfo[ShiftBlock::EAST].overlapMin << endl;
   outs << "overlapMax: "
        << shiftinfo[ShiftBlock::EAST].overlapMax << endl;
   outs << endl;
   
   outs << "SOUTH: " << endl;
   outs << "shiftRangeMin: "
        << shiftinfo[ShiftBlock::SOUTH].shiftRangeMin << endl;
   outs << "shiftRangeMax: "
        << shiftinfo[ShiftBlock::SOUTH].shiftRangeMax << endl;
   outs << "overlapMin: "
        << shiftinfo[ShiftBlock::SOUTH].overlapMin << endl;
   outs << "overlapMax: "
        << shiftinfo[ShiftBlock::SOUTH].overlapMax << endl;
   outs << endl;
   
   outs << "WEST: " << endl;
   outs << "shiftRangeMin: "
        << shiftinfo[ShiftBlock::WEST].shiftRangeMin << endl;
   outs << "shiftRangeMax: "
        << shiftinfo[ShiftBlock::WEST].shiftRangeMax << endl;
   outs << "overlapMin: "
        << shiftinfo[ShiftBlock::WEST].overlapMin << endl;
   outs << "overlapMax: "
        << shiftinfo[ShiftBlock::WEST].overlapMax << endl;
   outs << endl;
   outs << "--------" << endl;
}
// --------------------------------------------------------

      
   
   
