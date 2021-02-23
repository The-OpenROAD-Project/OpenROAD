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


#ifndef NETS_H
#define NETS_H

#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "Net.h"

namespace parquetfp
{
 
   class Nodes;
   typedef std::vector<Net>::iterator itNet;
   typedef std::vector<Net>::const_iterator itNetConst;

   struct eqstr
   {
     bool operator()(const char* s1, const char* s2) const
     {
       return strcmp(s1, s2) == 0;
     }
   };

   class Nets
   {
   private:
      std::vector<Net> _nets;
     std::unordered_map<std::string, int> _name2IdxMap;

   public:
      Nets(const std::string &baseName);
      Nets()
         {}

      void clean(void)
         { _nets.clear(); }

      void parseNets(const std::string &fnameNets);

      void parseWts(const std::string &fnameWts);

      void updateNodeInfo(Nodes& nodes);

      itNet netsBegin(void)
         { return _nets.begin(); }

      itNet netsEnd(void)
         { return _nets.end(); }

      itNetConst netsBegin(void) const
         { return _nets.begin(); }

      itNetConst netsEnd(void) const
         { return _nets.end(); }

      Net& getNet(unsigned index)
         { return _nets[index]; }

      const Net& getNet(unsigned index) const
         { return _nets[index]; }

      void putNewNet(Net& net)
         { _nets.push_back(net); /*_numPins+=net.getDegree();*/ }

      int getNumPins(void) const;
      //unsigned getNumPins(void) 
      //   { return _numPins; }

      unsigned getNumNets(void) const
         { return _nets.size(); }

      void initName2IdxMap(void);
      void putName2IdxEntry(std::string netName, int idx);
      int getIdxFrmName(std::string netName);
   };
}

//using namespace parquetfp;

#endif
