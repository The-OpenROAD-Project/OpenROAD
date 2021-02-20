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


#include "FPcommon.h"
#include "PlToSP.h"

using namespace parquetfp;
using std::cout;
using std::endl;
using std::vector;

Pl2SP::Pl2SP(vector<float>& xloc, vector<float>& yloc, 
	     vector<float>& widths, vector<float>& heights, 
	     PL2SP_ALGO whichAlgo) : _xloc(xloc), _yloc(yloc), 
	       _widths(widths), _heights(heights)
{
  unsigned size = _xloc.size();
  if(_yloc.size() != size || _widths.size() != size || _heights.size() != size)
    cout<<"ERROR: in Pl2SP. Sizes do not match"<<endl;

  switch(whichAlgo)
    {
    case NAIVE_ALGO :
      naiveAlgo();
      break;
    case TCG_ALGO:
      TCGAlgo();
      break;
    }
}

void Pl2SP::naiveAlgo(void)
{
  unsigned size = _xloc.size();
  float rowHeight = std::numeric_limits<float>::max();
  float maxYLoc = -std::numeric_limits<float>::max();
  
  for(unsigned i=0; i<size; ++i)
   {
     if(rowHeight > _heights[i])
       rowHeight = _heights[i];
       
     if(maxYLoc < _yloc[i])
       maxYLoc = _yloc[i];
   }

  unsigned numRows = unsigned(ceil(maxYLoc/rowHeight)+1);

  //snap to y grid here
  for(unsigned i=0; i<size; ++i)
   {
     unsigned reqdRow = static_cast<unsigned>((_yloc[i]/rowHeight)+0.5);
     //unsigned reqdRow = unsigned(rint(_yloc[i]/rowHeight));
     _yloc[i] = reqdRow*rowHeight;
   }
  
  vector< vector<RowElem> > rows;
  vector<RowElem> singleRow;
  RowElem tempRE;
  
  float currHeight = 0;
  
  for(unsigned i=0; i<numRows; ++i)
   {
     for(unsigned j=0; j<size; ++j)
       { 
         if(std::abs(_yloc[j]-currHeight)<0.0001)
	   {
	     tempRE.index = j;
	     tempRE.xloc = _xloc[j];
	     singleRow.push_back(tempRE);
	   }
       }
       
     currHeight += rowHeight;

     std::stable_sort(singleRow.begin(), singleRow.end(), less_mag());
     rows.push_back(singleRow);
     singleRow.clear();
   }

   //form the X and Y sequence pairs now
   for(unsigned i=0; i<rows.size(); ++i)
    {
      for(unsigned j=0; j<rows[i].size(); ++j)
        {
	  _YY.push_back(rows[i][j].index);
	}
    }
   for(int i=rows.size()-1; i>=0; --i)
    {
      for(unsigned j=0; j<rows[i].size(); ++j)
        {
	  _XX.push_back(rows[i][j].index);
	}
    }

  if(_XX.size() != _xloc.size() || _YY.size() != _xloc.size())
   {
    cout<<"ERROR: generated sequence pair of not correct sizes "<<_XX.size()<<" & "<<_YY.size()<<" vs "<<size<<endl;
    print();
   }
    
}

void Pl2SP::TCGAlgo(void)
{
  unsigned size = _xloc.size();

  vector< vector<bool> > TCGMatrixVert;
  vector< vector<bool> > TCGMatrixHoriz;

  TCGMatrixVert.resize(size);
  TCGMatrixHoriz.resize(size);

  for(unsigned i=0; i<size; ++i)
    {
      TCGMatrixVert[i].resize(size, false);
      TCGMatrixHoriz[i].resize(size, false);
    }

  float ebsilon = std::numeric_limits<float>::max();
  for(unsigned i=0; i<size; ++i) //pick min dimension
    {
      if(ebsilon > _widths[i])
	ebsilon = _widths[i];
      if(ebsilon > _heights[i])
	ebsilon = _heights[i];
    }
  ebsilon /= 1000;

  //set up the immediate constraints
  for(unsigned i=0; i<size; ++i)
    {
      for(unsigned j=0; j<=i; ++j)
	{
	  float iXStart, iXEnd, jXStart, jXEnd;
	  float iYStart, iYEnd, jYStart, jYEnd;

	  float horizOverlap = 0.0f;
	  float vertOverlap = 0.0f;
	  unsigned vertOverlapDir = 0;
	  unsigned horizOverlapDir = 0;


	  if (i == j)
	    {
	      TCGMatrixHoriz[i][j] = 1;
	      TCGMatrixVert[i][j] = 1;
	      continue;
	    }

	  TCGMatrixHoriz[i][j] = 0;
	  TCGMatrixVert[i][j] = 0;
	  TCGMatrixHoriz[j][i] = 0;
	  TCGMatrixVert[j][i] = 0;

	  iXStart = _xloc[i];
	  iXEnd = _xloc[i] + _widths[i];
	  jXStart = _xloc[j];
	  jXEnd = _xloc[j] + _widths[j];

	  iYStart = _yloc[i];
	  iYEnd = _yloc[i] + _heights[i];
	  jYStart = _yloc[j];
	  jYEnd = _yloc[j] + _heights[j];

	  //horizontal constraint
	  if(jYStart < iYStart + ebsilon && jYEnd < iYEnd + ebsilon &&
	     jYEnd > iYStart + ebsilon)
	    {
	      vertOverlap = jYEnd - iYStart; //lower overlap
	      vertOverlapDir = 0; 
	    }
	  else if(jYStart > iYStart + ebsilon && jYEnd < iYEnd + ebsilon)
	    {
	      vertOverlap = jYEnd - jYStart; //inner overlap
	      if(iYEnd-jYEnd > jYStart-iYStart)
		vertOverlapDir = 0;
	      else
		vertOverlapDir = 1;
	    }
	  else if(jYStart > iYStart + ebsilon && jYStart < iYEnd + ebsilon &&
		  jYEnd > iYEnd + ebsilon)
	    {
	      vertOverlap = iYEnd - jYStart; //upper overlap
	      vertOverlapDir = 1;
	    }
	  else if(jYStart < iYStart + ebsilon && jYEnd > iYEnd + ebsilon)
	    {
	      vertOverlap = iYEnd - iYStart; //outer overlap
	      if(jYEnd-iYEnd  > iYStart-jYStart)
		vertOverlapDir = 1;
	      else
		vertOverlapDir = 0;
	    }
	  else
	    TCGMatrixHoriz[i][j] = 0;


	  //vertical constraint
	  if(jXStart < iXStart + ebsilon && jXEnd < iXEnd + ebsilon &&
	     jXEnd > iXStart + ebsilon)
	    {
	      horizOverlap = jXEnd - iXStart; //right overlap
	      horizOverlapDir = 0; 
	    }
	  else if(jXStart > iXStart + ebsilon && jXEnd < iXEnd + ebsilon)
	    {
	      horizOverlap = jXEnd - jXStart; //inner overlap
	      if(iXEnd-jXEnd > jXStart-iXStart)
		horizOverlapDir = 0;
	      else
		horizOverlapDir = 1;
	    }
	  else if(jXStart > iXStart + ebsilon && jXStart < iXEnd + ebsilon &&
		  jXEnd > iXEnd + ebsilon)
	    {
	      horizOverlap = iXEnd - jXStart; //left overlap
	      horizOverlapDir = 1;
	    }
	  else if(jXStart < iXStart + ebsilon && jXEnd > iXEnd + ebsilon)
	    {
	      horizOverlap = iXEnd - iXStart; //outer overlap
	      if(jXEnd-iXEnd  > iXStart-jXStart )
		horizOverlapDir = 1;
	      else
		horizOverlapDir = 0;
	    }
	  else
	    TCGMatrixVert[i][j] = 0;

	  if(vertOverlap > ebsilon && horizOverlap <= ebsilon)
	    {
	      if(iXStart <= jXStart)
		TCGMatrixHoriz[i][j] = 1;
	      else
		TCGMatrixHoriz[j][i] = 1;
	    }
	  else if(horizOverlap > ebsilon && vertOverlap <= ebsilon)
	    {
	      if(iYStart <= jYStart)
		TCGMatrixVert[i][j] = 1;
	      else
		TCGMatrixVert[j][i] = 1;
	    }
	  //overlapping
	  else if(horizOverlap > ebsilon && vertOverlap > ebsilon)
	    {
	      if(vertOverlap >= horizOverlap)
		{
		  if(horizOverlapDir == 1)
		    TCGMatrixHoriz[i][j] = 1;
		  else
		    TCGMatrixHoriz[j][i] = 1;
		}
	      else
		{
		  if(vertOverlapDir == 1)
		    TCGMatrixVert[i][j] = 1;
		  else
		    TCGMatrixVert[j][i] = 1;
		}
	    }
	}
    }

  //floyd marshal to find transitive closure
  //TCG_FM(TCGMatrixHoriz, TCGMatrixVert);

  //dynamic programming DFS algo to find transitive closure
  TCG_DP(TCGMatrixHoriz, TCGMatrixVert);

  //find ties and break them
  for(unsigned i=0; i<size; ++i)
    {
      for(unsigned j=0; j<i; ++j)
	{
	  if(i==j)
	    continue;
	  if(TCGMatrixHoriz[i][j] == 1 && TCGMatrixHoriz[j][i] == 1)
	    {
	      cout<<"ERROR in TCG 1 "<<i<<"\t"<<j<<endl;
	    }
	  if(TCGMatrixVert[i][j] == 1 && TCGMatrixVert[j][i] == 1)
	    {
	      cout<<"ERROR in TCG 2 "<<i<<"\t"<<j<<endl;
	    }
	  unsigned ctr = 0;
	  if(TCGMatrixHoriz[i][j] == 1)
	    ++ctr;
	  if(TCGMatrixHoriz[j][i] == 1)
	    ++ctr;
	  if(TCGMatrixVert[i][j] == 1)
	    ++ctr;
	  if(TCGMatrixVert[j][i] == 1)
	    ++ctr;

	  if(ctr > 1)
	    {
	      unsigned dir = rand()%2;
	      if(dir == 0) //H constraint
		{
		  TCGMatrixVert[i][j] = 0;
		  TCGMatrixVert[j][i] = 0;
		}
	      else //V constraint
		{
		  TCGMatrixHoriz[i][j] = 0;
		  TCGMatrixHoriz[j][i] = 0;
		}
	      /*
	      cout<<"ERROR in TCG 3 "<<i<<"\t"<<j<<"\t";
	      if(TCGMatrixHoriz[i][j] == 1)
		cout<<"H\t";
	      if(TCGMatrixHoriz[j][i] == 1)
		cout<<"InvH\t";
	      if(TCGMatrixVert[i][j] == 1)
		cout<<"V\t";
	      if(TCGMatrixVert[j][i] == 1)
		cout<<"InvV\t";
	      cout<<endl;
	      */
	    }
	  
	  //no constraint between the blocks
	  if(TCGMatrixHoriz[i][j] == 0 && TCGMatrixHoriz[j][i] == 0 &&
	     TCGMatrixVert[i][j] == 0 && TCGMatrixVert[j][i] == 0)
	    {
	      if(_xloc[i] < _xloc[j])
		TCGMatrixHoriz[i][j] = 1;
	      else
		TCGMatrixHoriz[j][i] = 1;

	      /*
		unsigned dir = rand()%2;
		if(dir == 0) //H constraint
		{
		if(_xloc[i] < _xloc[j])
		TCGMatrixHoriz[i][j] = 1;
		else
		TCGMatrixHoriz[j][i] = 1;
		}
		else //V constraint
		{
		if(_yloc[i] < _yloc[j])
		TCGMatrixVert[i][j] = 1;
		else
		TCGMatrixVert[j][i] = 1;
		}
	      */
	    }
	}
    }
  //get the sequence pair now
  _XX.resize(size);
  _YY.resize(size);
  for(unsigned i=0; i<size; ++i)
    {
      _XX[i] = i;
      _YY[i] = i;
    }
  
  SPXRelation SPX(TCGMatrixHoriz, TCGMatrixVert);
  SPYRelation SPY(TCGMatrixHoriz, TCGMatrixVert);

  std::sort(_XX.begin(), _XX.end(), SPX);
  std::sort(_YY.begin(), _YY.end(), SPY);

  /*
  cout<<"TCGMatrixHoriz"<<endl;
  cout<<"\t";
  for(unsigned i=0; i<size; ++i)
    cout<<i<<" ";
  cout<<endl;
  for(unsigned i=0; i<size; ++i)
    {
      cout<<i<<"\t";
      for(unsigned j=0; j<size; ++j)
	{
	  cout<<TCGMatrixHoriz[i][j]<<" ";
	}
      cout<<endl;
    }
  cout<<"TCGMatrixVert"<<endl;
  cout<<"\t";
  for(unsigned i=0; i<size; ++i)
    cout<<i<<" ";
  cout<<endl;
  for(unsigned i=0; i<size; ++i)
    {
      cout<<i<<"\t";
      for(unsigned j=0; j<size; ++j)
	{
	  cout<<TCGMatrixVert[i][j]<<" ";
	}
      cout<<endl;
    }
  print();
  */
}


void Pl2SP::print() const
{
  cout<<"XSequence Pair"<<endl;
  for(unsigned i=0; i<_XX.size(); ++i)
    cout<<_XX[i]<<"  ";
  cout<<endl<<"YSequence Pair"<<endl;
  for(unsigned i=0; i<_YY.size(); ++i)
    cout<<_YY[i]<<"  ";
  cout<<endl;
}

void Pl2SP::TCG_FM(vector< vector <bool> >& TCGMatrixHoriz, 
		   vector< vector <bool> >& TCGMatrixVert)
{
  unsigned size = TCGMatrixHoriz[0].size();
  for(unsigned k=0; k<size; ++k)
    {
      for(unsigned i=0; i<size; ++i)
	{
	  for(unsigned j = 0; j<size; ++j)
	    {
	      TCGMatrixHoriz[i][j] = TCGMatrixHoriz[i][j] | 
		(TCGMatrixHoriz[i][k] & TCGMatrixHoriz[k][j]);
	      TCGMatrixVert[i][j] = TCGMatrixVert[i][j] | 
		(TCGMatrixVert[i][k] & TCGMatrixVert[k][j]);
	    }
	}
    }
}

void Pl2SP::TCG_DP(vector< vector <bool> >& TCGMatrixHoriz, 
		   vector< vector <bool> >& TCGMatrixVert)
{
  int size = TCGMatrixHoriz[0].size();
  vector< vector <bool> > adjMatrixHoriz = TCGMatrixHoriz;
  vector< vector <bool> > adjMatrixVert = TCGMatrixVert;
  vector<int> pre(size, -1);
  
  int i;
  for(i=0; i<size; ++i)
    {
      fill(TCGMatrixHoriz[i].begin(), TCGMatrixHoriz[i].end(), false);
      fill(TCGMatrixVert[i].begin(), TCGMatrixVert[i].end(), false);
    }

  _cnt = 0;
  for(i=0; i<size; ++i)
    if(pre[i] == -1)
      TCGDfs(TCGMatrixHoriz, adjMatrixHoriz, i, pre);

  _cnt = 0;
  fill(pre.begin(), pre.end(), -1);
  for(i=0; i<size; ++i)
    if(pre[i] == -1)
      TCGDfs(TCGMatrixVert, adjMatrixVert, i, pre);
}

void Pl2SP::TCGDfs(vector< vector <bool> >& TCGMatrix, 
		   vector< vector <bool> >& adjMatrix, int v, vector<int>& pre)
{
  int u, i;
  pre[v] = _cnt++;
  int size = adjMatrix[0].size();

  for(u=0; u<size; ++u)
    {
      if(adjMatrix[v][u])
	{
	  TCGMatrix[v][u] = true;
	  if(pre[u] > pre[v]) continue;

	  if(pre[u] == -1)
	    TCGDfs(TCGMatrix, adjMatrix, u, pre);

	  for(i=0; i<size; ++i)
	    if(TCGMatrix[u][i] == 1)
	      TCGMatrix[v][i] = 1;
	}
    }
}

