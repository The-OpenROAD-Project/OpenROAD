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

#ifndef PLTOSP_H
#define PLTOSP_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <vector>

namespace parquetfp {
enum PL2SP_ALGO
{
  NAIVE_ALGO,
  TCG_ALGO
};

class Pl2SP
{
 private:
  std::vector<float> _xloc;
  std::vector<float> _yloc;
  std::vector<float> _widths;
  std::vector<float> _heights;

  std::vector<unsigned> _XX;
  std::vector<unsigned> _YY;

  int _cnt;

 public:
  Pl2SP(std::vector<float>& xloc,
        std::vector<float>& yloc,
        std::vector<float>& widths,
        std::vector<float>& heights,
        PL2SP_ALGO whichAlgo);

  ~Pl2SP() {}

  void naiveAlgo(void);
  void TCGAlgo(void);

  // Floyd Marshal to find TCG
  void TCG_FM(std::vector<std::vector<bool>>& TCGMatrixHoriz,
              std::vector<std::vector<bool>>& TCGMatrixVert);

  // DP to find TCG
  void TCG_DP(std::vector<std::vector<bool>>& TCGMatrixHoriz,
              std::vector<std::vector<bool>>& TCGMatrixVert);
  void TCGDfs(std::vector<std::vector<bool>>& TCGMatrix,
              std::vector<std::vector<bool>>& adjMatrix,
              int v,
              std::vector<int>& pre);

  const std::vector<unsigned>& getXSP(void) const { return _XX; }

  const std::vector<unsigned>& getYSP(void) const { return _YY; }

  void print(void) const;
};

struct RowElem
{
  unsigned index;
  float xloc;
};

struct less_mag
{
  bool operator()(const RowElem& elem1, const RowElem& elem2)
  {
    return (elem1.xloc < elem2.xloc);
  }
};

class SPXRelation
{
  const std::vector<std::vector<bool>>& TCGMatrixHoriz;
  const std::vector<std::vector<bool>>& TCGMatrixVert;

 public:
  SPXRelation(const std::vector<std::vector<bool>>& TCGMatrixHorizIP,
              const std::vector<std::vector<bool>>& TCGMatrixVertIP)
      : TCGMatrixHoriz(TCGMatrixHorizIP), TCGMatrixVert(TCGMatrixVertIP)
  {
  }

  bool operator()(const unsigned i, const unsigned j) const
  {
    if (i == j) {
      return false;
    } else if (TCGMatrixHoriz[i][j]) {
      return true;
    } else if (TCGMatrixHoriz[j][i]) {
      return false;
    } else if (TCGMatrixVert[j][i]) {
      return true;
    } else if (TCGMatrixVert[i][j]) {
      return false;
    } else {
      // cout<<"ERROR IN PL2SP SPX "<<i<<"\t"<<j<<endl;
      if (i < j) {
        return true;
      } else {
        return false;
      }
    }
  }
};

class SPYRelation
{
  const std::vector<std::vector<bool>>& TCGMatrixHoriz;
  const std::vector<std::vector<bool>>& TCGMatrixVert;

 public:
  SPYRelation(const std::vector<std::vector<bool>>& TCGMatrixHorizIP,
              const std::vector<std::vector<bool>>& TCGMatrixVertIP)
      : TCGMatrixHoriz(TCGMatrixHorizIP), TCGMatrixVert(TCGMatrixVertIP)
  {
  }
  bool operator()(const unsigned i, const unsigned j) const
  {
    if (i == j) {
      return false;
    }
    if (TCGMatrixHoriz[i][j])
      return true;
    else if (TCGMatrixHoriz[j][i])
      return false;
    else if (TCGMatrixVert[j][i])
      return false;
    else if (TCGMatrixVert[i][j])
      return true;
    else {
      // cout<<"ERROR IN PL2SP SPY "<<i<<"\t"<<j<<endl;
      if (i < j)
        return true;
      else
        return false;
    }
  }
};
}  // namespace parquetfp
// using namespace parquetfp;

#endif
