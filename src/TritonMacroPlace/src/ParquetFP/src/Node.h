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


#ifndef NODE_H
#define NODE_H

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "FPcommon.h"

namespace parquetfp
{
   class Nets;

   struct NodePin
   {
      unsigned netIndex;
      unsigned pinOffset;
   };

   typedef std::vector<NodePin>::iterator itNodePin;

   class Node
   {
       private:
           float _area;
           float _minAr;
           float _maxAr;
           float _width;
           float _height;
           
           float _snapX;
           float _snapY;
           float _haloX;
           float _haloY;
           float _channelX;
           float _channelY;

           ORIENT _orient;          //N,E,S,W,FN.FE,FS or FW. N on initialization
           bool _isOrientFixed;
           float _slackX;
           float _slackY;
           int _index;
           bool _type;              //0 normal node,1 pad
           bool _isMacro;           //is Node Macro(used only during clustering)
		   bool _needsFP;           //<aaronnn> if node needs FP (e.g., if this node triggered FP)
           Point _placement;
           std::vector<NodePin> _pins;       //all the pins of this node
           std::vector<int> _subBlockIndices; //indices of subBlocks

           std::string _name;
           float _origWidth;
           float _origHeight;

//           Node(void) {} //Default constructor private and disabled use a reference or pointer
       public:
           bool allPinsAtCenter;   //helps for HPWL evaluation. made public member for speed. be carefull in modifying this.
           bool calcAllPinsAtCenter(Nets& nets); //use this only for initialization. else use above variable

           //ctors
           Node(const std::string block_name,float block_area,float minAr,float maxAr,
                   int index,bool type);
           Node(const Node& orig);
//           Node& operator=(const Node& rhs);

           bool getType() const            {return _type;}
           int getIndex(void) const        {return _index;} 
           ORIENT getOrient(void) const    {return _orient;}
           bool isOrientFixed(void) const  {return _isOrientFixed;}
           float getHeight(void) const     {return _height;}
           float getWidth(void) const      {return _width;}
           float getOrigHeight(void) const {return _origHeight;}
           float getOrigWidth(void) const  {return _origWidth;}
           std::string getName(void) const {return _name;}
           float getX(void) const          {return _placement.x;}
           float getY(void) const          {return _placement.y;}
           float getslackX(void) const     {return _slackX;}
           float getslackY(void) const     {return _slackY;}
           float getminAR(void) const      {return _minAr;}
           float getmaxAR(void) const      {return _maxAr;}
           float getArea(void) const       {return _area;}

           float getSnapX(void) const      {return _snapX;}
           float getSnapY(void) const      {return _snapY;}
           float getHaloX(void) const      {return _haloX;}
           float getHaloY(void) const      {return _haloY;}
           float getChannelX(void) const   {return _channelX;}
           float getChannelY(void) const   {return _channelY;}

           bool isMacro(void) const        {return _isMacro;}
	   bool needsFP(void) const        {return _needsFP;} // <aaronnn>
           bool isHard(void) const         {return equalFloat(_maxAr, _minAr);}
           void updateMacroInfo(bool isMacro) {_isMacro = isMacro;}

           itNodePin pinsBegin()           {return _pins.begin(); }
           itNodePin pinsEnd()             {return _pins.end(); }
           unsigned getDegree() const      { return _pins.size(); }
           void clearPins()                { _pins.clear(); }
           std::vector<int>::iterator subBlocksBegin()  {return _subBlockIndices.begin();}
           std::vector<int>::iterator subBlocksEnd()    {return _subBlockIndices.end();}
           unsigned numSubBlocks()                 {return _subBlockIndices.size();}
           std::vector<int>& getSubBlocks()             { return _subBlockIndices; }


           void putArea(float area)         {_area=area;}

           void putWidth(float w)           {_width=w;}
           void putHeight(float h)          {_height=h;}
           void putHaloX(float hx)          {_haloX = hx;}
           void putHaloY(float hy)          {_haloY = hy;}
           void putSnapX(float sx)          {_snapX = sx;}
           void putSnapY(float sy)          {_snapY = sy;}
           void putChannelX(float cx)       {_channelX = cx;}
           void putChannelY(float cy)       {_channelY = cy;}

           void putX(float x)               {_placement.x=x;}
           void putY(float y)               {_placement.y=y;}
           void putslackX(float x)          {_slackX=x;}
           void putslackY(float y)          {_slackY=y;}
	         void putNeedsFP(bool needFP)     {_needsFP=needFP;} // <aaronnn>
           void addPin(NodePin& pinTemp)    {_pins.push_back(pinTemp);}

           //to be used only during initialization else use changeOrient
           void putOrient(ORIENT newOrient) {
             _orient = newOrient;
           } 
           void putmaxAR(float newMaxAR)    { _maxAr = newMaxAR;   }
           void putminAR(float newMinAR)    { _minAr = newMinAR;   }

           void putIsOrientFixed(bool value)	{ _isOrientFixed = value;}
           void addSubBlockIndex(int index)  { _subBlockIndices.push_back(index); }

           void changeOrient(ORIENT newOrient, Nets& nets);
           void syncOrient(Nets& nets);
           bool needSyncOrient;  //during parsing if DIMS found then no need to change
           //H & W. If orient found and no DIMS then need change
           //in H & W

   };

}
//using namespace parquetfp;

#endif
