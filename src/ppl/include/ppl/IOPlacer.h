/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "ppl/Parameters.h"

namespace utl {
class Logger;
}

namespace odb {
class dbBTerm;
class dbBlock;
class dbDatabase;
class dbTech;
class dbTechLayer;
}  // namespace odb

namespace ppl {
class AbstractIOPlacerRenderer;
class Core;
class Interval;
class IOPin;
class Netlist;
class SimulatedAnnealing;
struct Constraint;
struct Section;
struct Slot;
struct TopLayerGrid;

using odb::Point;
using odb::Rect;

using utl::Logger;

struct pinSetComp
{
  bool operator()(const odb::dbBTerm* lhs, const odb::dbBTerm* rhs) const
  {
    return lhs->getId() < rhs->getId();
  }
};

// A list of pins that will be placed together in the die boundary
using PinSet = std::set<odb::dbBTerm*, pinSetComp>;
using PinList = std::vector<odb::dbBTerm*>;
using MirroredPins = std::unordered_map<odb::dbBTerm*, odb::dbBTerm*>;

struct PinGroup
{
  PinList pins;
  bool order;
};

struct PinGroupByIndex
{
  std::vector<int> pin_indices;
  bool order;
};

struct FallbackPins
{
  std::vector<std::pair<std::vector<int>, bool>> groups;
  std::vector<int> pins;
};

enum class Edge
{
  top,
  bottom,
  left,
  right,
  invalid
};

enum class Direction
{
  input,
  output,
  inout,
  feedthru,
  invalid
};

using int64 = std::int64_t;

class IOPlacer
{
 public:
  IOPlacer();
  ~IOPlacer();
  void init(odb::dbDatabase* db, Logger* logger);
  void clear();
  void clearConstraints();
  void run(bool random_mode);
  void runAnnealing(bool random);
  void reportHPWL();
  void printConfig(bool annealing = false);
  Parameters* getParameters() { return parms_.get(); }
  int64 computeIONetsHPWL();
  void excludeInterval(Edge edge, int begin, int end);
  void addNamesConstraint(PinSet* pins, Edge edge, int begin, int end);
  void addDirectionConstraint(Direction direction,
                              Edge edge,
                              int begin,
                              int end);
  void addTopLayerConstraint(PinSet* pins, const odb::Rect& region);
  void addMirroredPins(odb::dbBTerm* bterm1, odb::dbBTerm* bterm2);
  void addHorLayer(odb::dbTechLayer* layer);
  void addVerLayer(odb::dbTechLayer* layer);
  void addPinGroup(PinList* group, bool order);
  void addTopLayerPinPattern(odb::dbTechLayer* layer,
                             int x_step,
                             int y_step,
                             const Rect& region,
                             int pin_width,
                             int pin_height,
                             int keepout);
  odb::dbTechLayer* getTopLayer() const;
  void placePin(odb::dbBTerm* bterm,
                odb::dbTechLayer* layer,
                int x,
                int y,
                int width,
                int height,
                bool force_to_die_bound);

  static Direction getDirection(const std::string& direction);
  static Edge getEdge(const std::string& edge);

  void setAnnealingConfig(float temperature,
                          int max_iterations,
                          int perturb_per_iter,
                          float alpha);
  void checkPinPlacement();

  void setRenderer(std::unique_ptr<AbstractIOPlacerRenderer> ioplacer_renderer);
  AbstractIOPlacerRenderer* getRenderer();

  // annealing debug functions
  void setAnnealingDebugOn();
  bool isAnnealingDebugOn() const;

  void setAnnealingDebugPaintInterval(int iters_between_paintings);
  void setAnnealingDebugNoPauseMode(bool no_pause_mode);

  void writePinPlacement(const char* file_name);

 private:
  void createTopLayerPinPattern();
  void initNetlistAndCore(const std::set<int>& hor_layer_idx,
                          const std::set<int>& ver_layer_idx);
  void initIOLists();
  void initParms();
  std::vector<int> getValidSlots(int first, int last, bool top_layer);
  void randomPlacement();
  void randomPlacement(std::vector<int> pin_indices,
                       std::vector<int> slot_indices,
                       bool top_layer,
                       bool is_group);
  int placeFallbackPins(bool random);
  void assignMirroredPins(IOPin& io_pin,
                          MirroredPins& mirrored_pins,
                          std::vector<IOPin>& assignment);
  int getSlotIdxByPosition(const odb::Point& position,
                           int layer,
                           std::vector<Slot>& slots);
  int getFirstSlotToPlaceGroup(int first_slot,
                               int last_slot,
                               int group_size,
                               bool check_mirrored,
                               IOPin& first_pin);
  void placeFallbackGroup(const std::pair<std::vector<int>, bool>& group,
                          int place_slot);
  void findSlots(const std::set<int>& layers, Edge edge);
  void findSlotsForTopLayer();
  void filterObstructedSlotsForTopLayer();
  std::vector<Section> findSectionsForTopLayer(const odb::Rect& region);
  void defineSlots();
  void findSections(int begin,
                    int end,
                    Edge edge,
                    std::vector<Section>& sections);
  std::vector<Section> createSectionsPerConstraint(Constraint& constraint);
  void getPinsFromDirectionConstraint(Constraint& constraint);
  void initMirroredPins(bool annealing = false);
  void initConstraints(bool annealing = false);
  void sortConstraints();
  void checkPinsInMultipleConstraints();
  void checkPinsInMultipleGroups();
  bool overlappingConstraints(const Constraint& c1, const Constraint& c2);
  void createSectionsPerEdge(Edge edge, const std::set<int>& layers);
  void createSections();
  void addGroupToFallback(const std::vector<int>& pin_group, bool order);
  void setupSections(int assigned_pins_count);
  bool assignPinsToSections(int assigned_pins_count);
  bool assignPinToSection(IOPin& io_pin,
                          int idx,
                          std::vector<Section>& sections);
  void assignMirroredPin(IOPin& io_pin);
  int assignGroupsToSections(int& mirrored_pins_cnt);
  int updateSection(Section& section, std::vector<Slot>& slots);
  int updateConstraintSections(Constraint& constraint);
  void assignConstrainedGroupsToSections(Constraint& constraint,
                                         std::vector<Section>& sections,
                                         int& mirrored_pins_cnt,
                                         bool mirrored_only);
  bool groupHasMirroredPin(const std::vector<int>& group);
  int assignGroupToSection(const std::vector<int>& io_group,
                           std::vector<Section>& sections,
                           bool order);
  std::vector<Section> assignConstrainedPinsToSections(Constraint& constraint,
                                                       int& mirrored_pins_cnt,
                                                       bool mirrored_only);
  std::vector<int> findPinsForConstraint(const Constraint& constraint,
                                         Netlist* netlist,
                                         bool mirrored_only);
  int64 computeIONetsHPWL(Netlist* netlist);
  void findPinAssignment(std::vector<Section>& sections,
                         bool mirrored_groups_only);
  void updateSlots();
  void excludeInterval(Interval interval);

  void updateOrientation(IOPin& pin);
  void updatePinArea(IOPin& pin);
  void movePinToTrack(odb::Point& pos,
                      int layer,
                      int width,
                      int height,
                      const Rect& die_boundary);
  Interval getIntervalFromPin(IOPin& io_pin, const Rect& die_boundary);
  bool checkBlocked(Edge edge, int pos, int layer);
  std::vector<Interval> findBlockedIntervals(const odb::Rect& die_area,
                                             const odb::Rect& box);
  void getBlockedRegionsFromMacros();
  void getBlockedRegionsFromDbObstructions();
  Edge getMirroredEdge(const Edge& edge);
  int computeNewRegionLength(const Interval& interval, int num_pins);
  int64_t computeIncrease(int min_dist, int64_t num_pins, int64_t curr_length);

  // db functions
  void populateIOPlacer(const std::set<int>& hor_layer_idx,
                        const std::set<int>& ver_layer_idx);
  void findConstraintRegion(const Interval& interval,
                            const Rect& constraint_box,
                            Rect& region);
  void commitConstraintsToDB();
  void commitIOPlacementToDB(std::vector<IOPin>& assignment);
  void commitIOPinToDB(const IOPin& pin);
  void initCore(const std::set<int>& hor_layer_idxs,
                const std::set<int>& ver_layer_idxs);
  void initNetlist();
  void initTracks();
  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;
  std::string getEdgeString(Edge edge);

  std::unique_ptr<Netlist> netlist_;
  std::unique_ptr<Core> core_;
  std::vector<IOPin> assignment_;

  int slots_per_section_ = 0;
  float slots_increase_factor_ = 0;
  int top_layer_pins_count_ = 0;
  // set the offset on tracks as 15 to approximate the size of a GCell in global
  // router
  const int num_tracks_offset_ = 15;
  const int pins_per_report_ = 5;
  const int default_min_dist_ = 2;

  std::vector<Interval> excluded_intervals_;
  std::vector<Constraint> constraints_;
  std::vector<PinGroup> pin_groups_;
  MirroredPins mirrored_pins_;
  FallbackPins fallback_pins_;

  Logger* logger_ = nullptr;
  std::unique_ptr<Parameters> parms_;
  std::unique_ptr<Netlist> netlist_io_pins_;
  std::vector<Slot> slots_;
  std::vector<Slot> top_layer_slots_;
  std::vector<Section> sections_;
  std::vector<IOPin> zero_sink_ios_;
  std::set<int> hor_layers_;
  std::set<int> ver_layers_;
  std::unique_ptr<TopLayerGrid> top_grid_;

  std::unique_ptr<AbstractIOPlacerRenderer> ioplacer_renderer_;

  // simulated annealing variables
  float init_temperature_ = 0;
  int max_iterations_ = 0;
  int perturb_per_iter_ = 0;
  float alpha_ = 0;

  // simulated annealing debugger variables
  bool annealing_debug_mode_ = false;

  // db variables
  odb::dbDatabase* db_ = nullptr;
};

}  // namespace ppl
