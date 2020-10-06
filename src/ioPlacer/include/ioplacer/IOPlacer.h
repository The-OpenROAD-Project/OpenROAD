/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __IOPLACEMENTKERNEL_H_
#define __IOPLACEMENTKERNEL_H_

#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "Parameters.h"
#include "Slots.h"

namespace ord {
class OpenRoad;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
}

namespace ioPlacer {

using odb::Point;
using odb::Rect;

enum RandomMode
{
  None,
  Full,
  Even,
  Group
};

enum class Edge
{
  Top,
  Bottom,
  Left,
  Right,
  Invalid
};

struct Interval
{
  Edge edge;
  int begin;
  int end;
  Interval() = default;
  Interval(Edge edg, int b, int e) :
    edge(edg), begin(b), end(e)
  {}
  Edge getEdge() { return edge; }
  int getBegin() { return begin; }
  int getEnd() { return end; }
};

class IOPlacer
{
 public:
  IOPlacer() = default;
  ~IOPlacer();
  void init(ord::OpenRoad* openroad);
  void run(int horLayer, int verLayer);
  void printConfig();
  Parameters* getParameters() { return _parms; }
  int returnIONetsHPWL();
  void excludeInterval(Edge edge, int begin, int end);
  Edge getEdge(std::string edge);

 protected:
  Netlist _netlist;
  Core _core;
  std::vector<IOPin> _assignment;
  bool _reportHPWL;

  int _slotsPerSection;
  float _slotsIncreaseFactor;

  float _usagePerSection;
  float _usageIncreaseFactor;

  bool _forcePinSpread;
  std::string _blockagesFile;
  std::vector<Interval> _excludedIntervals;

 private:
  void makeComponents();
  void deleteComponents();
  void initNetlistAndCore(int horLayerIdx, int verLayerIdx);
  void initIOLists();
  void initParms();
  void randomPlacement(const RandomMode);
  void defineSlots();
  void createSections();
  void setupSections();
  bool assignPinsSections();
  int returnIONetsHPWL(Netlist&);

  inline void updateOrientation(IOPin&);
  inline void updatePinArea(IOPin&);
  inline bool checkBlocked(Edge edge, int pos);

  // db functions
  void populateIOPlacer(int horLayerIdx, int verLayerIdx);
  void commitIOPlacementToDB(std::vector<IOPin>& assignment, int horLayerIdx,
                             int verLayerIdx);
  void initCore(int horLayerIdx, int verLayerIdx);
  void initNetlist();
  void initTracks();

  ord::OpenRoad* _openroad;
  Parameters* _parms;
  Netlist _netlistIOPins;
  slotVector_t _slots;
  sectionVector_t _sections;
  std::vector<IOPin> _zeroSinkIOs;
  RandomMode _randomMode = RandomMode::Full;
  bool _cellsPlaced = true;
  // db variables
  odb::dbDatabase* _db;
  odb::dbTech* _tech;
  odb::dbBlock* _block;
  bool _verbose = false;
};

}  // namespace ioPlacer
#endif /* __IOPLACEMENTKERNEL_H_ */
