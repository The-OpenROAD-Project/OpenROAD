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


#ifndef NETLIST_H
#define NETLIST_H

#include "basepacking.h"

#include <iostream>
#include <string>

// --------------------------------------------------------
class NetType
{
public:
   class PinType;
   class PadType;
   NetType(const std::vector<PinType>& newPins,
           const std::vector<PadType>& newPads)
      : pins(in_pins),
        pads(in_pads),
        in_pins(newPins),
        in_pads(newPads) {}

   inline NetType(const NetType& newNet);
   inline void operator =(const NetType& newNet);

   const std::vector<PinType>& pins;
   const std::vector<PadType>& pads;

   float getHPWL(const OrientedPacking& pk) const;

   class PinType
   {
   public:
      int block;       // index of blocks
      float x_offset; // offset from center
      float y_offset; // offset from center
   };

   class PadType
   {
   public:
      float xloc;
      float yloc;
   };

private:
   std::vector<PinType> in_pins;
   std::vector<PadType> in_pads;

   inline void getOffsets(float orig_x_offset,
                          float orig_y_offset,
                          OrientedPacking::ORIENT orient,
                          float& x_offset,
                          float& y_offset) const;
};
// --------------------------------------------------------
class NetListType
{
public:
   inline NetListType(std::ifstream& infile_pl, std::ifstream& infile_nets,
                      const HardBlockInfoType& blockinfo);

   class PadInfoType;
   const std::vector<NetType>& nets;
   const std::vector<PadInfoType>& padinfo;
   inline float getHPWL(const OrientedPacking& pk) const;

   class PadInfoType
   {
   public:
      std::string pad_name;
      float xloc;
      float yloc;
   };

private:
   std::vector<NetType> in_nets;
   std::vector<PadInfoType> in_all_pads;

   void ParsePl(std::ifstream& infile,
                const HardBlockInfoType& blockinfo);
   void ParseNets(std::ifstream& infile,
                  const HardBlockInfoType& blockinfo);
   inline void get_index(const std::string& name,
                         std::vector<int>& indices) const;
   inline int get_index(const std::string& name,
                        const HardBlockInfoType& blockinfo) const;

   NetListType(const NetListType&);
};
// --------------------------------------------------------

// ==============
// IMPLEMENTATION
// ==============
inline NetListType::NetListType(std::ifstream& infile_pl,
                                std::ifstream& infile_nets,
                                const HardBlockInfoType& blockinfo)
   : nets(in_nets),
     padinfo(in_all_pads)
{
   ParsePl(infile_pl, blockinfo);
   ParseNets(infile_nets, blockinfo);
}
// --------------------------------------------------------
inline float NetListType::getHPWL(const OrientedPacking& pk) const
{
   int num_nets = in_nets.size();
   float HPWL = 0;
   for (int i = 0; i < num_nets; i++)
   {
//      printf("---Net [%d]---\n", i);
      HPWL += in_nets[i].getHPWL(pk);
//      std::cout.setf(ios::fixed);
//      std::cout.precision(2);
//      std::cout << HPWL << std::endl << std::endl;
   }
   return HPWL;
}
// --------------------------------------------------------
inline int NetListType::get_index(const std::string& name,
                                  const HardBlockInfoType& blockinfo) const
{
   for (int i = 0; i < blockinfo.blocknum(); i++)
      if (name == blockinfo.block_names[i])
         return i;
   return -1;
}
// --------------------------------------------------------
inline void NetListType::get_index(const std::string& name,
                                   std::vector<int>& indices) const
{
   indices.clear();
   for (unsigned int i = 0; i < in_all_pads.size(); i++)
   {
      const std::string& this_pad_name = in_all_pads[i].pad_name;
      int pos = this_pad_name.find(name, 0);     
      if (pos == 0)
      {
         if (this_pad_name.length() == name.length())
            indices.push_back(i);
         else if (this_pad_name[name.length()] == '@')
            indices.push_back(i);
      }
   }
}
// --------------------------------------------------------
inline NetType::NetType(const NetType& newNet)
   : pins(in_pins),
     pads(in_pads)
{
   in_pins = newNet.in_pins;
   in_pads = newNet.in_pads;
}
// --------------------------------------------------------
inline void NetType::operator =(const NetType& newNet)
{
   in_pins = newNet.in_pins;
   in_pads = newNet.in_pads;
}
// --------------------------------------------------------
inline void NetType::getOffsets(float orig_x_offset,
                                float orig_y_offset,
                                OrientedPacking::ORIENT blk_orient,
                                float& x_offset,
                                float& y_offset) const
{
   switch (blk_orient)
   {
   case OrientedPacking::N:
      x_offset = orig_x_offset;
      y_offset = orig_y_offset;
      break;
      
   case OrientedPacking::E:
      x_offset = orig_y_offset;
      y_offset = -1 * orig_x_offset;
      break;
      
   case OrientedPacking::S:
      x_offset = -1 * orig_x_offset;
      y_offset = -1 * orig_y_offset;
      break;
      
   case OrientedPacking::W:
      x_offset = -1 * orig_y_offset;
      y_offset = orig_x_offset;
      break;

   case OrientedPacking::FN:
      x_offset = -1 * orig_x_offset;
      y_offset = orig_y_offset;
      break;

   case OrientedPacking::FE:
      x_offset = orig_y_offset;
      y_offset = orig_x_offset;
      break;

   case OrientedPacking::FS:
      x_offset = orig_x_offset;
      y_offset = -1 * orig_y_offset;
      break;

   case OrientedPacking::FW:
      x_offset = -1 * orig_y_offset;
      y_offset = -1 * orig_x_offset;
      break;

   case OrientedPacking::OrientUndefined:
      std::cout << "ERROR: the orientation for this block is undefined."
           << std::endl;
      exit(1);
      break;
                            
   default:
      std::cout << "ERROR in specified orientation." << std::endl;
      exit(1);
      break;
   }
}
// --------------------------------------------------------
#endif
