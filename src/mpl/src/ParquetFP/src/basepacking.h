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

#ifndef BASEPACKING_H
#define BASEPACKING_H

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// struct-like classes for representation conversion
// --------------------------------------------------------
class BasePacking
{
 public:
  std::vector<float> xloc;
  std::vector<float> yloc;
  std::vector<float> width;
  std::vector<float> height;
};
// --------------------------------------------------------
class OrientedPacking : public BasePacking
{
 public:
  enum ORIENT
  {
    N,
    E,
    S,
    W,
    FN,
    FE,
    FS,
    FW,
    OrientUndefined = -1
  };
  std::vector<ORIENT> orient;

  inline static ORIENT toOrient(char* orient);
  inline static const char* toChar(ORIENT orient);
  inline static ORIENT flip(ORIENT orient);
};
inline std::ostream& operator<<(std::ostream& outs, OrientedPacking::ORIENT);
inline void Save_bbb(std::ostream& outs, const OrientedPacking& pk);
inline void Read_bbb(std::istream& ins, const OrientedPacking& pk);
// --------------------------------------------------------
namespace basepacking_h {
class Dimension
{
 public:
  std::vector<float> width;
  std::vector<float> height;

  // Grid Snapping
  std::vector<float> snapX, snapY;
  // Macro Halo Information
  std::vector<float> haloX, haloY;
  std::vector<float> channelX, channelY;

  static const float Infty;             // = 1e100;
  static const float Epsilon_Accuracy;  // = 1e10;
  static const int Undefined;           // = -1;
  static const int Orient_Num;          // = 8;
};
}  // namespace basepacking_h
// --------------------------------------------------------
class HardBlockInfoType
{
 public:
  HardBlockInfoType(std::ifstream& ins,          // formats:
                    const std::string& format);  // "txt" or "blocks"
  const std::vector<basepacking_h::Dimension>& blocks;
  const std::vector<std::string>& block_names;
  inline const basepacking_h::Dimension& operator[](int index) const;
  inline int blocknum() const;
  inline float blockArea() const;

  static const int Orient_Num;  // = basepacking_h::Dimension::Orient_Num;

  friend class MixedBlockInfoType;
  friend class MixedBlockInfoTypeFromDB;

 protected:
  std::vector<basepacking_h::Dimension>
      in_blocks;  // store the left & bottom edges at the back
  std::vector<std::string> in_block_names;  // parallel array with in_blocks

  // support
  void set_dimensions(int i,
                      float w,
                      float h,
                      float snapX = 0,
                      float snapY = 0,
                      float haloX = 0,
                      float haloY = 0,
                      float channelX = 0,
                      float channelY = 0);
  void ParseTxt(std::ifstream& ins);
  void ParseBlocks(std::ifstream& ins);

  HardBlockInfoType(int blocknum)
      : blocks(in_blocks),
        block_names(in_block_names),
        in_blocks(blocknum + 2),
        in_block_names(blocknum + 2)
  {
  }

  HardBlockInfoType(const HardBlockInfoType&);
};
// --------------------------------------------------------
inline void PrintDimensions(float width, float height);
inline void PrintAreas(float deadspace, float blockArea);
inline void PrintUtilization(float deadspace, float blockArea);
// --------------------------------------------------------

// =========================
//      IMPLEMENTATIONS
// =========================
inline std::ostream& operator<<(std::ostream& outs,
                                OrientedPacking::ORIENT orient)
{
  switch (orient) {
    case OrientedPacking::N:
      outs << "N";
      break;
    case OrientedPacking::E:
      outs << "E";
      break;
    case OrientedPacking::S:
      outs << "S";
      break;
    case OrientedPacking::W:
      outs << "W";
      break;
    case OrientedPacking::FN:
      outs << "FN";
      break;
    case OrientedPacking::FE:
      outs << "FE";
      break;
    case OrientedPacking::FS:
      outs << "FS";
      break;
    case OrientedPacking::FW:
      outs << "FW";
      break;
    case OrientedPacking::OrientUndefined:
      outs << "--";
      break;
    default:
      std::cout << "ERROR in outputting orientations." << std::endl;
      exit(1);
      break;
  }
  return outs;
}
// --------------------------------------------------------
inline void Save_bbb(std::ostream& outs, const OrientedPacking& pk)
{
  float totalWidth = 0.0f;
  float totalHeight = 0.0f;
  int blocknum = pk.xloc.size();
  for (int i = 0; i < blocknum; i++) {
    totalWidth = std::max(totalWidth, pk.xloc[i] + pk.width[i]);
    totalHeight = std::max(totalHeight, pk.yloc[i] + pk.height[i]);
  }

  outs << totalWidth << std::endl;
  outs << totalHeight << std::endl;
  outs << blocknum << std::endl;
  for (int i = 0; i < blocknum; i++)
    outs << pk.width[i] << " " << pk.height[i] << std::endl;
  outs << std::endl;

  for (int i = 0; i < blocknum; i++)
    outs << pk.xloc[i] << " " << pk.yloc[i] << std::endl;
}
// --------------------------------------------------------
inline void Read_bbb(std::istream& ins, OrientedPacking& pk)
{
  float width, height;
  ins >> width >> height;

  int blocknum = -1;
  ins >> blocknum;

  pk.xloc.resize(blocknum);
  pk.yloc.resize(blocknum);
  pk.width.resize(blocknum);
  pk.height.resize(blocknum);
  pk.orient.resize(blocknum);
  for (int i = 0; i < blocknum; i++) {
    ins >> pk.width[i] >> pk.height[i];
    pk.orient[i] = OrientedPacking::N;
  }

  for (int i = 0; i < blocknum; i++)
    ins >> pk.xloc[i] >> pk.yloc[i];
}
// --------------------------------------------------------
inline OrientedPacking::ORIENT OrientedPacking::flip(
    OrientedPacking::ORIENT orient)
{
  switch (orient) {
    case N:
      return FE;
    case E:
      return FN;
    case S:
      return FW;
    case W:
      return FS;
    case FN:
      return E;
    case FE:
      return N;
    case FS:
      return W;
    case FW:
      return S;
    case OrientUndefined:
      return OrientUndefined;
    default:
      std::cout << "ERROR: invalid orientation: " << orient << std::endl;
      exit(1);
      break;
  }
}
// --------------------------------------------------------
inline OrientedPacking::ORIENT OrientedPacking::toOrient(char* orient)
{
  if (!strcmp(orient, "N"))
    return N;
  if (!strcmp(orient, "E"))
    return E;
  if (!strcmp(orient, "S"))
    return S;
  if (!strcmp(orient, "W"))
    return W;
  if (!strcmp(orient, "FN"))
    return FN;
  if (!strcmp(orient, "FE"))
    return FE;
  if (!strcmp(orient, "FS"))
    return FS;
  if (!strcmp(orient, "FW"))
    return FW;
  if (!strcmp(orient, "--"))
    return OrientUndefined;

  std::cout << "ERROR: in converting char* to ORIENT" << std::endl;
  exit(1);
  return OrientUndefined;
}
// --------------------------------------------------------
inline const char* OrientedPacking::toChar(ORIENT orient)
{
  if (orient == N)
    return ("N");
  if (orient == E)
    return ("E");
  if (orient == S)
    return ("S");
  if (orient == W)
    return ("W");
  if (orient == FN)
    return ("FN");
  if (orient == FE)
    return ("FE");
  if (orient == FS)
    return ("FS");
  if (orient == FW)
    return ("FW");
  if (orient == OrientUndefined)
    return ("--");

  std::cout << "ERROR in converting ORIENT to char* " << std::endl;
  exit(1);
  return "--";
}
// ========================================================
inline const basepacking_h::Dimension& HardBlockInfoType::operator[](
    int index) const
{
  return in_blocks[index];
}
// --------------------------------------------------------
inline int HardBlockInfoType::blocknum() const
{
  return (in_blocks.size() - 2);
}
// --------------------------------------------------------
inline float HardBlockInfoType::blockArea() const
{
  float sum = 0;
  for (int i = 0; i < blocknum(); i++)
    sum += in_blocks[i].width[0] * in_blocks[i].height[0];
  return sum;
}
// ========================================================
void PrintDimensions(float width, float height)
{
  std::cout << "width:  " << width << std::endl;
  std::cout << "height: " << height << std::endl;
}
// --------------------------------------------------------
void PrintAreas(float deadspace, float blockArea)
{
  std::cout << "total area: " << std::setw(11) << deadspace + blockArea
            << std::endl;
  std::cout << "block area: " << std::setw(11) << blockArea << std::endl;
  std::cout << "dead space: " << std::setw(11) << deadspace << " ("
            << (deadspace / blockArea) * 100 << "%)" << std::endl;
}
// --------------------------------------------------------
void PrintUtilization(float deadspace, float blockArea)
{
  float totalArea = deadspace + blockArea;
  std::cout << "area usage   (wrt. total area): "
            << ((1 - (deadspace / totalArea)) * 100) << "%" << std::endl;
  std::cout << "dead space % (wrt. total area): "
            << ((deadspace / totalArea) * 100) << "%" << std::endl;
  std::cout << "---------------------------" << std::endl;
}
// ========================================================

#endif
