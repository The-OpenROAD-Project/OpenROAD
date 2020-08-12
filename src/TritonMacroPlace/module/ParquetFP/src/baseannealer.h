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


#ifndef BASEANNEALER_H
#define BASEANNEALER_H

#include "CommandLine.h"
#include "DB.h"
#include "AnalytSolve.h"
#include "basepacking.h"
#include "ABKCommon/abkcommon.h"
#include <string>

// --------------------------------------------------------
class BaseAnnealer
{
public:
   BaseAnnealer(const parquetfp::Command_Line *const params,
                parquetfp::DB *const db);
   virtual ~BaseAnnealer();
   
   virtual bool go() = 0;           // go() == entire annealing process
   virtual bool packOneBlock() = 0; // floorplan only one block 
   
   virtual void takePlfromDB() = 0; // get init soln from *db
   virtual void solveQP();          // get a quad-minimum soln and update *db
   virtual void compactSoln(bool minWL, bool fixedOutline, float reqdH, float reqdW) = 0;
   void postHPWLOpt();

   inline float isFixedOutline() const;
   inline float outlineDeadspaceRatio() const;
   inline float outlineArea() const;
   inline float outlineWidth() const;
   inline float outlineHeight() const;

   // basic constants for readability
   static const int UNINITIALIZED;
   static const unsigned int UNSIGNED_UNINITIALIZED;
   static const int FREE_OUTLINE;
   static const int NOT_FOUND;

   enum MOVE_TYPES {MISC = -1,
                    NOOP = 0,
                    REP_SPEC_MIN = 1, // representation-specific
                    REP_SPEC_ORIENT = 2, 
                    REP_SPEC_MAX = 5,
                    SLACKS_MOVE = 6,
                    AR_MOVE = 7,
                    ORIENT = 10,
                    SOFT_BL = 11,
                    HPWL = 12,
                    ARWL = 13};

   class SolutionInfo
   {
   public:
      SolutionInfo() 
         : area(UNINITIALIZED),
           width(UNINITIALIZED),
           height(UNINITIALIZED),
           HPWL(UNINITIALIZED) {}

      float area;
      float width;
      float height;
      float HPWL;
   };      
   void printResults(const Timer& tm,
                     const SolutionInfo& curr) const;
   float annealTime;

protected:
   parquetfp::DB *const _db;                     // _db, _params behaves like
   const parquetfp::Command_Line *const _params; // references, use ptrs for
   parquetfp::AnalytSolve *const _analSolve;       // code backwd compatibility
   std::string _baseFileName;

   const bool _isFixedOutline;
   const float _outlineDeadspaceRatio;
   const float _outlineArea;
   const float _outlineWidth;
   const float _outlineHeight;

   BaseAnnealer()
      : _db(NULL), _params(NULL), _analSolve(NULL),
        _isFixedOutline(false),
        _outlineDeadspaceRatio(basepacking_h::Dimension::Infty),     
        _outlineArea(basepacking_h::Dimension::Infty),     
        _outlineWidth(basepacking_h::Dimension::Infty),     
        _outlineHeight(basepacking_h::Dimension::Infty)
      { /* compilerCheck(); */ }
};
// --------------------------------------------------------

inline float BaseAnnealer::isFixedOutline() const
{   return _isFixedOutline; }
// --------------------------------------------------------
inline float BaseAnnealer::outlineDeadspaceRatio() const
{   return _outlineDeadspaceRatio; }
// --------------------------------------------------------
inline float BaseAnnealer::outlineArea() const
{   return _outlineArea; }
// --------------------------------------------------------
inline float BaseAnnealer::outlineWidth() const
{   return _outlineWidth; }
// --------------------------------------------------------
inline float BaseAnnealer::outlineHeight() const
{   return _outlineHeight; }
// --------------------------------------------------------

#endif
