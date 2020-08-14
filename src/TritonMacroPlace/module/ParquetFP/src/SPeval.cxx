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


#include <ABKCommon/abkcommon.h>
#include "SPeval.h"
#include "PlToSP.h"
#include <algorithm>

using std::cout;
using std::endl;
using std::map;
using std::vector;

using namespace parquetfp;

SPeval::SPeval(const vector<float>& heights,
               const vector<float>& widths,
               bool paramUseFastSP)
{
   _heights=heights;
   _widths=widths;
  
   unsigned size = heights.size();
   _match.resize(size);
   _LL.resize(size);
   _reverseXX.resize(size);
   _reverseYY.resize(size);
   xloc.resize(size);
   yloc.resize(size);
   xlocRev.resize(size);
   ylocRev.resize(size);
   xSlacks.resize(size);
   ySlacks.resize(size);
   xlocRev.resize(size);
   ylocRev.resize(size);
   _TCGMatrixInitialized = false;
   _paramUseFastSP=paramUseFastSP;
}

void SPeval::_initializeTCGMatrix(unsigned size)
{
   _TCGMatrixVert.resize(size);
   _TCGMatrixHoriz.resize(size);
   for(unsigned i=0; i<size; ++i)
   {
      _TCGMatrixVert[i].resize(size);
      _TCGMatrixHoriz[i].resize(size);
   }
   _TCGMatrixInitialized = true;
}

float SPeval::_lcsCompute(const vector<unsigned>& X,
                           const vector<unsigned>& Y,
                           const vector<float>& weights,
                           vector<unsigned>& match,
                           vector<float>& P,
                           vector<float>& L)
{
   unsigned size = X.size();
   for(unsigned i=0;i<size;++i)
   {
      match[Y[i]]=i;
      L[i]=0;
   }
  
   float t;
   unsigned j;
   for(unsigned i=0;i<size;++i)
   {
      unsigned p = match[X[i]];
      P[X[i]]=L[p];
      t = P[X[i]]+weights[X[i]];
      
      for(j=p;j<size;++j)
      {
         if(t>L[j])
            L[j]=t;
         else
            break;
      }
   }
   return L[size-1];
}

float SPeval::xEval(const vector<unsigned>& XX, const vector<unsigned>& YY,
                    bool leftpack, vector<float>& xcoords)
{
   fill(_match.begin(),_match.end(),0);
   if(leftpack) {
     return _lcsCompute( XX, YY, _widths, _match, xcoords, _LL);
   }
   else {
     float xspan = _lcsReverseCompute( XX, YY, _widths, _match, xcoords, _LL);
     for(unsigned i = 0; i < XX.size(); ++i)
     {
       xcoords[i] = xspan - xcoords[i] - _widths[i];
     }
     return xspan;
   }
}

float SPeval::xEvalCompact(const vector<unsigned>& XX, const vector<unsigned>& YY,
                           bool leftpack, vector<float>& xcoords, vector<float>& ycoords)
{
   fill(_match.begin(),_match.end(),0);
   if(leftpack) {
     return _lcsComputeCompact( XX, YY, _widths, _match, xcoords, _LL, ycoords, _heights);
   }
   else {
     float xspan = _lcsComputeCompact( _reverseXX, _reverseYY, _widths, _match, xcoords,
                                       _LL, ycoords, _heights);
     for(unsigned i = 0; i < XX.size(); ++i)
     {
       xcoords[i] = xspan - xcoords[i] - _widths[i];
     }
     return xspan;
   }
}

float SPeval::xEvalFast(const vector<unsigned>& XX, const vector<unsigned>& YY,
                        bool leftpack, vector<float>& xcoords)
{
   fill(_match.begin(),_match.end(),0);
   if(leftpack) {
     return _lcsComputeFast( XX, YY, _widths, _match, xcoords, _LL);
   }
   else {
     float xspan = _lcsComputeFast( _reverseXX, _reverseYY, _widths, _match, xcoords, _LL);
     for(unsigned i = 0; i < XX.size(); ++i)
     {
       xcoords[i] = xspan - xcoords[i] - _widths[i];
     }
     return xspan;
   }
}

float SPeval::yEval(const vector<unsigned>& XX, const vector<unsigned>& YY,
                    bool botpack, vector<float>& ycoords)
{
   fill(_match.begin(),_match.end(),0);
   if(botpack) {
     return _lcsCompute( _reverseXX, YY, _heights, _match, ycoords, _LL);
   }
   else {
     float yspan = _lcsReverseCompute( _reverseXX, YY, _heights, _match, ycoords, _LL);
     for(unsigned i = 0; i < XX.size(); ++i)
     {
       ycoords[i] = yspan - ycoords[i] - _heights[i];
     }
     return yspan;
   }
}

float SPeval::yEvalCompact(const vector<unsigned>& XX, const vector<unsigned>& YY,
                           bool botpack, vector<float>& xcoords, vector<float>& ycoords)
{
   fill(_match.begin(),_match.end(),0);
   if(botpack) {
     return _lcsComputeCompact( _reverseXX, YY, _heights, _match, ycoords, _LL,
                                xcoords, _widths);
   }
   else {
     float yspan = _lcsComputeCompact( XX, _reverseYY, _heights, _match, ycoords, _LL,
                                       xcoords, _widths);
     for(unsigned i = 0; i < YY.size(); ++i)
     {
       ycoords[i] = yspan - ycoords[i] - _heights[i];
     }
     return yspan;
   }
}

float SPeval::yEvalFast(const vector<unsigned>& XX, const vector<unsigned>& YY,
                        bool botpack, vector<float>& ycoords)
{
   fill(_match.begin(),_match.end(),0);
   if(botpack) {
     return _lcsComputeFast( _reverseXX, YY, _heights, _match, ycoords, _LL);
   }
   else {
     float yspan = _lcsComputeFast( XX, _reverseYY, _heights, _match, ycoords, _LL);
     for(unsigned i = 0; i < YY.size(); ++i)
     {
       ycoords[i] = yspan - ycoords[i] - _heights[i];
     }
     return yspan;
   }
}

void SPeval::evaluate(const vector<unsigned>& X,
                      const vector<unsigned>& Y,
                      bool leftpack, bool botpack)
{
   if(_paramUseFastSP)
   {
      evaluateFast(X, Y, leftpack, botpack);
      return;
   }
  
   reverse_copy(X.begin(), X.end(), _reverseXX.begin());
   xSize = xEval(X,Y,leftpack,xloc);
   ySize = yEval(X,Y,botpack, yloc);
}

void SPeval::evaluateFast(const vector<unsigned>& X,
                          const vector<unsigned>& Y,
                          bool leftpack, bool botpack)
{
   if(!leftpack || botpack)
   {
     reverse_copy(X.begin(), X.end(), _reverseXX.begin());
   }
   if(!leftpack || !botpack)
   {
     reverse_copy(Y.begin(), Y.end(), _reverseYY.begin());
   }
   xSize = xEvalFast(X,Y,leftpack,xloc);
   ySize = yEvalFast(X,Y,botpack,yloc);
}

void SPeval::evaluateCompact(const vector<unsigned>& X,
                             const vector<unsigned>& Y,
                             bool whichDir, bool leftpack, bool botpack)
{
   if(whichDir)  //evaluate xloc first and then compact
   {
      if(!leftpack || botpack)
      {
        reverse_copy(X.begin(), X.end(), _reverseXX.begin());
      }
      if(!leftpack || !botpack)
      {
        reverse_copy(Y.begin(), Y.end(), _reverseYY.begin());
      }
      xEval(X,Y,leftpack,xloc);
      ySize = yEvalCompact(X,Y,botpack,xloc,yloc);
      xSize = xEvalCompact(X,Y,leftpack,xloc,yloc);
   }
   else          //evaluate yloc first and then compact
   {
      reverse_copy(X.begin(), X.end(), _reverseXX.begin());
      if(!leftpack || !botpack)
      {
        reverse_copy(Y.begin(), Y.end(), _reverseYY.begin());
      }
      yEval(X,Y,botpack,yloc);
      xSize = xEvalCompact(X,Y,leftpack,xloc,yloc);
      ySize = yEvalCompact(X,Y,botpack,xloc,yloc);
   }
}

void SPeval::evalSlacks(const vector<unsigned>& X,
                        const vector<unsigned>& Y)
{
   if(_paramUseFastSP)
   {
      evalSlacksFast(X, Y);
      return;
   }

   reverse_copy(X.begin(), X.end(), _reverseXX.begin());

   bool leftpack = true;
   xSize = xEval(X,Y,leftpack,xloc);
           xEval(X,Y,!leftpack,xlocRev);

   bool botpack = true;
   ySize = yEval(X,Y,botpack,yloc);
           yEval(X,Y,!botpack,ylocRev);

   for(unsigned i = 0; i < X.size(); ++i)
   {
      xSlacks[i] = (xlocRev[i] - xloc[i])*100./xSize;
      ySlacks[i] = (ylocRev[i] - yloc[i])*100./ySize;
   }
}

void SPeval::evalSlacksFast(const vector<unsigned>& X,
                            const vector<unsigned>& Y)
{
   reverse_copy(X.begin(), X.end(), _reverseXX.begin());
   reverse_copy(Y.begin(), Y.end(), _reverseYY.begin());

   bool leftpack = true;
   xSize = xEvalFast(X,Y,leftpack,xloc);
           xEvalFast(X,Y,!leftpack,xlocRev);

   bool botpack = true;
   ySize = yEvalFast(X,Y,botpack,yloc);
           yEvalFast(X,Y,!botpack,ylocRev);

   for(unsigned i = 0; i < X.size(); ++i)
   {
      xSlacks[i] = (xlocRev[i] - xloc[i])*100./xSize;
      ySlacks[i] = (ylocRev[i] - yloc[i])*100./ySize;
   }
}

float SPeval::_lcsComputeCompact(const vector<unsigned>& X,
                                 const vector<unsigned>& Y,
                                 const vector<float>& weights,
                                 vector<unsigned>& match,
                                 vector<float>& P,
                                 vector<float>& L,
                                 vector<float>& oppLocs,
                                 vector<float>& oppWeights)
{
   float finalSize = -std::numeric_limits<float>::max();
   unsigned size = X.size();
   for(unsigned i=0;i<size;++i)
   {
      match[Y[i]]=i;
      L[i]=0;
   }
  
   float t;
   unsigned j;
   for(unsigned i=0;i<size;++i)
   {
      unsigned p = match[X[i]];
      P[X[i]]=L[p];
      t = P[X[i]]+weights[X[i]];
      
      float iStart = oppLocs[X[i]];
      float iEnd = oppLocs[X[i]] + oppWeights[X[i]];

      for(j=p;j<size;++j)
      {
         float jStart = oppLocs[Y[j]];
         float jEnd = oppLocs[Y[j]] + oppWeights[Y[j]];
	  
         if(iStart >= jEnd || iEnd <= jStart) //no constraint
            continue;
	  
         if(t>L[j])
         {
            L[j]=t;
            if(t > finalSize)
               finalSize = t;
         }
      }
   }
   return finalSize;
}

void SPeval::computeConstraintGraphs(const vector<unsigned>& X, const vector<unsigned>& Y)
{
   unsigned size = X.size();
   if(!_TCGMatrixInitialized)
      _initializeTCGMatrix(size);

   vector<unsigned> matchX;
   vector<unsigned> matchY;
   matchX.resize(size);
   matchY.resize(size);

   for(unsigned i=0;i<size;++i)
   {
      matchX[X[i]]=i;
      matchY[Y[i]]=i;
   }

   for(unsigned i=0;i<size;++i)
   {
      for(unsigned j=0; j<size; ++j)
      {
         if(i==j)
         {
            _TCGMatrixHoriz[i][j] = 1;
            _TCGMatrixVert[i][j] = 1;
            continue;
         }

         _TCGMatrixHoriz[i][j] = 0;
         _TCGMatrixVert[i][j] = 0;
         _TCGMatrixHoriz[j][i] = 0;
         _TCGMatrixVert[j][i] = 0;

	  
         if(matchX[i] < matchX[j] && matchY[i] < matchY[j])
            _TCGMatrixHoriz[i][j] = 1;
         else if(matchX[i] > matchX[j] && matchY[i] > matchY[j])
            _TCGMatrixHoriz[j][i] = 1;
         else if(matchX[i] < matchX[j] && matchY[i] > matchY[j])
            _TCGMatrixVert[j][i] = 1;
         else if(matchX[i] > matchX[j] && matchY[i] < matchY[j])
            _TCGMatrixVert[i][j] = 1;
         else
            cout<<"ERROR: in computeConstraintGraph \n";
      }
   }
}

void SPeval::removeRedundantConstraints(const vector<unsigned>& X, const vector<unsigned>& Y, bool knownDir)
{
   unsigned size = X.size();
   float iStart, iEnd, jStart, jEnd;
   for(unsigned i=0; i<size; ++i)
   {
      if(knownDir == 0) //horizontal
      {
         iStart = xloc[i];
         iEnd = iStart+_widths[i];
      }
      else  //vertical
      {
         iStart = yloc[i];
         iEnd = iStart+_heights[i];
      }
      for(unsigned j=0; j<size; ++j)
      {
         if(i == j)
            continue;

         if(knownDir == 0)
         {
            jStart = xloc[j];
            jEnd = jStart+_widths[j];
         }
         else
         {
            jStart = yloc[j];
            jEnd = jStart+_heights[j];
         }
	  
         if(knownDir == 0)
         {
            if(_TCGMatrixVert[i][j] == 1)
            {
               if(iStart >= jEnd || iEnd <= jStart) //no constraint
               {
                  _TCGMatrixVert[i][j] = 0;
                  if(iStart < jStart)
                     _TCGMatrixHoriz[i][j] = 1;
                  else
                     _TCGMatrixHoriz[j][i] = 1;
               }
            }
         }
         else
         {
            if(_TCGMatrixHoriz[i][j] == 1)
            {
               if(iStart >= jEnd || iEnd <= jStart) //no constraint
               {
                  cout<<i<<"\t"<<j<<"\t"<<iStart<<"\t"<<iEnd<<"\t"<<
                     jStart<<"\t"<<jEnd<<endl;
                  _TCGMatrixHoriz[i][j] = 0;
                  if(iStart < jStart)
                     _TCGMatrixVert[i][j] = 1;
                  else
                     _TCGMatrixVert[j][i] = 1;
               }
            }
         }
      }
   }
}

void SPeval::computeSPFromCG(vector<unsigned>& X, vector<unsigned>& Y)
{
   unsigned size = X.size();
   for(unsigned i=0; i<size; ++i)
   {
      X[i] = i;
      Y[i] = i;
   }
  
   SPXRelation SPX(_TCGMatrixHoriz, _TCGMatrixVert);
   SPYRelation SPY(_TCGMatrixHoriz, _TCGMatrixVert);

   std::sort(X.begin(), X.end(), SPX);
   std::sort(Y.begin(), Y.end(), SPY);
}

void SPeval::changeWidths(const vector<float>& widths)
{
   _widths = widths;
}

void SPeval::changeHeights(const vector<float>& heights)
{
   _heights = heights;
}

void SPeval::changeNodeWidth(unsigned index, float width)
{
   _widths[index] = width;
}

void SPeval::changeNodeHeight(unsigned index, float height)
{
   _heights[index] = height;
}

void SPeval::changeOrient(unsigned index)
{
   float tempWidth = _heights[index];
   _heights[index] = _widths[index];
   _widths[index] = tempWidth;
}


float SPeval::_lcsReverseCompute(const vector<unsigned>& X,
                                  const vector<unsigned>& Y,
                                  const vector<float>& weights,
                                  vector<unsigned>& match,
                                  vector<float>& P,
                                  vector<float>& L
   )
{
   unsigned size = X.size();
   for(unsigned i=0;i<size;++i)
   {
      match[Y[i]]=i;
      L[i]=0;
   }
  
   float t;
   int j;
   for(int i=size-1;i>=0;--i)
   {
      unsigned p = match[X[i]];
      P[X[i]]=L[p];
      t = P[X[i]]+weights[X[i]];
      
      for(j=p;j>=0;--j)
      {
         if(t>L[j])
            L[j]=t;
         else
            break;
      }
   }
   return L[0];
}

void SPeval::_discardNodesBST(unsigned index, float length)
{
   map<unsigned , float>::iterator iter;
   map<unsigned , float>::iterator nextIter;
   map<unsigned , float>::iterator endIter;
   endIter = _BST.end();
   iter = _BST.find(index);
   nextIter = iter;
   ++nextIter;
   if(nextIter != _BST.end())
   {
      ++iter;
      while(true)
      {
         ++nextIter;
         if((*iter).second < length)
            _BST.erase(iter);
         if(nextIter == endIter)
            break;
         iter = nextIter;
      }
   }
}

float SPeval::_findBST(unsigned index)
{
   map<unsigned , float>::iterator iter;  
   float loc;
   iter = _BST.lower_bound(index);
   if(iter != _BST.begin())
   {
      iter--;
      loc = (*iter).second;
   }
   else
      loc = 0;
   return loc;
}

float SPeval::_lcsComputeFast(const vector<unsigned>& X,
                               const vector<unsigned>& Y,
                               const vector<float>& weights,
                               vector<unsigned>& match,
                               vector<float>& P,
                               vector<float>& L
   )
{
   _BST.clear();
   _BST[0] = 0;
   unsigned size = X.size();
   for(unsigned i=0;i<size;++i)
   {
      match[Y[i]]=i;
   }
  
   float t;
//unsigned j;
   for(unsigned i=0;i<size;++i)
   {
      unsigned p = match[X[i]];
      P[X[i]]=_findBST(p);
      t = P[X[i]]+weights[X[i]];
      _BST[p] = t;
      _discardNodesBST(p,t);
   }
   float length = _findBST(size);
   return length;
}
