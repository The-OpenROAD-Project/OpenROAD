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
#include "Nets.h"
#include "Node.h"

using std::string;
using std::vector;

namespace parquetfp
{

    Node::Node(const string block_name,float block_area,float minAr,float maxAr,
            int index,bool type)
        : _area(block_area),_minAr(minAr),_maxAr(maxAr),_width(sqrt(_area*minAr)),
        _height(_width/minAr), 
        _snapX(0), _snapY(0),
        _haloX(0), _haloY(0),
        _channelX(0), _channelY(0),  
        _orient(N), _isOrientFixed(false),
        _slackX(0), _slackY(0),_index(index),_type(type),
        _isMacro(false), _needsFP(false), _placement(0.f,0.f),_name(block_name), 
        _origWidth(_width), _origHeight(_height), 
        allPinsAtCenter(false),needSyncOrient(true)
        {
            //by default orientation is N
        }

    //default ctor --Disabled
    //Node::Node()
    //  : _area(0),_minAr(0),_maxAr(0),_orient(N),_isOrientFixed(false),_slackX(0),
    //    _slackY(0),_index(0),_type(true),_isMacro(false),//_isHard(true),
    //    allPinsAtCenter(false)
    //{
    //  //strcpy(_name,"");
    //  _name = "";
    //  _placement.x=0;
    //  _placement.y=0;
    //  _height = 0;
    //  _width = 0;
    //    // cout<<"Setting "<<_index<<" "<<_name<<" new AR (d) max: "<<_maxAr<<" min: "<<_minAr<<" hard?: "<<_isHard<<" "<< isHard()<<endl;
    //}



    Node::Node(const Node& orig):
      _area(orig._area), _minAr(orig._minAr), _maxAr(orig._maxAr), _width(orig._width),
      _height(orig._height), 
      _snapX(orig._snapX), _snapY(orig._snapY),
      _haloX(orig._haloX), _haloY(orig._haloY), 
      _channelX(orig._channelX), _channelY(orig._channelY),
      _orient(orig._orient), _isOrientFixed(orig._isOrientFixed),
      _slackX(orig._slackX), _slackY(orig._slackY), _index(orig._index), _type(orig._type),
      _isMacro(orig._isMacro), _needsFP(orig._needsFP), _placement(orig._placement), 
      _pins(orig._pins),
      _subBlockIndices(orig._subBlockIndices), _name(orig._name), _origWidth(orig._origWidth),
      _origHeight(orig._origHeight), allPinsAtCenter(orig.allPinsAtCenter),
      needSyncOrient(orig.needSyncOrient)
    { }

/*
    Node& Node::operator=(const Node& rhs)
    {

        _name=rhs._name; 
        _area=rhs._area;
        _minAr=rhs._minAr;
        _maxAr=rhs._maxAr;
        _width=rhs._width;
        _height=rhs._height;
        _origWidth=rhs._origWidth; 
        _origHeight=rhs._origHeight;
        _orient=rhs._orient; 
        _isOrientFixed=rhs._isOrientFixed;
        _slackX=rhs._slackX;
        _slackY=rhs._slackY;
        _index=rhs._index;
        _type=rhs._type;
        _isMacro=rhs._isMacro;
        _pins.clear();
        _pins.insert(_pins.end(), rhs._pins.begin(), rhs._pins.end());
        _subBlockIndices.clear();
        _subBlockIndices.insert(_subBlockIndices.end(), rhs._subBlockIndices.begin(), rhs._subBlockIndices.end());
        allPinsAtCenter=rhs.allPinsAtCenter;
        needSyncOrient=rhs.needSyncOrient;
        _placement.x=rhs._placement.x;
        _placement.y=rhs._placement.y;
        return *this;
    }
*/

    void Node::changeOrient(ORIENT newOrient, Nets& nets)
    {
        if(_orient == newOrient)
            return;

        if(_orient%2 != newOrient%2)
        {
            float tempHeight = _height;
            _height = _width;
            _width = tempHeight;
        }

        //update the pinoffsets of the netlist now
        for(vector<NodePin>::const_iterator itP = _pins.begin(); itP != _pins.end(); ++itP)
        {
            Net& net = nets.getNet(itP->netIndex);
            pin& netPin = net.getPin(itP->pinOffset);
            netPin.changeOrient(newOrient);
        }
        _orient = newOrient;

    }

    void Node::syncOrient(Nets& nets)
    {
        //update the heights and widths only if not updated earlier in parsePl
        if(needSyncOrient)
        {
            if(int(_orient)%2 == 1)  //needs swap of height and width
            {
                float tempHeight = _height;
                _height = _width;
                _width = tempHeight;
            }
        }

        //update the pinoffsets of the netlist now
        //itNodePin itP;
        //for(itP = _pins.begin(); itP != _pins.end(); ++itP)
        //  {
        //    pin& netPin = nets.getNet(itP->netIndex).getPin(itP->pinOffset);
        //    netPin.changeOrient(_orient);
        //  }
        for(itNodePin itP = _pins.begin(); itP != _pins.end(); ++itP)
        {
            Net& net = nets.getNet(itP->netIndex);
            pin& netPin = net.getPin(itP->pinOffset);
            netPin.changeOrient(_orient);
        }
    }

    bool Node::calcAllPinsAtCenter(Nets& nets)
    {
        itNodePin itP;
        bool localAllPinsAtCenter=true;
        for(itP = _pins.begin(); itP != _pins.end(); ++itP)
        {
            pin& netPin = nets.getNet(itP->netIndex).getPin(itP->pinOffset);
            if(netPin.getXOffset() != 0 || netPin.getYOffset() != 0)
            {
                localAllPinsAtCenter = false;
                break;
            }
        }
        return(localAllPinsAtCenter);
    }
}
