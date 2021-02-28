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


#ifndef MIXEDPACKING_H
#define MIXEDPACKING_H

#include "basepacking.h"

#include <string>

// --------------------------------------------------------
class MixedBlockInfoType
{
public:
   MixedBlockInfoType(const std::string& blocksfilename,
                      const std::string& format); // "blocks" or "txt"
   virtual ~MixedBlockInfoType() {}
   
   class BlockARInfo
   {
   public:
      float area;
      std::vector<float> maxAR; // maxAR/minAR for the "North" orientation
      std::vector<float> minAR;
      bool isSoft;
   };
   const HardBlockInfoType& currDimensions;
   const std::vector<BlockARInfo>& blockARinfo;
   static const int Orient_Num; // = HardBlockInfoType::Orient_Num;

   inline void setBlockDimensions(int index, float newWidth, float newHeight,
                                  int theta);

protected:
   HardBlockInfoType _currDimensions;
   std::vector<BlockARInfo> _blockARinfo;

   void ParseBlocks(std::ifstream& input);
   void ParseTxt(std::ifstream& input);
   inline void set_blockARinfo_AR(int index, float minAR, float maxAR);

   // used by descendent class "MixedBlockInfoTypeFromDB"
   MixedBlockInfoType(int blocknum)
      : currDimensions(_currDimensions),
        blockARinfo(_blockARinfo),
        _currDimensions(blocknum),
        _blockARinfo(blocknum+2)
      {}
};
// --------------------------------------------------------

// ===============
// IMPLEMENTATIONS
// ===============
void MixedBlockInfoType::setBlockDimensions(int index,
                                            float newWidth,
                                            float newHeight,
                                            int theta)
{
   float realWidth = (theta % 2 == 0)? newWidth : newHeight;
   float realHeight = (theta % 2 == 0)? newHeight : newWidth;
   _currDimensions.set_dimensions(index, realWidth, realHeight);
}
// --------------------------------------------------------
void MixedBlockInfoType::set_blockARinfo_AR(int index,
                                            float minAR,
                                            float maxAR)
{
   _blockARinfo[index].minAR.resize(Orient_Num);
   _blockARinfo[index].maxAR.resize(Orient_Num);
   for (int i = 0; i < Orient_Num; i++)
   {
      _blockARinfo[index].minAR[i] = ((i%2 == 0)?
                                      minAR : (1.f / maxAR));
      _blockARinfo[index].maxAR[i] = ((i%2 == 0)?
                                      maxAR : (1.f / minAR));
   }
}         
// -------------------------------------------------------- 
#endif
