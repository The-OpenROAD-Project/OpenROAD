/**************************************************************************
***    
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
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
using std::ostream;
using std::cout;
using std::endl;

ostream& parquetfp::operator<<(ostream& out, const ORIENT& orient)
{
  if(orient == N)
    out<<"N";
  else if(orient == E)
    out<<"E";
  else if(orient == S)
    out<<"S";
  else if(orient == W)
    out<<"W";
  else if(orient == FN)
    out<<"FN";
  else if(orient == FE)
    out<<"FE";
  else if(orient == FS)
    out<<"FS";
  else if(orient == FW)
    out<<"FW";
  else
    cout<<"ERROR in outputting orientations"<<endl;
  return out;
}

bool parquetfp::operator<(const parquetfp::Point &a, const parquetfp::Point &b)
{
  if(a.x < b.x) return true;
  if(a.x > b.x) return false;
  return (a.y < b.y);
}

bool parquetfp::operator==(const parquetfp::Point &a, const parquetfp::Point &b)
{
  return (a.x == b.x) && (a.y == b.y);
}

parquetfp::BBox::BBox()
{
  _minX=(std::numeric_limits<float>::max());
  _maxX=(-std::numeric_limits<float>::max());
  _minY=(std::numeric_limits<float>::max());
  _maxY=(-std::numeric_limits<float>::max());
  _valid=0;
}

void parquetfp::BBox::clear(void)
{
  _minX=(std::numeric_limits<float>::max());
  _maxX=(-std::numeric_limits<float>::max());
  _minY=(std::numeric_limits<float>::max());
  _maxY=(-std::numeric_limits<float>::max());
  _valid=0;
}

void parquetfp::BBox::put(const Point& point)
{
  if(point.x < _minX)
    _minX = point.x;
  if(point.x > _maxX)
    _maxX = point.x;
  if(point.y < _minY)
    _minY = point.y;
  if(point.y > _maxY)
    _maxY = point.y;
  _valid = 1;
}

float parquetfp::BBox::getHPWL(void) const
{
  return((_maxX-_minX)+(_maxY-_minY));
}

float parquetfp::BBox::getXSize(void) const
{ return(_maxX-_minX); }

float parquetfp::BBox::getYSize(void) const
{ return(_maxY-_minY); }

float parquetfp::BBox::getMinX(void) const
{ return _minX; }

float parquetfp::BBox::getMinY(void) const
{ return _minY; }

bool parquetfp::BBox::isValid(void) const
{
  return _valid;
}

std::istream& parquetfp::operator>>(std::istream& in, BBox &box)
{
  Point botLeft, topRight;
  char comma;
  in >> botLeft.x;
  abkfatal(needCaseChar(in,','), ", expected parsing BBox");
  in >> comma >> botLeft.y;
  abkfatal(needCaseChar(in,','), ", expected parsing BBox");
  in >> comma >> topRight.x;
  abkfatal(needCaseChar(in,','), ", expected parsing BBox");
  in >> comma >> topRight.y;
  box.clear(); box.put(botLeft); box.put(topRight);
  return in;
}

std::istream& parquetfp::eatblank(std::istream& i)
{
  while (i.peek()==' ' || i.peek()=='\t') i.get();
  return i;
}

std::istream& parquetfp::skiptoeol(std::istream& i)
{
  while (!i.eof() && i.peek()!='\n' && i.peek()!='\r') i.get();
  i.get();
  return i;
}

bool parquetfp::needCaseChar(std::istream& i, char character)
{
  while(!i.eof() && i.peek() != character) i.get();
  if(i.eof())
    return 0;
  else
    return 1;
}
std::istream& parquetfp::eathash(std::istream& i)
{
  return skiptoeol(i);
}

parquetfp::ORIENT parquetfp::toOrient(char* orient)
{
  if(!strcmp(orient, "N"))
    return N;
  if(!strcmp(orient, "E"))
    return E;
  if(!strcmp(orient, "S"))
    return S;
  if(!strcmp(orient, "W"))
    return W;
  if(!strcmp(orient, "FN"))
    return FN;
  if(!strcmp(orient, "FE"))
    return FE;
  if(!strcmp(orient, "FS"))
    return FS;
  if(!strcmp(orient, "FW"))
    return FW;

  cout<<"ERROR: in converting char* to ORIENT"<<endl;
  return N;
}

const char* parquetfp::toChar(ORIENT orient)
{
  if(orient == N)
   { return("N"); }
  if(orient == E)
   { return("E"); }
  if(orient == S)
   { return("S"); }
  if(orient == W)
   { return("W"); }
  if(orient == FN)
   { return("FN"); }
  if(orient == FE)
   { return("FE"); }
  if(orient == FS)
   { return("FS"); }
  if(orient == FW)
   { return("FW"); }
  cout<<"ERROR: in converting ORIENT to char* "<<endl;
  return "N";
}
