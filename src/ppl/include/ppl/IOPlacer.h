// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "ppl/Parameters.h"
#include "utl/Logger.h"
#include "utl/validation.h"

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

// A list of pins that will be placed together in the die boundary
using PinSet = std::set<odb::dbBTerm*>;
using PinList = std::vector<odb::dbBTerm*>;

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
  invalid,
  polygonEdge,
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
  IOPlacer(odb::dbDatabase* db, utl::Logger* logger);
  ~IOPlacer();
  void clear();
  void clearConstraints();
  void runHungarianMatching();
  void runAnnealing();
  Parameters* getParameters() { return parms_.get(); }
  int64 computeIONetsHPWL();
  void excludeInterval(Edge edge, int begin, int end);
  void addHorLayer(odb::dbTechLayer* layer);
  void addVerLayer(odb::dbTechLayer* layer);
  void placePin(odb::dbBTerm* bterm,
                odb::dbTechLayer* layer,
                int x,
                int y,
                int width,
                int height,
                bool force_to_die_bound,
                bool placed_status);

  void setAnnealingConfig(float temperature,
                          int max_iterations,
                          int perturb_per_iter,
                          float alpha);

  void setRenderer(std::unique_ptr<AbstractIOPlacerRenderer> ioplacer_renderer);
  AbstractIOPlacerRenderer* getRenderer();

  // annealing debug functions
  void setAnnealingDebugOn();
  bool isAnnealingDebugOn() const;

  void setAnnealingDebugPaintInterval(int iters_between_paintings);
  void setAnnealingDebugNoPauseMode(bool no_pause_mode);

  void writePinPlacement(const char* file_name, bool placed);

  static Direction getDirection(const std::string& direction);
  static Edge getEdge(const std::string& edge);

 private:
  void checkPinPlacement();
  bool checkPinConstraints();
  bool checkMirroredPins();
  void reportHPWL();
  void printConfig(bool annealing = false);
  void initNetlistAndCore(const std::set<int>& hor_layer_idx,
                          const std::set<int>& ver_layer_idx);
  std::vector<int> getValidSlots(int first, int last, bool top_layer);
  std::vector<int> findValidSlots(const Constraint& constraint, bool top_layer);
  std::string getSlotsLocation(Edge edge, bool top_layer);
  int placeFallbackPins();
  void assignMirroredPins(IOPin& io_pin, std::vector<IOPin>& assignment);
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
  void findSlots(const std::set<int>& layers,
                 Edge edge,
                 odb::Line line,
                 bool is_die_polygon);
  std::vector<odb::Point> findLayerSlots(int layer,
                                         Edge edge,
                                         odb::Line line,
                                         bool is_die_polygon);
  void initTopLayerGrid();
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
  void initExcludedIntervals();
  Interval findIntervalFromRect(const odb::Rect& rect);
  void getConstraintsFromDB();
  void initConstraints(bool annealing = false);
  void sortConstraints();
  void checkPinsInMultipleConstraints();
  void checkPinsInMultipleGroups();
  bool overlappingConstraints(const Constraint& c1, const Constraint& c2);
  void createSectionsPerEdge(Edge edge, const std::set<int>& layers);
  void createSectionsPerEdgePolygon(odb::Line poly_edge,
                                    const std::set<int>& layers);
  bool isPointOnLine(const odb::Point& point, const odb::Line& line) const;
  void createSections();
  void createSectionsPolygon();
  void addGroupToFallback(const std::vector<int>& pin_group, bool order);
  bool assignPinsToSections(int assigned_pins_count);
  bool assignPinsToSectionsPolygon(int assigned_pins_count);
  bool assignPinToSection(IOPin& io_pin,
                          int idx,
                          std::vector<Section>& sections);
  void assignMirroredPinToSection(IOPin& io_pin);
  int getMirroredPinCost(IOPin& io_pin, const odb::Point& position);
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
  bool isPointInsidePolygon(odb::Point point, const odb::Polygon& polygon);
  void updateOrientationPolygon(IOPin& pin);
  void updatePinArea(IOPin& pin);
  void movePinToTrack(odb::Point& pos,
                      int layer,
                      int width,
                      int height,
                      const odb::Rect& die_boundary);
  Interval getIntervalFromPin(IOPin& io_pin, const odb::Rect& die_boundary);
  bool checkBlocked(Edge edge, odb::Line, const odb::Point& pos, int layer);
  std::vector<Interval> findBlockedIntervals(const odb::Rect& die_area,
                                             const odb::Rect& box);
  void getBlockedRegionsFromMacros();
  void getBlockedRegionsFromDbObstructions();
  Edge getMirroredEdge(const Edge& edge);
  void computeRegionIncrease(const Interval& interval,
                             int num_pins,
                             int& new_begin,
                             int& new_end);
  int getMinDistanceForInterval(const Interval& interval);
  int64_t computeIncrease(int min_dist, int64_t num_pins, int64_t curr_length);

  // db functions
  void findConstraintRegion(const Interval& interval,
                            const odb::Rect& constraint_box,
                            odb::Rect& region);
  void commitIOPlacementToDB(std::vector<IOPin>& assignment);
  void commitIOPinToDB(const IOPin& pin);
  void initCore(const std::set<int>& hor_layer_idxs,
                const std::set<int>& ver_layer_idxs);
  void initNetlist();
  void initTracks();
  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;
  std::string getEdgeString(Edge edge);
  std::string getDirectionString(Direction direction);
  template <class PinSetOrList>
  std::string getPinSetOrListString(const PinSetOrList& group);

  std::unique_ptr<Netlist> netlist_;
  std::unique_ptr<Core> core_;
  std::vector<IOPin> assignment_;

  int slots_per_section_ = 0;
  int top_layer_pins_count_ = 0;
  // set the offset on tracks as 15 to approximate the size of a GCell in global
  // router
  const int num_tracks_offset_ = 15;
  const int pins_per_report_ = 5;
  const int default_min_dist_ = 2;
  int corner_avoidance_ = 0;

  std::vector<Interval> excluded_intervals_;
  std::vector<Constraint> constraints_;
  FallbackPins fallback_pins_;
  std::map<int, std::vector<odb::Rect>> layer_fixed_pins_shapes_;

  utl::Logger* logger_ = nullptr;
  std::unique_ptr<utl::Validator> validator_;
  std::unique_ptr<Parameters> parms_;
  std::vector<Slot> slots_;
  std::vector<Slot> top_layer_slots_;
  std::vector<Section> sections_;
  std::vector<IOPin> zero_sink_ios_;
  std::set<int> hor_layers_;
  std::set<int> ver_layers_;
  std::unique_ptr<odb::dbBlock::dbBTermTopLayerGrid> top_grid_;

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
