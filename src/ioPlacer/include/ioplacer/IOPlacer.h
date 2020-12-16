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

namespace ppl {

using odb::Point;
using odb::Rect;

enum class RandomMode
{
  None,
  Full,
  Even,
  Group,
  Invalid
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

struct Constraint
{
  std::string name;
  Direction direction;
  Interval interval;
  Constraint(std::string name, Direction dir, Interval interv) :
    name(name), direction(dir), interval(interv)
  {}
};

class IOPlacer
{
 public:
  IOPlacer() = default;
  ~IOPlacer();
  void init(ord::OpenRoad* openroad);
  void run(bool randomMode);
  void printConfig();
  Parameters* getParameters() { return _parms; }
  int returnIONetsHPWL();
  void excludeInterval(Edge edge, int begin, int end);
  void addDirectionConstraint(Direction direction, Edge edge, 
                               int begin, int end);
  void addNameConstraint(std::string name, Edge edge, 
                               int begin, int end);
  void addHorLayer(int layer) { _horLayers.insert(layer); }
  void addVerLayer(int layer) { _verLayers.insert(layer); }
  Edge getEdge(std::string edge);
  Direction getDirection(std::string direction);

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
  std::vector<Constraint> _constraints;

 private:
  void makeComponents();
  void deleteComponents();
  void initNetlistAndCore(std::set<int> horLayerIdx, std::set<int> verLayerIdx);
  void initIOLists();
  void initParms();
  void randomPlacement(const RandomMode);
  void findSlots(const std::set<int>& layers, Edge edge);
  void defineSlots();
  void createSections();
  void setupSections();
  bool assignPinsSections();
  int returnIONetsHPWL(Netlist&);

  void updateOrientation(IOPin&);
  void updatePinArea(IOPin&);
  bool checkBlocked(Edge edge, int pos);

  // db functions
  void populateIOPlacer(std::set<int> horLayerIdx, std::set<int> verLayerIdx);
  void commitIOPlacementToDB(std::vector<IOPin>& assignment);
  void initCore(std::set<int> horLayerIdxs, std::set<int> verLayerIdxs);
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
  std::set<int> _horLayers;
  std::set<int> _verLayers;
  // db variables
  odb::dbDatabase* _db;
  odb::dbTech* _tech;
  odb::dbBlock* _block;
  bool _verbose = false;
};

}  // namespace ppl
#endif /* __IOPLACEMENTKERNEL_H_ */
