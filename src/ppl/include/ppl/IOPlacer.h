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

#pragma once

#include <memory>

#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "Parameters.h"
#include "Slots.h"

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
}  // namespace odb

namespace ppl {

using odb::Point;
using odb::Rect;

using utl::Logger;

// A list of pins that will be placed together in the die boundary
typedef std::vector<odb::dbBTerm*> PinList;

enum class Edge
{
  top,
  bottom,
  left,
  right,
  invalid
};

struct Interval
{
  Edge edge;
  int begin;
  int end;
  Interval() = default;
  Interval(Edge edg, int b, int e) : edge(edg), begin(b), end(e) {}
  Edge getEdge() { return edge; }
  int getBegin() { return begin; }
  int getEnd() { return end; }
};

struct Constraint
{
  PinList pin_list;
  Direction direction;
  Interval interval;
  odb::Rect box;
  Constraint() = default;
  Constraint(PinList pins, Direction dir, Interval interv)
      : pin_list(pins), direction(dir), interval(interv)
  {
    box = odb::Rect(-1, -1, -1, -1);
  }
  Constraint(PinList pins, Direction dir, odb::Rect b)
      : pin_list(pins), direction(dir), box(b)
  {
    interval = Interval(Edge::invalid, -1, -1);
  }
};

struct TopLayerGrid
{
  int layer;
  int x_step;
  int y_step;
  int llx;
  int lly;
  int urx;
  int ury;
  int width;
  int height;
  TopLayerGrid() = default;
  TopLayerGrid(int l, int x_s, int y_s, int x_l, int y_l,
               int x_u, int y_u, int w, int h)
    : layer(l), x_step(x_s), y_step(y_s), llx(x_l), lly(y_l),
      urx(x_u), ury(y_u), width(w), height(h) {}
};

class IOPlacer
{
 public:
  void init(odb::dbDatabase* db, Logger* logger);
  void clear();
  void run(bool random_mode);
  void printConfig();
  Parameters* getParameters() { return parms_.get(); }
  int returnIONetsHPWL();
  void excludeInterval(Edge edge, int begin, int end);
  void addNamesConstraint(PinList* pins, Edge edge, int begin, int end);
  void excludeInterval(Interval interval);
  void addDirectionConstraint(Direction direction,
                              Edge edge,
                              int begin,
                              int end);
  void addTopLayerConstraint(PinList* pins,
                             int x1, int y1,
                             int x2, int y2);
  void addHorLayer(int layer) { hor_layers_.insert(layer); }
  void addVerLayer(int layer) { ver_layers_.insert(layer); }
  Edge getEdge(std::string edge);
  Direction getDirection(std::string direction);
  void addPinGroup(PinList* group);
  void addPinToList(odb::dbBTerm* pin, PinList* pin_group);
  void addTopLayerPinPattern(int layer, int x_step, int y_step,
                             int llx, int lly, int urx, int ury,
                             int width, int height);
  int getTopLayer() { return top_grid_.layer; }
  void placePin(odb::dbBTerm* bterm, int layer, int x, int y, int width, int height);

 private:
  Netlist netlist_;
  Core core_;
  std::vector<IOPin> assignment_;
  bool report_hpwl_;

  int slots_per_section_;
  float slots_increase_factor_;
  float usage_increase_factor_;

  bool force_pin_spread_;
  std::vector<Interval> excluded_intervals_;
  std::vector<Constraint> constraints_;
  std::vector<PinList> pin_groups_;

  Logger* logger_;
  std::unique_ptr<Parameters> parms_;
  Netlist netlist_io_pins_;
  std::vector<Slot> slots_;
  std::vector<Slot> top_layer_slots_;
  std::vector<Section> sections_;
  std::vector<Section> sections_for_constraints_;
  std::vector<IOPin> zero_sink_ios_;
  std::set<int> hor_layers_;
  std::set<int> ver_layers_;
  TopLayerGrid top_grid_;

  // db variables
  odb::dbDatabase* db_;
  odb::dbTech* tech_;
  odb::dbBlock* block_;

  void createTopLayerPinPattern();
  void initNetlistAndCore(std::set<int> hor_layer_idx,
                          std::set<int> ver_layer_idx);
  void initIOLists();
  void initParms();
  void randomPlacement();
  void randomPlacement(std::vector<int> pin_indices, std::vector<int> slot_indices, bool top_layer);
  void findSlots(const std::set<int>& layers, Edge edge);
  void findSlotsForTopLayer();
  std::vector<Section> findSectionsForTopLayer(const odb::Rect& region);
  void defineSlots();
  void findSections(int begin, int end, Edge edge, std::vector<Section>& sections);
  std::vector<Section> createSectionsPerConstraint(const Constraint &constraint);
  void getPinsFromDirectionConstraint(Constraint &constraint);
  void initConstraints();
  void createSectionsPerEdge(Edge edge, const std::set<int>& layers);
  void createSections();
  void setupSections();
  bool assignPinsToSections();
  bool assignPinToSection(IOPin& io_pin, int idx, std::vector<Section>& sections);
  int assignGroupsToSections();
  int assignConstrainedGroupsToSections();
  int assignGroupToSection(const std::vector<int> &io_group,
                           std::vector<Section> &sections);
  void assignConstrainedPinsToSections();
  std::vector<int> findPinsForConstraint(const Constraint &constraint);
  int returnIONetsHPWL(Netlist&);
  void findPinAssignment(std::vector<Section>& sections);

  void updateOrientation(IOPin&);
  void updatePinArea(IOPin&);
  bool checkBlocked(Edge edge, int pos);
  std::vector<Interval> findBlockedIntervals(const odb::Rect& die_area,
                                             const odb::Rect& box);
  void getBlockedRegionsFromMacros();
  void getBlockedRegionsFromDbObstructions();

  // db functions
  void populateIOPlacer(std::set<int> hor_layer_idx,
                        std::set<int> ver_layer_idx);
  void commitIOPlacementToDB(std::vector<IOPin>& assignment);
  void commitIOPinToDB(const IOPin& pin);
  void initCore(std::set<int> hor_layer_idxs, std::set<int> ver_layer_idxs);
  void initNetlist();
  void initTracks();
};

}  // namespace ppl
