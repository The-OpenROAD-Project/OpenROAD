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


// 040607 hhchan added dummy base class "BaseAnnealer"
//               void go() -> virtual bool go() [.h and .cxx]

#ifndef ANNEALER_H
#define ANNEALER_H

#include "FPcommon.h"
#include "DB.h"
#include "SeqPair.h"
#include "SPeval.h"
#include "AnalytSolve.h"
#include "baseannealer.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace parquetfp
{
   class Command_Line;
   class Annealer : public BaseAnnealer
   {
   protected:
      SeqPair *const _sp;
      SPeval *const _spEval;
      MaxMem *_maxMem;
  
      std::vector<Point> sortedXSlacks;
      std::vector<Point> sortedYSlacks;

      Annealer()
         : BaseAnnealer(), _sp(NULL), _spEval(NULL), _maxMem(NULL) {}

   public:
      Annealer(const Command_Line *const params, DB *const db, MaxMem *maxMem);

      virtual ~Annealer();
      void parseConfig();

      virtual bool go();
      void anneal();
      virtual bool packOneBlock() { return true; }

      // compacts current (init/final) soln
      virtual void compactSoln(bool minWL, bool fixedOutline, float reqdH, float reqdW);
      virtual void takePlfromDB(); // takeSPformDB() + eval()
      void takeSPfromDB();         // converts the present pl to a seq-pair
  
      void eval();       // just evaluate the current SP and set up required
                         // data structures and values
      void evalSlacks(); // evaluate slacks
      void evalCompact(bool whichDir); // just evaluate the current SP with
                                       // compaction and set up required data
                                       // structures and values

      int makeMove(std::vector<unsigned>& tempX,
                   std::vector<unsigned>& tempY);
      int makeMoveSlacks(std::vector<unsigned>& tempX,
                         std::vector<unsigned>& tempY);
      int makeMoveSlacksOrient(std::vector<unsigned>& A, std::vector<unsigned>& B,
                               unsigned& index,               
                               parquetfp::ORIENT& oldOrient,  
                               parquetfp::ORIENT& newOrient); 
      int makeMoveOrient(unsigned& index,
                         parquetfp::ORIENT& oldOrient,
                         parquetfp::ORIENT& newOrient);
      int makeARMove(std::vector<unsigned>& A, std::vector<unsigned>& B,
                     float currAR);
      int makeSoftBlMove(const std::vector<unsigned>& A, const std::vector<unsigned>& B,
                         unsigned &index,
                         float &newWidth, float &newHeight);
      int makeIndexSoftBlMove(const std::vector<unsigned>& A, const std::vector<unsigned>& B,
                              unsigned index,
                              float &newWidth, float &newHeight);

      int makeHPWLMove(std::vector<unsigned>& tempX, std::vector<unsigned>& tempY);
      int makeARWLMove(std::vector<unsigned>& tempX, std::vector<unsigned>& tempY, 
                       float currAR);

      void sortSlacks(std::vector<Point>& sortedXSlacks,
                      std::vector<Point>& sortedYSlacks);

      float getXSize(); // requires that spEvaluations be done earlier
      float getYSize(); // requires that spEvaluations be done earlier

      int packSoftBlocks(unsigned numIter); // need to update DB after this
      void updatePlacement(); // update the placement of DB with spEval info
   };

   // used for std::sort function to sort slacks
   struct sort_slacks
   {
      bool operator()(Point pt1, Point pt2)
         {
            return (pt1.x < pt2.x);
         }
   };
}

#endif 
