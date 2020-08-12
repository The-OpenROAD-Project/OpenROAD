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


#ifndef SPEVAL_H
#define SPEVAL_H

#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace parquetfp
{
   class SPeval
   {
   private:
      //buffers go in here
      std::vector<unsigned> _match;
      std::vector<unsigned> _reverseXX;
      std::vector<unsigned> _reverseYY;
      std::vector<float> _LL;      
      std::vector<float> _heights;
      std::vector<float> _widths;
      std::vector<float> _xlocRev;
      std::vector<float> _ylocRev;
      std::map<unsigned , float> _BST;  //for the O(nlogn) algo

      std::vector< std::vector<bool> > _TCGMatrixHoriz;
      std::vector< std::vector<bool> > _TCGMatrixVert;

      float _lcsCompute(const std::vector<unsigned>& X,
                        const std::vector<unsigned>& Y,
                        const std::vector<float>& weights,
                        std::vector<unsigned>& match,
                        std::vector<float>& P,
                        std::vector<float>& L
         );

      float _lcsReverseCompute(const std::vector<unsigned>& X,
                               const std::vector<unsigned>& Y,
                               const std::vector<float>& weights,
                               std::vector<unsigned>& match,
                               std::vector<float>& P,
                               std::vector<float>& L
         );
  
      float _lcsComputeCompact(const std::vector<unsigned>& X,
                               const std::vector<unsigned>& Y,
                               const std::vector<float>& weights,
                               std::vector<unsigned>& match,
                               std::vector<float>& P,
                               std::vector<float>& L,
                               std::vector<float>& oppLocs,
                               std::vector<float>& oppWeights
         );
  
      //fast are for the O(nlog n) algo
      float _findBST(unsigned index); //see the paper for definitions
      void _discardNodesBST(unsigned index, float length);

      float _lcsComputeFast(const std::vector<unsigned>& X,
                             const std::vector<unsigned>& Y,
                             const std::vector<float>& weights,
                             std::vector<unsigned>& match,
                             std::vector<float>& P,
                             std::vector<float>& L
         );


      bool _TCGMatrixInitialized;
      void _initializeTCGMatrix(unsigned size);
      bool _paramUseFastSP;

   public:
      std::vector<float> xloc;
      std::vector<float> yloc;
      std::vector<float> xlocRev;
      std::vector<float> ylocRev;
      float xSize;
      float ySize;
      std::vector<float> xSlacks;
      std::vector<float> ySlacks;


      SPeval(const std::vector<float>& heights,
             const std::vector<float>& widths,
             bool paramUseFastSP);

  
      void evaluate(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                    bool leftpack, bool botpack);      
      void evalSlacks(const std::vector<unsigned>& X, const std::vector<unsigned>& Y);
      void evaluateCompact(const std::vector<unsigned>& X,
                           const std::vector<unsigned>& Y, 
                           bool whichDir, bool leftpack, bool botpack);

      //following are for evaluating with the O(nlog n) scheme
      void evaluateFast(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                        bool leftpack, bool botpack);
      void evalSlacksFast(const std::vector<unsigned>& X, const std::vector<unsigned>& Y);
   private:

      float xEval(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                  bool leftpack, std::vector<float>& xcoords);
      float yEval(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                  bool botpack, std::vector<float>& ycoords);
      float xEvalCompact(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                         bool leftpack, std::vector<float>& xcoords, std::vector<float>& ycoords);
      float yEvalCompact(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                         bool botpack, std::vector<float>& xcoords, std::vector<float>& ycoords);
      void computeConstraintGraphs(const std::vector<unsigned>& X, const std::vector<unsigned>& Y);
      void removeRedundantConstraints(const std::vector<unsigned>& X, const std::vector<unsigned>& Y, bool knownDir);
      void computeSPFromCG(std::vector<unsigned>& X, std::vector<unsigned>& Y);

      //following are for evaluating with the O(nlog n) scheme
      float xEvalFast(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                      bool leftpack, std::vector<float>& xcoords);
      float yEvalFast(const std::vector<unsigned>& X, const std::vector<unsigned>& Y,
                      bool botpack,  std::vector<float>& ycoords);

   public:
      //miscelleneous functions
      void changeWidths(const std::vector<float>& widths);
      void changeHeights(const std::vector<float>& heights);
      void changeNodeWidth(const unsigned index, float width);
      void changeNodeHeight(const unsigned index, float height);
      void changeOrient(unsigned index);
   };
}
//using namespace parquetfp;

#endif
