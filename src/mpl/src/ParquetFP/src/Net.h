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


#ifndef NET_H
#define NET_H

#include "FPcommon.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace parquetfp
{

   class pin;
   typedef std::vector<pin>::iterator itPin;
   typedef std::vector<pin>::const_iterator itPinConst;


//this class holds the pin offsets and pinindex on a particular node
   class pin
   {
   private:
      Point _origOffset;    //original offset wrt center. In relative terms
      Point _offset;  //offsets during iteration. to account for orientation changes
      ORIENT _orient;       //keeps track of orientation of the node
      std::string _name;
      bool _type;
      int _nodeIndex;       //index of the node in which the pin is located
      int _netIndex;        //index of net to which pin is attached
  
   public:
  
      pin(const std::string name, bool type, float xoffset, float yoffset, int netIndex)
        :_type(type), _nodeIndex(0)
         {
	   //strncpy(_name,name,199);
	   //_name[199] = '\0';
	    _name = name;
            _origOffset.x=xoffset;
            _origOffset.y=yoffset;
            _offset.x=xoffset;
            _offset.y=yoffset;
            _orient=N;
            _netIndex=netIndex;
         }
      pin()
         {
         }

      bool getType() const
         { return _type;}
      const std::string getName(void) const
         {return _name;}
      int getNodeIndex() const
         {return _nodeIndex;}
      int getNetIndex() const
         { return _netIndex; }
      void putNodeIndex(int nodeIndex)
         { _nodeIndex = nodeIndex; }
      void putNetIndex(int netIndex)
         { _netIndex = netIndex; }
      void putType(bool type)
         { _type = type; }
  
      float getXOffset(void) const
         { return _offset.x; }
      float getYOffset(void) const
         { return _offset.y; }
      float getOrigXOffset(void) const
         { return _origOffset.x; }
      float getOrigYOffset(void) const
         { return _origOffset.y; }

      ORIENT getOrient(void) const
         { return _orient; }
  
      void changeOrient(ORIENT newOrient);
	    
   };



   class Net
   {

   public:
      std::vector<pin> _pins;
 
   private: 
      int _index;
      std::string _name;
      float _weight;

   public:
      Net()
         {
            _index = 0;
            _weight = 1.0;
         }

      void putName(const std::string name)
         { 
	   _name = name;
         }

      void addNode(const pin& node)
         {
            _pins.push_back(node);
         }
      void clean(void)
         {
            _pins.clear();
         }
      itPin pinsBegin()
         {
            return _pins.begin();
         }
      itPin pinsEnd()
         {
            return _pins.end();
         }

      pin& getPin(unsigned pinOffset)
         { return _pins[pinOffset]; }

      const pin& getPin(unsigned pinOffset) const
         { return _pins[pinOffset]; }

      int getIndex() const
         {
            return _index;
         }
      void putIndex(int netIndex)
         {
            _index = netIndex;
         }
      float getWeight() const
         {
            return _weight;
         }
      void putWeight(float netWeight)
         {
            _weight = netWeight;
         }
      const std::string getName(void) const
         { return _name; }

      unsigned getDegree(void) const
         { return _pins.size(); }
  
   };

}

//using namespace parquetfp;

#endif
