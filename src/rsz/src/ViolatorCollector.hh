// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace rsz {

using std::pair;
using std::set;
using std::vector;
using utl::RSZ;

enum class ViolatorSortType
{
  SORT_BY_LOAD_DELAY = 0,
  SORT_AND_FILTER_BY_LOAD_DELAY = 1,
  SORT_BY_WNS = 2,
  SORT_BY_TNS = 3,
  SORT_BY_HEURISTIC = 4,
  SORT_AND_FILTER_BY_HEURISTIC = 5,
  MAX = 6
};

struct pinData
{
  std::string name;
  sta::Delay slack;
  sta::Delay tns;
  float intrinsic_delay;
  float load_delay;
  int level;
};

// Class to collect instances with violating output pins.
class ViolatorCollector
{
 public:
  // Constructor
  ViolatorCollector(Resizer* resizer) : resizer_(resizer)
  {
    logger_ = resizer_->logger_;
    sta_ = resizer_->sta_;
    network_ = resizer_->network_;
    max_ = resizer_->max_;
    dcalc_ap_ = -1;
    lib_ap_ = -1;
    max_passes_per_endpoint_ = 1000;  // Default value
    cached_cone_threshold_ = 0.0;
    needs_threshold_recompute_ = true;  // Force compute on first call
  }

  void init(float slack_margin);
  void printHistogram(int numBins = 20) const;
  void printViolators(int numPrint) const;

  int repairsPerPass(int max_repairs_per_pass);

  // Endpoint pass tracking
  void setMaxPassesPerEndpoint(int max_passes);
  bool shouldSkipEndpoint() const;
  int getEndpointPassCount() const;
  void resetEndpointPasses();

  // Endpoint iteration - ViolatorCollector manages iteration internally
  bool hasMoreEndpoints() const;
  void advanceToNextEndpoint();
  void setToEndpoint(int index);
  sta::Vertex* getCurrentEndpoint() const { return current_endpoint_; }
  sta::Slack getCurrentEndpointSlack() const;
  sta::Slack getCurrentEndpointOriginalSlack() const
  {
    return current_end_original_slack_;
  }
  int getCurrentEndpointIndex() const { return current_endpoint_index_; }
  int getMaxEndpointCount() const { return violating_endpoints_.size(); }
  int getCurrentPass() const;
  void useWorstEndpoint(sta::Vertex* end);

  // Collect violators for the current endpoint
  vector<const sta::Pin*> collectViolators(
      int numPathsPerEndpoint = 1,
      int numPins = 1000,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY);

  vector<const sta::Pin*> collectViolatorsFromEndpoints(
      const vector<sta::Vertex*>& endpoints,
      int numPathsPerEndpoint,
      int numPins,
      ViolatorSortType sort_type);

  // Collect violators directly by pin slack (not path-based)
  vector<const sta::Pin*> collectViolatorsByPin(
      int numPins,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY);

  // Collect violators within slack margin of worst endpoint
  // Returns pins where:
  // 1. slack < worst_slack + slack_margin
  // 2. load_delay > load_delay_threshold * intrinsic_delay
  vector<const sta::Pin*> collectViolatorsBySlackMargin(float slack_margin);

  // Collect violators by traversing fanin cones of critical endpoints
  // More efficient than whole-design scan
  vector<const sta::Pin*> collectViolatorsByFaninTraversal(
      float slack_margin,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY);

  // Collect violators by traversing fanin cone of a SINGLE endpoint
  // Uses cached adaptive threshold to target ~200 pins
  vector<const sta::Pin*> collectViolatorsByConeTraversal(
      sta::Vertex* endpoint,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY,
      std::optional<sta::Slack> explicit_threshold = std::nullopt);

  // Legacy function - kept for compatibility if needed
  vector<const sta::Pin*> collectViolatorsByFaninTraversalForEndpoint(
      sta::Vertex* endpoint,
      float slack_margin,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY);

  // Collect violators by traversing fanout cone from a startpoint
  // Used for STARTPOINT_FANOUT phase
  vector<const sta::Pin*> collectViolatorsByFanoutTraversal(
      sta::Vertex* startpoint,
      ViolatorSortType sort_type = ViolatorSortType::SORT_BY_LOAD_DELAY,
      sta::Slack slack_threshold = 0.0);

  // Invalidate cached cone threshold (e.g., after major design changes)
  void invalidateConeThreshold() { needs_threshold_recompute_ = true; }

  // For statistics on critical paths
  int getTotalViolations() const;

  // Get current number of violating endpoints
  int getNumViolatingEndpoints() const { return violating_endpoints_.size(); }

  // Public utility methods
  const char* getEnumString(ViolatorSortType sort_type);

  // Public endpoint collection for TNS Phase
  void collectViolatingEndpoints();
  sta::Slack getPathSlackByIndex(const sta::Pin* endpoint_pin, int path_index);
  const vector<std::pair<const sta::Pin*, sta::Slack>>& getViolatingEndpoints()
      const
  {
    return violating_endpoints_;
  }

  // Public startpoint collection for STARTPOINT_FANOUT Phase
  void collectViolatingStartpoints();
  const vector<std::pair<const sta::Pin*, sta::Slack>>&
  getViolatingStartpoints() const
  {
    return violating_startpoints_;
  }

  // Startpoint iteration methods
  void setToStartpoint(int index);
  sta::Vertex* getCurrentStartpoint() const { return current_startpoint_; }
  int getCurrentStartpointIndex() const { return current_startpoint_index_; }
  int getMaxStartpointCount() const { return violating_startpoints_.size(); }

  // Get slack metrics (worst and total across endpoints/startpoints)
  sta::Slack getOverallStartpointWNS() const;
  sta::Slack getOverallStartpointTNS(bool use_cone = false) const;
  sta::Slack getOverallEndpointWNS() const;
  sta::Slack getOverallEndpointTNS(bool use_cone = false) const;

  sta::Slack getEndpointWNS(const sta::Pin* endpoint_pin) const;
  sta::Slack getEndpointTNS(const sta::Pin* endpoint_pin) const;
  sta::Slack getStartpointWNS(const sta::Pin* startpoint_pin) const;
  sta::Slack getStartpointTNS(const sta::Pin* startpoint_pin) const;

  // Proxy methods that return either startpoint or endpoint metrics
  sta::Slack getWNS() const;  // WNS is the same for both start and endpoints
  sta::Slack getTNS(bool use_startpoints, bool use_cone = false) const;
  const sta::Pin* getWorstPin(bool use_startpoints) const;

  // Unified wrapper methods for directional traversal (fanin vs fanout)
  // These dispatch to either endpoint or startpoint methods based on
  // use_startpoints
  void collectViolatingPoints(bool use_startpoints);
  int getMaxPointCount(bool use_startpoints) const;
  void setToPoint(int index, bool use_startpoints);
  sta::Vertex* getCurrentPoint(bool use_startpoints) const;
  const sta::Pin* getCurrentPointPin(bool use_startpoints) const;
  vector<const sta::Pin*> collectViolatorsByDirectionalTraversal(
      bool use_startpoints,
      int point_index,
      sta::Slack slack_threshold,
      ViolatorSortType sort_type);

  // Get all violating pins collected during optimization
  const vector<const sta::Pin*>& getViolatingPins() const
  {
    return violating_pins_;
  }

  // Get pin data for a specific pin (for MoveTracker reporting)
  bool getPinData(const sta::Pin* pin,
                  float& load_delay,
                  float& intrinsic_delay) const
  {
    auto it = pin_data_.find(pin);
    if (it != pin_data_.end()) {
      load_delay = it->second.load_delay;
      intrinsic_delay = it->second.intrinsic_delay;
      return true;
    }
    return false;
  }

  // Get effort delays (load and intrinsic) for a pin
  std::pair<sta::Delay, sta::Delay> getEffortDelays(const sta::Pin* pin);

  // Track endpoints visited during WNS phase
  void markEndpointVisitedInWNS(const sta::Pin* endpoint);
  bool wasEndpointVisitedInWNS(const sta::Pin* endpoint) const;
  void clearWNSVisitedEndpoints();

  // Track pins considered during optimization (for Slack phase)
  void markPinConsidered(const sta::Pin* pin);
  bool wasPinConsidered(const sta::Pin* pin) const;
  void clearConsideredPins();
  const std::set<const sta::Pin*>& getConsideredPins() const;
  std::vector<const sta::Pin*> getCriticalPinsNeverConsidered();

 private:
  void updatePinData(const sta::Pin* pin, pinData& pd);

  set<const sta::Pin*> collectPinsByPathEndpoint(const sta::Pin* endpoint_pin,
                                                 size_t paths_per_endpoint = 1);
  void collectBySlack();
  void collectByPaths(int endPointIndex = 1,
                      int numEndpoints = 1,
                      int numPathsPerEndpoint = 1);

  void sortPins(int numPins = 0,
                ViolatorSortType sort_type
                = ViolatorSortType::SORT_BY_LOAD_DELAY);
  void sortByLoadDelay(float load_delay_threshold = 0.0);
  void sortByWNS();
  void sortByLocalTNS();
  void sortByHeuristic(float load_delay_threshold = 0.0);
  std::map<const sta::Pin*, sta::Delay> getLocalTNS() const;
  sta::Delay getLocalPinTNS(const sta::Pin* pin) const;

  // Helper functions for cone-based collection
  void traverseFaninCone(
      sta::Vertex* endpoint,
      std::vector<std::pair<const sta::Pin*, sta::Slack>>& pins_with_slack,
      sta::Slack slack_threshold = 0.0);
  sta::Slack computeAdaptiveThreshold(
      const std::vector<std::pair<const sta::Pin*, sta::Slack>>&
          pins_with_slack,
      sta::Slack endpoint_slack,
      int& pin_count);
  void collectPinsWithThreshold(
      const std::vector<std::pair<const sta::Pin*, sta::Slack>>&
          pins_with_slack,
      sta::Slack threshold);

  Resizer* resizer_;
  utl::Logger* logger_;
  sta::Sta* sta_;
  sta::Graph* graph_;
  sta::Network* network_;
  sta::dbNetwork* db_network_;
  const sta::MinMax* max_;
  sta::Search* search_;
  sta::Sdc* sdc_;
  sta::Report* report_;
  sta::Scene* scene_;
  int dcalc_ap_;
  int lib_ap_;

  float slack_margin_;
  vector<const sta::Pin*> violating_pins_;
  std::map<const sta::Pin*, pinData> pin_data_;
  vector<std::pair<const sta::Pin*, sta::Slack>> violating_endpoints_;
  vector<std::pair<const sta::Pin*, sta::Slack>> violating_startpoints_;

  // Endpoint pass tracking
  int max_passes_per_endpoint_;
  int current_pass_count_;
  std::map<const sta::Pin*, int> endpoint_times_considered_;

  // Current endpoint iteration state
  bool iteration_began_;
  sta::Vertex* current_endpoint_;
  sta::Slack current_end_original_slack_;
  int current_endpoint_index_;

  // Current startpoint iteration state
  sta::Vertex* current_startpoint_;
  int current_startpoint_index_;

  // Track endpoints visited during WNS phase (for skipping in TNS phase)
  std::set<const sta::Pin*> wns_visited_endpoints_;

  // Track pins considered during optimization (for Slack phase)
  std::set<const sta::Pin*> considered_pins_;

  // Cached cone threshold for adaptive collection
  sta::Slack cached_cone_threshold_;
  bool needs_threshold_recompute_;
};

}  // namespace rsz
