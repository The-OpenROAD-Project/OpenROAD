// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ViolatorCollector.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "Rebuffer.hh"
#include "RepairSetup.hh"
#include "rsz/Resizer.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::map;
using std::set;
using std::string;
using utl::RSZ;

using sta::Edge;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::InstancePinIterator;
using sta::NetConnectedPinIterator;
using sta::Pin;
using sta::Slew;
using sta::TimingArcSet;
using sta::VertexInEdgeIterator;
using sta::VertexIterator;
using sta::VertexOutEdgeIterator;

const char* ViolatorCollector::getEnumString(ViolatorSortType sort_type)
{
  switch (sort_type) {
    case ViolatorSortType::SORT_BY_TNS:
      return "SORT_BY_TNS";
    case ViolatorSortType::SORT_BY_WNS:
      return "SORT_BY_WNS";
    case ViolatorSortType::SORT_BY_LOAD_DELAY:
      return "SORT_BY_LOAD_DELAY";
    case ViolatorSortType::SORT_AND_FILTER_BY_LOAD_DELAY:
      return "SORT_AND_FILTER_BY_LOAD_DELAY";
    case ViolatorSortType::SORT_BY_HEURISTIC:
      return "SORT_BY_HEURISTIC";
    case ViolatorSortType::SORT_AND_FILTER_BY_HEURISTIC:
      return "SORT_AND_FILTER_BY_HEURISTIC";
    default:
      return "UNKNOWN";
  }
}

void ViolatorCollector::printViolators(int numPrint = 0) const
{
  if (violating_pins_.empty()) {
    logger_->info(RSZ, 8, "No violating pins found.");
    return;
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             23,
             "============= Found {} violating pins. =================",
             violating_pins_.size());
  int count = 0;
  for (const Pin* pin : violating_pins_) {
    if (count++ >= numPrint && numPrint > 0) {
      break;
    }
    LibertyPort* port = network_->libertyPort(pin);
    LibertyCell* cell = port->libertyCell();
    float slack = pin_data_.at(pin).slack;
    float tns = pin_data_.at(pin).tns;
    float load_delay = pin_data_.at(pin).load_delay;
    float intrinsic_delay = pin_data_.at(pin).intrinsic_delay;
    float total_delay = load_delay + intrinsic_delay;
    Vertex* vertex = graph_->pinDrvrVertex(pin);
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "{} ({}) slack={} tns={} level={} delay={} "
               "(load_delay={} + "
               "intrinsic_delay={})",
               vertex->name(network_),
               cell->name(),
               delayAsString(slack, sta_, 3),
               delayAsString(tns, sta_, 3),
               pin_data_.at(pin).level,
               delayAsString(total_delay, sta_, 3),
               delayAsString(load_delay, sta_, 3),
               delayAsString(intrinsic_delay, sta_, 3));
  }
}

int ViolatorCollector::getTotalViolations() const
{
  return violating_ends_.size();
}

void ViolatorCollector::printHistogram(int numBins) const
{
  if (violating_pins_.empty()) {
    debugPrint(
        logger_, RSZ, "violator_collector", 1, "No violating pins found.");
    return;
  }
  std::map<int, int> slack_histogram;

  // Dynamically determine the bucket size
  float min_slack = std::numeric_limits<float>::max();
  float max_slack = std::numeric_limits<float>::lowest();
  for (const Pin* pin : violating_pins_) {
    float slack = sta_->pinSlack(pin, max_);
    min_slack = std::min(min_slack, slack);
    max_slack = std::max(max_slack, slack);
  }
  float bucket_size = (max_slack - min_slack) / float(numBins - 1);
  debugPrint(logger_,
             RSZ,
             "violator_collector",
             1,
             "Slack histogram: pins={} min={} max={} bucket_size={}",
             violating_pins_.size(),
             delayAsString(min_slack, sta_, 3),
             delayAsString(max_slack, sta_, 3),
             delayAsString(bucket_size, sta_, 3));

  // Initialize the bins
  int min_bin_index = static_cast<int>(std::floor(min_slack / bucket_size));
  int max_bin_index = static_cast<int>(std::floor(max_slack / bucket_size));
  for (int i = min_bin_index; i <= max_bin_index; ++i) {
    slack_histogram[i] = 0;
  }

  // Track the slack counts in each bin
  for (const Pin* pin : violating_pins_) {
    float slack = sta_->pinSlack(pin, max_);
    int slack_bucket = static_cast<int>(std::floor(slack / bucket_size));
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               "Pin: {} Slack: {} Bucket: {}",
               network_->pathName(pin),
               delayAsString(slack, sta_, 3),
               slack_bucket);
    slack_histogram[slack_bucket]++;
  }

  for (int i = min_bin_index; i <= max_bin_index; ++i) {
    float bin_slack = (i + 1) * bucket_size;
    int count = slack_histogram[i];
    std::string bar(count, '*');
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               1,
               "Slack: < {:7}: ({:4}) {}",
               delayAsString(bin_slack, sta_, 3),
               count,
               bar);
  }
}

// Must be called after STA is initialized
void ViolatorCollector::init(float slack_margin)
{
  graph_ = sta_->graph();
  search_ = sta_->search();
  sdc_ = sta_->sdc();
  report_ = sta_->report();
  // IMPROVE ME: always looks at cmd corner
  corner_ = sta_->cmdCorner();

  slack_margin_ = slack_margin;

  collectViolatingEndpoints();

  pin_data_.clear();
  violating_pins_.clear();

  // Initialize endpoint iteration and reset pass tracking
  current_endpoint_index_ = 0;
  current_pass_count_ = 0;
  endpoint_times_considered_.clear();
  if (!violating_ends_.empty()) {
    setToEndpoint(0);
  }
  iteration_began_ = false;
}

void ViolatorCollector::collectBySlack()
{
  violating_pins_.clear();

  InstanceSeq insts = network_->leafInstances();
  for (auto inst : insts) {
    auto pin_iter
        = std::unique_ptr<InstancePinIterator>(network_->pinIterator(inst));
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      if (!network_->direction(pin)->isOutput() || network_->isTopLevelPort(pin)
          || sta_->isClock(pin)) {
        continue;
      }
      Vertex* vertex = graph_->pinDrvrVertex(pin);
      float slack = sta_->pinSlack(pin, max_);
      if (fuzzyLess(slack, slack_margin_)) {
        LibertyPort* port = network_->libertyPort(pin);
        LibertyCell* cell = port->libertyCell();
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   4,
                   "Found violating instance: {} ({}) slack={} level={}",
                   network_->pathName(pin),
                   cell->name(),
                   delayAsString(slack, sta_, 3),
                   vertex->level());
        violating_pins_.push_back(pin);
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Found {} violating pins.",
             violating_pins_.size());
}

std::pair<Delay, Delay> ViolatorCollector::getEffortDelays(const Pin* pin)
{
  Vertex* path_vertex = graph_->pinDrvrVertex(pin);
  Delay selected_load_delay = -sta::INF;
  Delay selected_intrinsic_delay = -sta::INF;
  Slack worst_slack = sta::INF;

  // Find the arc (specific edge + rise/fall) with worst slack
  VertexInEdgeIterator edge_iter(path_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* prev_edge = edge_iter.next();
    Vertex* from_vertex = prev_edge->from(graph_);

    // Ignore output-to-output timing arcs (e.g., in asap7 multi-output gates)
    const Pin* from_pin = from_vertex->pin();
    if (!from_pin) {
      continue;
    }
    if (network_->direction(from_pin)->isOutput()) {
      continue;
    }

    const TimingArcSet* arc_set = prev_edge->timingArcSet();
    for (const RiseFall* rf : RiseFall::range()) {
      TimingArc* prev_arc = arc_set->arcTo(rf);
      // This can happen for flops with only one transition type arc.
      if (!prev_arc) {
        continue;
      }

      // Get the input transition for this arc
      const sta::Transition* from_trans = prev_arc->fromEdge();
      const RiseFall* from_rf = from_trans->asRiseFall();

      // Get slack for the input transition
      Slack from_slack = sta_->vertexSlack(from_vertex, from_rf, max_);

      const TimingArc* corner_arc = prev_arc->cornerArc(lib_ap_);
      const Delay intrinsic_delay = corner_arc->intrinsicDelay();
      const Delay delay
          = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap_->index());
      const Delay load_delay = delay - intrinsic_delay;

      debugPrint(logger_,
                 RSZ,
                 "violator_collector",
                 5,
                 "Arc: {}({})->{} ({}) Load Delay: {} Intrinsic "
                 "Delay: {} Slack: {}",
                 network_->pathName(from_vertex->pin()),
                 from_rf->name(),
                 network_->pathName(pin),
                 rf->name(),
                 delayAsString(load_delay, sta_, 3),
                 delayAsString(intrinsic_delay, sta_, 3),
                 delayAsString(from_slack, sta_, 3));

      // Select load_delay from arc with most negative slack
      // If slacks are equal, prefer arc with higher load_delay
      if (from_slack < worst_slack
          || (from_slack == worst_slack && load_delay > selected_load_delay)) {
        worst_slack = from_slack;
        selected_load_delay = load_delay;
        selected_intrinsic_delay = intrinsic_delay;
      }
    }
  }

  return {selected_load_delay, selected_intrinsic_delay};
}

void ViolatorCollector::updatePinData(const Pin* pin, pinData& pd)
{
  pd.name = network_->pathName(pin);

  auto [load_delay, intrinsic_delay] = getEffortDelays(pin);
  pd.load_delay = load_delay;
  pd.intrinsic_delay = intrinsic_delay;

  Vertex* vertex = graph_->pinDrvrVertex(pin);
  pd.level = vertex->level();
  pd.slack = sta_->pinSlack(pin, max_);
  pd.tns = getLocalPinTNS(pin);
}

int ViolatorCollector::repairsPerPass(int max_repairs_per_pass)
{
  Slack min_viol_ = -sta::INF;
  Slack max_viol_ = 0;
  if (!violating_ends_.empty()) {
    min_viol_ = -violating_ends_.back().second;
    max_viol_ = -violating_ends_.front().second;
  }

  Slack path_slack = getCurrentEndpointSlack();
  int repairs_per_pass = 1;
  if (max_viol_ - min_viol_ != 0.0) {
    repairs_per_pass
        += std::round((max_repairs_per_pass - 1) * (-path_slack - min_viol_)
                      / (max_viol_ - min_viol_));
  }

  return repairs_per_pass;
}

void ViolatorCollector::sortByLoadDelay(float load_delay_threshold)
{
  std::ranges::sort(violating_pins_, [this](const Pin* a, const Pin* b) {
    Delay load_delay1 = pin_data_[a].load_delay;
    Delay load_delay2 = pin_data_[b].load_delay;
    float tns1 = pin_data_[a].tns;
    float tns2 = pin_data_[b].tns;

    return load_delay1 > load_delay2
           || (load_delay1 == load_delay2 && tns1 < tns2)
           || (load_delay1 == load_delay2 && tns1 == tns2
               && network_->pathNameLess(a, b));
  });

  // Filter: only keep pins where load_delay > load_delay_threshold *
  // intrinsic_delay If threshold is 0.0 or negative, skip filtering
  if (load_delay_threshold > 0.0) {
    auto it = std::ranges::remove_if(
        violating_pins_, [this, load_delay_threshold](const Pin* pin) {
          Delay load_delay = pin_data_[pin].load_delay;
          Delay intrinsic_delay = pin_data_[pin].intrinsic_delay;
          return load_delay <= load_delay_threshold * intrinsic_delay;
        });
    violating_pins_.erase(it.begin(), violating_pins_.end());
  }

  for (auto pin : violating_pins_) {
    Delay worst_load_delay = pin_data_[pin].load_delay;
    Delay worst_intrinsic_delay = pin_data_[pin].intrinsic_delay;
    Vertex* vertex = graph_->pinDrvrVertex(pin);

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               "{} load_delay = {} intrinsic_delay = {}",
               vertex->name(network_),
               delayAsString(worst_load_delay, sta_, 3),
               delayAsString(worst_intrinsic_delay, sta_, 3));
  }
}

// Helper to get the slack of a specific path (by index) for an endpoint
Slack ViolatorCollector::getPathSlackByIndex(const Pin* endpoint_pin,
                                             int path_index)
{
  // Create ExceptionTo for this endpoint
  sta::PinSet* to_pins = new sta::PinSet(network_);
  to_pins->insert(endpoint_pin);
  sta::ExceptionTo* to = sdc_->makeExceptionTo(to_pins,
                                               nullptr,
                                               nullptr,
                                               sta::RiseFallBoth::riseFall(),
                                               sta::RiseFallBoth::riseFall());

  // Find paths to the endpoint - request only up to path_index+1 paths
  int num_paths_needed = path_index + 1;
  sta::PathEndSeq path_ends
      = search_->findPathEnds(nullptr,                // from
                              nullptr,                // thrus
                              to,                     // to
                              false,                  // unconstrained
                              corner_,                // corner
                              sta::MinMaxAll::all(),  // min_max
                              num_paths_needed,       // group_path_count
                              num_paths_needed,       // endpoint_path_count
                              false,                  // unique_pins
                              false,                  // unique_edges
                              -sta::INF,              // slack_min
                              sta::INF,               // slack_max
                              true,                   // sort_by_slack
                              nullptr,                // group_names
                              true,
                              false,
                              true,
                              true,
                              true,
                              true);  // checks

  // Return the slack of the requested path index
  if (path_index < static_cast<int>(path_ends.size())) {
    return path_ends[path_index]->slack(search_);
  }

  // If we don't have enough paths, return INF (no path available)
  return sta::INF;
}

vector<const Pin*> ViolatorCollector::collectViolators(
    int numPathsPerEndpoint,
    int numPins,
    ViolatorSortType sort_type)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  violating_pins_.clear();
  set<const Pin*> collected_pins;

  // If current_endpoint_ is set, collect only from that endpoint
  if (current_endpoint_) {
    const Pin* endpoint_pin = current_endpoint_->pin();
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Collecting from current endpoint {} (numPaths={})",
               network_->pathName(endpoint_pin),
               numPathsPerEndpoint);

    // Collect pins from the specified number of paths for this endpoint
    set<const Pin*> new_pins
        = collectPinsByPathEndpoint(endpoint_pin, numPathsPerEndpoint);
    collected_pins.insert(new_pins.begin(), new_pins.end());

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               3,
               "Collected {} unique pins from current endpoint's {} path(s)",
               collected_pins.size(),
               numPathsPerEndpoint);

    // Increment the times this endpoint has been considered
    endpoint_times_considered_[endpoint_pin]++;
  } else {
    // Dynamic endpoint discovery mode - find worst paths across all endpoints
    // Get fresh list of violating endpoints each iteration
    collectViolatingEndpoints();

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Found {} violating endpoints this iteration",
               violating_ends_.size());

    // Track which endpoints we've already processed and how many paths
    map<const Pin*, int> endpoint_paths_used;

    // Collect pins by comparing actual path slacks across all endpoints
    // If numPins is -1, collect all pins without limiting
    while (
        (numPins == -1 || collected_pins.size() < static_cast<size_t>(numPins))
        && !violating_ends_.empty()) {
      // Find the worst actual path slack among all available paths
      const Pin* best_endpoint = nullptr;
      Slack best_slack = sta::INF;
      int best_path_num = 0;

      // Check each violating endpoint
      for (const auto& [endpoint_pin, endpoint_slack] : violating_ends_) {
        // Skip endpoints that have been considered too many times
        int times_considered = endpoint_times_considered_[endpoint_pin];
        if (times_considered >= max_passes_per_endpoint_) {
          debugPrint(logger_,
                     RSZ,
                     "violator_collector",
                     4,
                     "Skipping endpoint {} (considered {} times >= {})",
                     network_->pathName(endpoint_pin),
                     times_considered,
                     max_passes_per_endpoint_);
          continue;
        }

        int paths_used = endpoint_paths_used[endpoint_pin];

        // If we haven't used all paths for this endpoint yet
        if (paths_used < numPathsPerEndpoint) {
          // Get the actual slack of the next available path for this endpoint
          Slack path_slack = getPathSlackByIndex(endpoint_pin, paths_used);

          // Skip if no path is available (shouldn't happen, but be defensive)
          if (fuzzyGreaterEqual(path_slack, slack_margin_)) {
            continue;
          }

          // Compare actual path slack, not endpoint slack
          if (path_slack < best_slack) {
            best_slack = path_slack;
            best_endpoint = endpoint_pin;
            best_path_num = paths_used;
          }
        }
      }

      // No more endpoints/paths to process
      if (best_endpoint == nullptr) {
        break;
      }

      // Collect pins from the selected endpoint/path
      set<const Pin*> new_pins = collectPinsByPathEndpoint(best_endpoint, 1);

      size_t before_size = collected_pins.size();
      collected_pins.insert(new_pins.begin(), new_pins.end());

      debugPrint(
          logger_,
          RSZ,
          "violator_collector",
          3,
          "Collected {} pins ({} new) from endpoint {} path {} (slack={})",
          new_pins.size(),
          collected_pins.size() - before_size,
          network_->pathName(best_endpoint),
          best_path_num + 1,
          delayAsString(best_slack, sta_, 3));

      // Mark that we've used another path from this endpoint
      endpoint_paths_used[best_endpoint]++;

      // Increment the times this endpoint has been considered
      endpoint_times_considered_[best_endpoint]++;

      // Stop if we've reached the desired number of pins (unless numPins is -1)
      if (numPins != -1
          && collected_pins.size() >= static_cast<size_t>(numPins)) {
        break;
      }
    }
  }

  // Convert set to vector
  violating_pins_.clear();
  violating_pins_.insert(
      violating_pins_.end(), collected_pins.begin(), collected_pins.end());

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collected {} unique violating pins",
             violating_pins_.size());

  sortPins(numPins, sort_type);

  // Mark all collected pins as considered for Slack phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  // Auto-increment pass count for current endpoint when collecting violators
  current_pass_count_++;
  debugPrint(logger_,
             RSZ,
             "violator_collector",
             4,
             "Endpoint {} pass count: {}/{}",
             network_->pathName(current_endpoint_->pin()),
             current_pass_count_,
             max_passes_per_endpoint_);

  return violating_pins_;
}

void ViolatorCollector::sortPins(int numPins, ViolatorSortType sort_type)
{
  for (auto pin : violating_pins_) {
    updatePinData(pin, pin_data_[pin]);
  }
  debugPrint(logger_,
             RSZ,
             "violator_collector",
             1,
             "Sorting {} violating pins by {}.",
             violating_pins_.size(),
             getEnumString(sort_type));
  switch (sort_type) {
    case ViolatorSortType::SORT_BY_TNS:
      sortByLocalTNS();
      break;
    case ViolatorSortType::SORT_BY_WNS:
      sortByWNS();
      break;
    case ViolatorSortType::SORT_BY_LOAD_DELAY:
      sortByLoadDelay(0.0);  // No filtering
      break;
    case ViolatorSortType::SORT_AND_FILTER_BY_LOAD_DELAY:
      sortByLoadDelay(0.75);  // Filter: keep only pins where load_delay > 0.75
                              // * intrinsic_delay
      break;
    case ViolatorSortType::SORT_BY_HEURISTIC:
      sortByHeuristic(0.0);  // No filtering
      break;
    case ViolatorSortType::SORT_AND_FILTER_BY_HEURISTIC:
      sortByHeuristic(0.75);  // Filter: keep only pins where load_delay > 0.75
                              // * intrinsic_delay
      break;
    default:
      logger_->error(
          RSZ, 9, "Unknown sort type: {}.", getEnumString(sort_type));
  }

  // Truncate to keep only the top numPins pins (unless numPins is -1)
  if (numPins > 0 && numPins < static_cast<int>(violating_pins_.size())) {
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               1,
               "Keeping only {} of {} violating pins.",
               numPins,
               violating_pins_.size());
    violating_pins_.resize(numPins);
  } else if (numPins == -1) {
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               1,
               "Keeping all {} violating pins (no limit).",
               violating_pins_.size());
  }

  printViolators();
}

void ViolatorCollector::sortByWNS()
{
  std::ranges::sort(violating_pins_, [this](const Pin* a, const Pin* b) {
    float slack1 = pin_data_[a].slack;
    float slack2 = pin_data_[b].slack;
    int level1 = pin_data_[a].level;
    int level2 = pin_data_[b].level;

    return slack1 < slack2 || (slack1 == slack2 && level1 > level2)
           || (slack1 == slack2 && level1 == level2
               && network_->pathNameLess(a, b));
  });
}

Delay ViolatorCollector::getLocalPinTNS(const Pin* pin) const
{
  Delay tns = 0;
  Vertex* drvr_vertex = graph_->pinDrvrVertex(pin);
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    Vertex* fanout_vertex = edge->to(graph_);
    // Watch out for problematic asap7 output->output timing arcs.
    const Pin* fanout_pin = fanout_vertex->pin();
    if (!fanout_pin) {
      continue;
    }
    if (network_->direction(fanout_pin)->isOutput()) {
      continue;
    }
    const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, resizer_->max_);
    if (fanout_slack < 0) {
      tns += fanout_slack;
    }
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               " pin {} fanout {} slack: {} tns: {}",
               network_->pathName(pin),
               network_->pathName(fanout_vertex->pin()),
               delayAsString(fanout_slack, sta_, 3),
               delayAsString(tns, sta_, 3));
  }
  return tns;
}

map<const Pin*, Delay> ViolatorCollector::getLocalTNS() const
{
  map<const Pin*, Delay> local_tns;
  for (auto pin : violating_pins_) {
    local_tns[pin] = getLocalPinTNS(pin);
  }
  return local_tns;
}

void ViolatorCollector::sortByLocalTNS()
{
  std::ranges::sort(violating_pins_, [this](const Pin* a, const Pin* b) {
    float tns1 = pin_data_[a].tns;
    float tns2 = pin_data_[b].tns;
    Delay load_delay1 = pin_data_[a].load_delay;
    Delay load_delay2 = pin_data_[b].load_delay;
    return tns1 < tns2 || (tns1 == tns2 && load_delay1 > load_delay2)
           || (tns1 == tns2 && load_delay1 == load_delay2
               && network_->pathNameLess(a, b));
  });
}

void ViolatorCollector::sortByHeuristic(float load_delay_threshold)
{
  std::ranges::sort(violating_pins_, [this](const Pin* a, const Pin* b) {
    // Heuristic: 0.5 * |slack| + 0.5 * load_delay
    Slack slack_a = pin_data_[a].slack;
    Slack slack_b = pin_data_[b].slack;
    Delay load_delay_a = pin_data_[a].load_delay;
    Delay load_delay_b = pin_data_[b].load_delay;

    float score_a = (0.5 * std::abs(slack_a)) + (0.5 * load_delay_a);
    float score_b = (0.5 * std::abs(slack_b)) + (0.5 * load_delay_b);

    return score_a > score_b
           || (score_a == score_b && network_->pathNameLess(a, b));
  });

  // Filter: only keep pins where load_delay > load_delay_threshold *
  // intrinsic_delay If threshold is 0.0 or negative, skip filtering
  if (load_delay_threshold > 0.0) {
    auto it = std::ranges::remove_if(
        violating_pins_, [this, load_delay_threshold](const Pin* pin) {
          Delay load_delay = pin_data_[pin].load_delay;
          Delay intrinsic_delay = pin_data_[pin].intrinsic_delay;
          return load_delay <= load_delay_threshold * intrinsic_delay;
        });
    violating_pins_.erase(it.begin(), violating_pins_.end());
  }
}

void ViolatorCollector::collectViolatingEndpoints()
{
  violating_ends_.clear();

  const VertexSet* endpoints = sta_->endpoints();
  for (Vertex* end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (fuzzyLess(end_slack, slack_margin_)) {
      violating_ends_.emplace_back(end->pin(), end_slack);
    }
  }
  std::ranges::stable_sort(violating_ends_.begin(),
                           violating_ends_.end(),
                           [](const auto& end_slack1, const auto& end_slack2) {
                             return end_slack1.second < end_slack2.second;
                           });

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             1,
             "Violating endpoints {}/{} {}%",
             violating_ends_.size(),
             endpoints->size(),
             int(violating_ends_.size() / double(endpoints->size()) * 100));
}

void ViolatorCollector::collectViolatingStartpoints()
{
  violating_startpoints_.clear();

  // Collect all startpoints (register outputs + primary inputs, excluding
  // clocks)
  std::vector<std::pair<const Pin*, Slack>> all_startpoints;

  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex* vertex = vertex_iter.next();
    const Pin* pin = vertex->pin();

    // Skip clock pins
    if (sta_->isClock(pin)) {
      continue;
    }

    sta::PortDirection* dir = network_->direction(pin);
    bool is_startpoint = false;

    // Check if it's a primary input
    if (network_->isTopLevelPort(pin) && dir->isAnyInput()) {
      is_startpoint = true;
    }
    // Check if it's a register output (DFF Q pin)
    else if (resizer_->isRegister(vertex) && dir->isAnyOutput()) {
      is_startpoint = true;
    }

    if (is_startpoint) {
      // Get worst slack for this startpoint across all paths originating from
      // it
      const Slack start_slack = sta_->vertexSlack(vertex, max_);
      all_startpoints.emplace_back(pin, start_slack);
    }
  }

  // Filter for violating startpoints and sort by slack
  for (const auto& start_pair : all_startpoints) {
    if (fuzzyLess(start_pair.second, slack_margin_)) {
      violating_startpoints_.push_back(start_pair);
    }
  }

  std::ranges::stable_sort(
      violating_startpoints_.begin(),
      violating_startpoints_.end(),
      [](const auto& start_slack1, const auto& start_slack2) {
        return start_slack1.second < start_slack2.second;
      });

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             1,
             "Violating startpoints {}/{} {}%",
             violating_startpoints_.size(),
             all_startpoints.size(),
             !all_startpoints.empty()
                 ? int(violating_startpoints_.size()
                       / double(all_startpoints.size()) * 100)
                 : 0);
}

Slack ViolatorCollector::getStartpointWNS(const Pin* startpoint_pin)
{
  // Return worst negative slack for this startpoint
  Vertex* vertex = graph_->pinLoadVertex(startpoint_pin);
  if (!vertex) {
    return 0.0;
  }
  return sta_->vertexSlack(vertex, max_);
}

Slack ViolatorCollector::getStartpointTNS(const Pin* startpoint_pin)
{
  // Calculate TNS (Total Negative Slack) for all paths from this startpoint
  Vertex* vertex = graph_->pinLoadVertex(startpoint_pin);
  if (!vertex) {
    return 0.0;
  }

  // Sum up all negative slacks in fanout cone of this startpoint
  Slack tns = 0.0;
  std::set<Vertex*> visited;
  std::queue<Vertex*> to_visit;
  to_visit.push(vertex);
  visited.insert(vertex);

  while (!to_visit.empty()) {
    Vertex* v = to_visit.front();
    to_visit.pop();

    Slack v_slack = sta_->vertexSlack(v, max_);
    if (v_slack < 0.0) {
      tns += v_slack;
    }

    // Traverse fanout
    VertexOutEdgeIterator edge_iter(v, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* to_vertex = edge->to(graph_);
      if (visited.find(to_vertex) == visited.end()) {
        visited.insert(to_vertex);
        to_visit.push(to_vertex);
      }
    }
  }

  return tns;
}

void ViolatorCollector::collectByPaths(int endPointIndex,
                                       int numEndpoints,
                                       int numPathsPerEndpoint)
{
  // For skipping duplicates
  set<const Pin*> viol_pins;

  size_t old_size = 0;
  int endpoint_count = 0;
  for (int i = endPointIndex; i < violating_ends_.size(); i++) {
    const auto end_original_slack = violating_ends_[i];
    // Only count the critical endpoints
    if (fuzzyLess(end_original_slack.second, slack_margin_)) {
      const Pin* endpoint_pin = end_original_slack.first;
      set<const Pin*> end_pins
          = collectPinsByPathEndpoint(endpoint_pin, numPathsPerEndpoint);
      viol_pins.insert(end_pins.begin(), end_pins.end());

      debugPrint(
          logger_,
          RSZ,
          "violator_collector",
          2,
          "Collected {} pins ({} unique) for endpoint {} total collected {}",
          end_pins.size(),
          viol_pins.size() - old_size,
          network_->pathName(endpoint_pin),
          viol_pins.size());
      old_size = viol_pins.size();
      endpoint_count++;
    }
    if (numEndpoints > 0 && endpoint_count >= numEndpoints) {
      break;
    }
  }

  violating_pins_.clear();
  violating_pins_.insert(
      violating_pins_.end(), viol_pins.begin(), viol_pins.end());
}

set<const Pin*> ViolatorCollector::collectPinsByPathEndpoint(
    const sta::Pin* endpoint_pin,
    size_t paths_per_endpoint)

{
  // Create a set to remove duplciates
  set<const Pin*> viol_pins;

  // This uses the old method for a single path at a time
  /*if (paths_per_endpoint == 1) {
    Vertex* end = graph_->pinDrvrVertex(endpoint_pin);
    Path* end_path = sta_->vertexWorstSlackPath(end, max_);
    PathExpanded expanded(end_path, sta_);
    if (expanded.size() > 1) {
      const int path_length = expanded.size();
      const int start_index = expanded.startIndex();
      for (int i = start_index; i < path_length; i++) {
        const Path* path = expanded.path(i);
        const Pin* path_pin = path->pin(sta_);
        Vertex* path_vertex = path->vertex(sta_);
        if (!path_vertex->isDriver(network_)
            || network_->isTopLevelPort(path_pin)) {
          continue;
        }
        viol_pins.insert(path_pin);
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   3,
                   "  - Pin: {}",
                   network_->pathName(path_pin));
      }
      return viol_pins;
    }
  } else {
    // FIXME later
    return viol_pins;
  }
    */

  // This code does not behave properly for single path case. At some points it
  // does not match the above vertexWorstSlackPath method. It seems to be
  // related to the use of the ExceptionTo object.
  // 1. Define the single endpoint for the path search.
  sta::PinSet* to_pins = new sta::PinSet(network_);
  to_pins->insert(endpoint_pin);
  // The ExceptionTo object will be owned and deleted by the SDC.
  sta::ExceptionTo* to = sdc_->makeExceptionTo(to_pins,
                                               nullptr,
                                               nullptr,
                                               sta::RiseFallBoth::riseFall(),
                                               sta::RiseFallBoth::riseFall());

  // 2. Find paths to the endpoint.
  sta::PathEndSeq path_ends
      = search_->findPathEnds(nullptr,                // from
                              nullptr,                // thrus
                              to,                     // to
                              false,                  // unconstrained
                              nullptr,                // corner
                              sta::MinMaxAll::all(),  // min_max
                              paths_per_endpoint,     // group_path_count
                              paths_per_endpoint,     // endpoint_path_count
                              false,                  // unique_pins
                              false,                  // unique_edges
                              -sta::INF,              // slack_min
                              sta::INF,               // slack_max
                              true,                   // sort_by_slack
                              nullptr,                // group_names
                              true,
                              false,
                              true,
                              true,
                              true,
                              true);  // checks

  int path_num = 1;
  for (const sta::PathEnd* path_end : path_ends) {
    // 3. Use PathExpanded to access the individual nodes in the path.
    sta::PathExpanded expanded(path_end->path(), search_);
    float path_slack = path_end->slack(search_);

    if (path_slack > 0.0) {
      break;
    }

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               "Critical path {} (Slack: {}ps) Pins: {}",
               path_num++,
               delayAsString(path_slack, sta_, 3),
               expanded.size());

    // 4. Iterate over the nodes (path segments) in the expanded path.
    for (size_t i = 0; i < expanded.size(); i++) {
      const sta::Path* path = expanded.path(i);
      const sta::Pin* pin = path->pin(graph_);
      if (!network_->direction(pin)->isOutput() || network_->isTopLevelPort(pin)
          || sta_->isClock(pin)) {
        continue;
      }

      const sta::Instance* inst = network_->instance(pin);

      if (network_->isTopLevelPort(pin)) {
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   5,
                   "  - Port: {}",
                   network_->pathName(pin));
      } else if (inst) {
        const sta::LibertyCell* lib_cell = network_->libertyCell(inst);
        if (lib_cell) {
          float pin_slack = path->slack(search_);
          debugPrint(logger_,
                     RSZ,
                     "violator_collector",
                     5,
                     "  - Gate: {} (Type: {}) -> Pin: {} Slack: {}",
                     network_->pathName(inst),
                     lib_cell->name(),
                     network_->portName(pin),
                     delayAsString(pin_slack, sta_, 3));
          viol_pins.insert(pin);
        }
      }
    }
    debugPrint(logger_, RSZ, "violator_collector", 5, "\n");
  }

  return viol_pins;
}

vector<const Pin*> ViolatorCollector::collectViolatorsFromEndpoints(
    const vector<Vertex*>& endpoints,
    int numPathsPerEndpoint,
    int numPins,
    ViolatorSortType sort_type)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  violating_pins_.clear();

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators from {} specific endpoints: paths={}, "
             "pins={}, sort={}",
             endpoints.size(),
             numPathsPerEndpoint,
             numPins,
             getEnumString(sort_type));

  // Collect violating pins from the specified endpoints
  for (Vertex* endpoint : endpoints) {
    Pin* endpoint_pin = endpoint->pin();

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               3,
               "Processing endpoint: {} (slack={})",
               endpoint->name(network_),
               delayAsString(sta_->vertexSlack(endpoint, max_), sta_, 3));

    // Collect pins from paths through this endpoint
    set<const Pin*> endpoint_pins
        = collectPinsByPathEndpoint(endpoint_pin, numPathsPerEndpoint);

    for (const Pin* pin : endpoint_pins) {
      violating_pins_.push_back(pin);
    }
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collected {} violating pins from {} endpoints",
             violating_pins_.size(),
             endpoints.size());

  // Sort and limit the pins
  sortPins(numPins, sort_type);

  // Mark all collected pins as considered for Slack phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const Pin*> ViolatorCollector::collectViolatorsByPin(
    int numPins,
    ViolatorSortType sort_type)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators by pin slack: pins={}, sort={}",
             numPins,
             getEnumString(sort_type));

  // Collect all pins with negative slack directly (not path-based)
  collectBySlack();

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Found {} total violating pins",
             violating_pins_.size());

  // Sort and limit the pins
  sortPins(numPins, sort_type);

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "After sorting and limiting: {} pins",
             violating_pins_.size());

  // Mark all collected pins as considered for Slack phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const Pin*> ViolatorCollector::collectViolatorsBySlackMargin(
    float slack_margin)
{
  // Use efficient fanin cone traversal instead of whole-design scan
  return collectViolatorsByFaninTraversal(slack_margin);
}

vector<const Pin*> ViolatorCollector::collectViolatorsByFaninTraversal(
    float slack_margin,
    ViolatorSortType sort_type)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  // Get worst endpoint slack as reference
  Slack worst_slack;
  Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);

  // Calculate threshold: pins with slack < worst_slack + slack_margin
  Slack slack_threshold = worst_slack + slack_margin;

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators by fanin traversal: worst_slack={}, "
             "slack_threshold={}, slack_margin={}",
             delayAsString(worst_slack, sta_, 3),
             delayAsString(slack_threshold, sta_, 3),
             slack_margin);

  // Find critical endpoints (within slack_margin of worst)
  const VertexSet* all_endpoints = sta_->endpoints();
  std::vector<Vertex*> critical_endpoints;

  for (Vertex* endpoint : *all_endpoints) {
    Slack endpoint_slack = sta_->vertexSlack(endpoint, max_);
    if (endpoint_slack < slack_threshold) {
      critical_endpoints.push_back(endpoint);
    }
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Found {} critical endpoints out of {} total",
             critical_endpoints.size(),
             all_endpoints->size());

  // Traverse fanin cones and collect pins
  std::set<Vertex*> visited_vertices;
  std::set<const Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<Vertex*> to_visit;

  // Initialize queue with critical endpoints
  for (Vertex* endpoint : critical_endpoints) {
    to_visit.push(endpoint);
    visited_vertices.insert(endpoint);
  }

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse input edges (fanin)
    VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* from_vertex = edge->from(graph_);

      // Skip if already visited
      if (visited_vertices.find(from_vertex) != visited_vertices.end()) {
        continue;
      }

      const Pin* from_pin = from_vertex->pin();

      // Safety check: skip if no pin
      if (!from_pin) {
        continue;
      }

      // Safety check: stop at clock pins
      if (sta_->isClock(from_pin)) {
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   4,
                   "Stopping at clock pin: {}",
                   network_->pathName(from_pin));
        continue;
      }

      // Mark as visited
      visited_vertices.insert(from_vertex);

      // Skip top-level ports
      if (network_->isTopLevelPort(from_pin)) {
        continue;
      }

      // If this is an output pin, it's a candidate for collection
      if (network_->direction(from_pin)->isOutput()) {
        // Check slack criterion
        Slack pin_slack = sta_->pinSlack(from_pin, max_);
        if (pin_slack < slack_threshold) {
          // Add to collected pins
          collected_pins_set.insert(from_pin);

          // Stop at sequential elements (registers)
          if (resizer_->isRegister(from_vertex)) {
            debugPrint(logger_,
                       RSZ,
                       "violator_collector",
                       4,
                       "Stopping at register: {}",
                       network_->pathName(from_pin));
            continue;
          }
        }
      }

      // Continue traversing through this vertex's fanin
      to_visit.push(from_vertex);
    }
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Fanin traversal: processed {} vertices, collected {} unique pins",
             vertices_processed,
             collected_pins_set.size());

  // Convert set to vector and populate pin_data
  violating_pins_.clear();
  for (const Pin* pin : collected_pins_set) {
    violating_pins_.push_back(pin);

    // Populate pin_data for sorting and filtering
    pinData pd;
    updatePinData(pin, pd);
    pin_data_[pin] = pd;
  }

  int pins_before_filter = violating_pins_.size();

  sortPins(0, sort_type);

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "After heuristic filter: {} pins (from {} before filter)",
             violating_pins_.size(),
             pins_before_filter);

  // Mark all collected pins as considered for Slack phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const Pin*> ViolatorCollector::collectViolatorsByFanoutTraversal(
    Vertex* startpoint,
    ViolatorSortType sort_type,
    Slack slack_threshold)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators by fanout traversal from startpoint: {}",
             network_->pathName(startpoint->pin()));

  // Traverse fanout cone and collect pins
  std::set<Vertex*> visited_vertices;
  std::set<const Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<Vertex*> to_visit;

  // Include the startpoint pin itself (e.g., flip-flop output)
  const Pin* startpoint_pin = startpoint->pin();
  if (startpoint_pin) {
    Slack pin_slack = sta_->pinSlack(startpoint_pin, max_);
    if (pin_slack < slack_threshold) {
      collected_pins_set.insert(startpoint_pin);
      debugPrint(logger_,
                 RSZ,
                 "violator_collector",
                 3,
                 "Including startpoint pin: {} slack={}",
                 network_->pathName(startpoint_pin),
                 delayAsString(pin_slack, sta_, 3));
    }
  }

  // Initialize queue with startpoint
  to_visit.push(startpoint);
  visited_vertices.insert(startpoint);

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse output edges (fanout)
    VertexOutEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* to_vertex = edge->to(graph_);

      // Skip if already visited
      if (visited_vertices.find(to_vertex) != visited_vertices.end()) {
        continue;
      }

      const Pin* to_pin = to_vertex->pin();

      // Safety check: skip if no pin
      if (!to_pin) {
        continue;
      }

      // Mark as visited
      visited_vertices.insert(to_vertex);

      // Skip top-level output ports
      if (network_->isTopLevelPort(to_pin)
          && network_->direction(to_pin)->isAnyOutput()) {
        continue;
      }

      // If this is an input pin driving a gate output, collect the output pin
      if (network_->direction(to_pin)->isInput()) {
        Instance* inst = network_->instance(to_pin);
        if (inst) {
          // Find the output pin of this instance
          InstancePinIterator* pin_iter = network_->pinIterator(inst);
          while (pin_iter->hasNext()) {
            const Pin* inst_pin = pin_iter->next();
            if (network_->direction(inst_pin)->isOutput()) {
              Vertex* out_vertex = graph_->pinDrvrVertex(inst_pin);
              if (out_vertex) {
                // Check slack criterion
                Slack pin_slack = sta_->pinSlack(inst_pin, max_);
                if (pin_slack < slack_threshold) {
                  // Add to collected pins
                  collected_pins_set.insert(inst_pin);

                  debugPrint(logger_,
                             RSZ,
                             "violator_collector",
                             4,
                             "Collected pin: {} slack={}",
                             network_->pathName(inst_pin),
                             delayAsString(pin_slack, sta_, 3));
                }

                // Continue traversing through this output
                if (visited_vertices.find(out_vertex)
                    == visited_vertices.end()) {
                  to_visit.push(out_vertex);
                  visited_vertices.insert(out_vertex);
                }
              }
            }
          }
          delete pin_iter;
        }
      }
    }
  }

  debugPrint(
      logger_,
      RSZ,
      "violator_collector",
      2,
      "Fanout traversal: processed {} vertices, collected {} unique pins",
      vertices_processed,
      collected_pins_set.size());

  // Convert set to vector and populate pin_data
  violating_pins_.clear();
  for (const Pin* pin : collected_pins_set) {
    violating_pins_.push_back(pin);

    // Populate pin_data for sorting and filtering
    pinData pd;
    updatePinData(pin, pd);
    pin_data_[pin] = pd;
  }

  int pins_before_filter = violating_pins_.size();

  sortPins(0, sort_type);

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "After heuristic filter: {} pins (from {} before filter)",
             violating_pins_.size(),
             pins_before_filter);

  // Mark all collected pins as considered for Slack phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const Pin*>
ViolatorCollector::collectViolatorsByFaninTraversalForEndpoint(
    Vertex* endpoint,
    float slack_margin,
    ViolatorSortType sort_type)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  // Get worst endpoint slack as reference
  Slack worst_slack;
  Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);

  // Calculate threshold: pins with slack < worst_slack + slack_margin
  Slack slack_threshold = worst_slack + slack_margin;

  const Pin* endpoint_pin = endpoint->pin();
  Slack endpoint_slack = sta_->vertexSlack(endpoint, max_);

  debugPrint(
      logger_,
      RSZ,
      "violator_collector",
      3,
      "Collecting violators by fanin traversal for endpoint {}: "
      "endpoint_slack={}, worst_slack={}, slack_threshold={}, slack_margin={}",
      network_->pathName(endpoint_pin),
      delayAsString(endpoint_slack, sta_, 3),
      delayAsString(worst_slack, sta_, 3),
      delayAsString(slack_threshold, sta_, 3),
      slack_margin);

  // Traverse fanin cone and collect pins
  std::set<Vertex*> visited_vertices;
  std::set<const Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<Vertex*> to_visit;

  // Initialize queue with single endpoint
  to_visit.push(endpoint);
  visited_vertices.insert(endpoint);

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse input edges (fanin)
    VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* from_vertex = edge->from(graph_);

      // Skip if already visited
      if (visited_vertices.find(from_vertex) != visited_vertices.end()) {
        continue;
      }

      const Pin* from_pin = from_vertex->pin();

      // Safety check: skip if no pin
      if (!from_pin) {
        continue;
      }

      // Safety check: stop at clock pins
      if (sta_->isClock(from_pin)) {
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   4,
                   "Stopping at clock pin: {}",
                   network_->pathName(from_pin));
        continue;
      }

      // Mark as visited
      visited_vertices.insert(from_vertex);

      // Skip top-level ports
      if (network_->isTopLevelPort(from_pin)) {
        continue;
      }

      // If this is an output pin, it's a candidate for collection
      if (network_->direction(from_pin)->isOutput()) {
        // Check slack criterion
        Slack pin_slack = sta_->pinSlack(from_pin, max_);
        if (pin_slack < slack_threshold) {
          // Add to collected pins
          collected_pins_set.insert(from_pin);

          // Stop at sequential elements (registers)
          if (resizer_->isRegister(from_vertex)) {
            debugPrint(logger_,
                       RSZ,
                       "violator_collector",
                       4,
                       "Stopping at register: {}",
                       network_->pathName(from_pin));
            continue;
          }
        }
      }

      // Continue traversing through this vertex's fanin
      to_visit.push(from_vertex);
    }
  }

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             3,
             "Fanin traversal for endpoint {}: processed {} vertices, "
             "collected {} unique pins",
             network_->pathName(endpoint_pin),
             vertices_processed,
             collected_pins_set.size());

  // Convert set to vector and populate pin_data
  violating_pins_.clear();
  for (const Pin* pin : collected_pins_set) {
    violating_pins_.push_back(pin);

    // Populate pin_data for sorting and filtering
    pinData pd;
    updatePinData(pin, pd);
    pin_data_[pin] = pd;
  }

  int pins_before_filter = violating_pins_.size();

  // Sort and filter by heuristic
  sortPins(0, ViolatorSortType::SORT_AND_FILTER_BY_HEURISTIC);

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             3,
             "After heuristic filter: {} pins (from {} before filter)",
             violating_pins_.size(),
             pins_before_filter);

  // Mark all collected pins as considered for WNS phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

// Helper function: BFS traversal of fanin cone to collect pins worse than
// threshold
void ViolatorCollector::traverseFaninCone(
    Vertex* endpoint,
    std::vector<std::pair<const Pin*, Slack>>& pins_with_slack,
    Slack slack_threshold)
{
  std::set<Vertex*> visited_vertices;
  std::queue<Vertex*> to_visit;

  to_visit.push(endpoint);
  visited_vertices.insert(endpoint);

  while (!to_visit.empty()) {
    Vertex* current_vertex = to_visit.front();
    to_visit.pop();

    VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* from_vertex = edge->from(graph_);

      if (visited_vertices.find(from_vertex) != visited_vertices.end()) {
        continue;
      }

      const Pin* from_pin = from_vertex->pin();
      if (!from_pin) {
        continue;
      }

      if (sta_->isClock(from_pin)) {
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   4,
                   "Stopping at clock pin: {}",
                   network_->pathName(from_pin));
        continue;
      }

      visited_vertices.insert(from_vertex);

      if (network_->isTopLevelPort(from_pin)) {
        continue;
      }

      // Only traverse through output pins
      if (network_->direction(from_pin)->isOutput()) {
        Slack pin_slack = sta_->pinSlack(from_pin, max_);

        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   3,
                   "Visiting gate: {} slack={} threshold={}",
                   network_->pathName(from_pin),
                   delayAsString(pin_slack, sta_, 3),
                   delayAsString(slack_threshold, sta_, 3));

        // Stop traversal at pins with slack >= threshold (not critical enough)
        if (pin_slack >= slack_threshold) {
          debugPrint(logger_,
                     RSZ,
                     "violator_collector",
                     3,
                     "  PRUNED: slack {} >= threshold {} - not critical enough",
                     delayAsString(pin_slack, sta_, 3),
                     delayAsString(slack_threshold, sta_, 3));
          continue;  // Don't collect, don't traverse further
        }

        // Collect pins worse than threshold
        pins_with_slack.emplace_back(from_pin, pin_slack);
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   3,
                   "  COLLECTED: slack {} < threshold {}",
                   delayAsString(pin_slack, sta_, 3),
                   delayAsString(slack_threshold, sta_, 3));

        // Stop at registers (don't traverse their fanin)
        if (resizer_->isRegister(from_vertex)) {
          debugPrint(logger_,
                     RSZ,
                     "violator_collector",
                     3,
                     "  REGISTER: stopping fanin traversal");
          continue;  // Collected pin, but stop traversal
        }

        // Continue traversing through this gate's fanin
        // Get the instance and add all its input pin vertices to the queue
        Instance* inst = network_->instance(from_pin);
        if (inst) {
          InstancePinIterator* pin_iter = network_->pinIterator(inst);
          while (pin_iter->hasNext()) {
            Pin* inst_pin = pin_iter->next();
            if (network_->direction(inst_pin)->isInput()) {
              Vertex* input_vertex = graph_->pinLoadVertex(inst_pin);
              if (input_vertex
                  && visited_vertices.find(input_vertex)
                         == visited_vertices.end()) {
                to_visit.push(input_vertex);
                visited_vertices.insert(input_vertex);
                debugPrint(logger_,
                           RSZ,
                           "violator_collector",
                           3,
                           "  TRAVERSE: adding input pin {} to queue",
                           network_->pathName(inst_pin));
              }
            }
          }
          delete pin_iter;
        }
      }
    }
  }
}

// Helper function: Compute adaptive threshold using endpoint-relative margins
Slack ViolatorCollector::computeAdaptiveThreshold(
    const std::vector<std::pair<const Pin*, Slack>>& pins_with_slack,
    Slack endpoint_slack,
    int& pin_count)
{
  const float margin_percentages[] = {0.10, 0.20, 0.30, 0.50};
  const int min_target_pins = 50;
  const int max_target_pins = 1000;
  int cone_size = pins_with_slack.size();

  Slack chosen_threshold = endpoint_slack;
  pin_count = 0;

  if (cone_size > 0) {
    for (float margin_pct : margin_percentages) {
      Slack margin = std::abs(endpoint_slack) * margin_pct;
      Slack threshold = endpoint_slack + margin;

      int count = 0;
      for (const auto& pin_slack_pair : pins_with_slack) {
        if (pin_slack_pair.second < threshold) {
          count++;
        }
      }

      debugPrint(logger_,
                 RSZ,
                 "violator_collector",
                 1,
                 "  Trying margin {:.1f}%: endpoint={}, margin={}, "
                 "threshold={}, pin_count={}",
                 margin_pct * 100.0,
                 delayAsString(endpoint_slack, sta_, 3),
                 delayAsString(margin, sta_, 3),
                 delayAsString(threshold, sta_, 3),
                 count);

      if (count >= min_target_pins && count <= max_target_pins) {
        chosen_threshold = threshold;
        pin_count = count;
        logger_->info(
            RSZ,
            218,
            "Adaptive cone threshold: margin={:.0f}% of endpoint slack, "
            "threshold={}, collecting {} of {} pins ({:.1f}%)",
            margin_pct * 100.0,
            delayAsString(chosen_threshold, sta_, 3),
            pin_count,
            cone_size,
            100.0 * pin_count / cone_size);
        return chosen_threshold;
      }
    }

    for (const auto& pin_slack_pair : pins_with_slack) {
      if (pin_slack_pair.second < chosen_threshold) {
        pin_count++;
      }
    }
    logger_->info(RSZ,
                  219,
                  "Adaptive cone threshold: no margin yielded target "
                  "range, using endpoint slack as threshold={}, collecting "
                  "{} of {} pins ({:.1f}%)",
                  delayAsString(chosen_threshold, sta_, 3),
                  pin_count,
                  cone_size,
                  cone_size > 0 ? 100.0 * pin_count / cone_size : 0.0);
  } else {
    chosen_threshold = -1e30;
    logger_->info(RSZ,
                  220,
                  "Adaptive cone threshold: no pins in cone, using very "
                  "negative threshold");
  }

  return chosen_threshold;
}

// Helper function: Collect pins with slack worse than threshold
void ViolatorCollector::collectPinsWithThreshold(
    const std::vector<std::pair<const Pin*, Slack>>& pins_with_slack,
    Slack threshold)
{
  violating_pins_.clear();
  pin_data_.clear();

  for (const auto& pin_slack_pair : pins_with_slack) {
    if (pin_slack_pair.second < threshold) {
      violating_pins_.push_back(pin_slack_pair.first);

      pinData pd;
      updatePinData(pin_slack_pair.first, pd);
      pin_data_[pin_slack_pair.first] = pd;
    }
  }
}

vector<const Pin*> ViolatorCollector::collectViolatorsByConeTraversal(
    Vertex* endpoint,
    ViolatorSortType sort_type,
    std::optional<Slack> explicit_threshold)
{
  dcalc_ap_ = corner_->findDcalcAnalysisPt(sta::MinMax::max());
  lib_ap_ = dcalc_ap_->libertyIndex();

  const Pin* endpoint_pin = endpoint->pin();
  Slack endpoint_slack = sta_->vertexSlack(endpoint, max_);

  // EXPLICIT THRESHOLD MODE: Use provided threshold directly
  if (explicit_threshold.has_value()) {
    Slack threshold = explicit_threshold.value();

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Using explicit cone threshold: {} for endpoint {}",
               delayAsString(threshold, sta_, 3),
               network_->pathName(endpoint_pin));

    // Traverse fanin cone with explicit threshold
    std::vector<std::pair<const Pin*, Slack>> cone_pins_with_slack;
    traverseFaninCone(endpoint, cone_pins_with_slack, threshold);
    collectPinsWithThreshold(cone_pins_with_slack, threshold);

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Explicit threshold collected {} pins for endpoint {}",
               violating_pins_.size(),
               network_->pathName(endpoint_pin));
  }
  // PHASE 1: Determine if we need full traversal or can use cached threshold
  else if (needs_threshold_recompute_) {
    // FULL TRAVERSAL MODE: Collect all pins to compute threshold

    debugPrint(
        logger_,
        RSZ,
        "violator_collector",
        2,
        "Computing adaptive cone threshold for endpoint {}: endpoint_slack={}",
        network_->pathName(endpoint_pin),
        delayAsString(endpoint_slack, sta_, 3));

    // Step 1: Traverse fanin cone to collect all pins with negative slack
    // Start with 0.0 threshold to get all critical pins in the cone
    Slack initial_threshold = 0.0;

    std::vector<std::pair<const Pin*, Slack>> cone_pins_with_slack;
    traverseFaninCone(endpoint, cone_pins_with_slack, initial_threshold);

    // Sort by slack (worst first)
    std::ranges::sort(
        cone_pins_with_slack.begin(),
        cone_pins_with_slack.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    // Step 2: Compute adaptive threshold targeting 50-1000 pins
    int chosen_pin_count = 0;
    cached_cone_threshold_ = computeAdaptiveThreshold(
        cone_pins_with_slack, endpoint_slack, chosen_pin_count);

    // Step 3: Collect pins using computed threshold
    collectPinsWithThreshold(cone_pins_with_slack, cached_cone_threshold_);

    needs_threshold_recompute_ = false;

  } else {
    // FAST MODE: Use cached threshold

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               3,
               "Using cached cone threshold: {} for endpoint {}",
               delayAsString(cached_cone_threshold_, sta_, 3),
               network_->pathName(endpoint_pin));

    // Traverse fanin cone stopping at pins >= cached threshold
    std::vector<std::pair<const Pin*, Slack>> cone_pins_with_slack;
    traverseFaninCone(endpoint, cone_pins_with_slack, cached_cone_threshold_);
    collectPinsWithThreshold(cone_pins_with_slack, cached_cone_threshold_);

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Cached threshold collected {} pins for endpoint {}",
               violating_pins_.size(),
               network_->pathName(endpoint_pin));
  }

  // Check if we should recompute threshold
  // With relative threshold approach, we target 50-1000 pins
  // TEMPORARILY DISABLED FOR TESTING
  /*
  int collected_count = violating_pins_.size();
  const int min_target_pins = 50;
  const int max_target_pins = 1000;

  if (collected_count < min_target_pins || collected_count > max_target_pins) {
    needs_threshold_recompute_ = true;
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Pin count {} outside target range [{}, {}], will recompute "
               "threshold next time",
               collected_count,
               min_target_pins,
               max_target_pins);
  }
  */

  int pins_before_filter = violating_pins_.size();

  // Sort and filter by heuristic
  sortPins(0, sort_type);

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             3,
             "After heuristic filter: {} pins (from {} before filter)",
             violating_pins_.size(),
             pins_before_filter);

  // Mark all collected pins as considered for WNS phase tracking
  for (const Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

void ViolatorCollector::setMaxPassesPerEndpoint(int max_passes)
{
  max_passes_per_endpoint_ = max_passes;
}

bool ViolatorCollector::shouldSkipEndpoint() const
{
  return current_pass_count_ >= max_passes_per_endpoint_;
}

int ViolatorCollector::getEndpointPassCount() const
{
  return current_pass_count_;
}

void ViolatorCollector::resetEndpointPasses()
{
  current_pass_count_ = 0;
  endpoint_times_considered_.clear();
  debugPrint(
      logger_, RSZ, "violator_collector", 2, "Reset endpoint pass tracking");
}

bool ViolatorCollector::hasMoreEndpoints() const
{
  if (iteration_began_) {
    return current_endpoint_index_ + 1
           < static_cast<int>(violating_ends_.size());
  }
  return !violating_ends_.empty();
}

void ViolatorCollector::setToEndpoint(int index)
{
  iteration_began_ = true;
  current_endpoint_index_ = index;
  const auto& end_slack_pair = violating_ends_[current_endpoint_index_];
  current_endpoint_ = graph_->pinLoadVertex(end_slack_pair.first);
  current_end_original_slack_ = end_slack_pair.second;
}

void ViolatorCollector::setToStartpoint(int index)
{
  current_startpoint_index_ = index;
  const auto& start_slack_pair
      = violating_startpoints_[current_startpoint_index_];
  current_startpoint_ = graph_->pinLoadVertex(start_slack_pair.first);
  // Note: For startpoints, we don't use current_end_original_slack_
}

void ViolatorCollector::advanceToNextEndpoint()
{
  if (hasMoreEndpoints()) {
    if (iteration_began_) {
      current_endpoint_index_++;
    }
    current_pass_count_ = 0;  // Reset pass count for new endpoint
    setToEndpoint(current_endpoint_index_);
    debugPrint(
        logger_,
        RSZ,
        "violator_collector",
        2,
        "Advancing to next endpoint {}/{} ({})",
        current_endpoint_index_,
        violating_ends_.size(),
        network_->pathName(violating_ends_[current_endpoint_index_].first));
  }
}

Slack ViolatorCollector::getCurrentEndpointSlack() const
{
  if (current_endpoint_) {
    return sta_->vertexSlack(current_endpoint_, max_);
  }
  return 0.0;
}

void ViolatorCollector::useWorstEndpoint(Vertex* end)
{
  current_endpoint_ = end;
}

int ViolatorCollector::getCurrentPass() const
{
  return current_pass_count_;
}

void ViolatorCollector::markEndpointVisitedInWNS(const Pin* endpoint)
{
  wns_visited_endpoints_.insert(endpoint);
}

bool ViolatorCollector::wasEndpointVisitedInWNS(const Pin* endpoint) const
{
  return wns_visited_endpoints_.contains(endpoint);
}

void ViolatorCollector::clearWNSVisitedEndpoints()
{
  wns_visited_endpoints_.clear();
}

void ViolatorCollector::markPinConsidered(const Pin* pin)
{
  considered_pins_.insert(pin);
}

bool ViolatorCollector::wasPinConsidered(const Pin* pin) const
{
  return considered_pins_.contains(pin);
}

void ViolatorCollector::clearConsideredPins()
{
  considered_pins_.clear();
}

const std::set<const Pin*>& ViolatorCollector::getConsideredPins() const
{
  return considered_pins_;
}

std::vector<const Pin*> ViolatorCollector::getCriticalPinsNeverConsidered()
{
  std::vector<const Pin*> never_considered;

  // Iterate through all instances in the design
  sta::LeafInstanceIterator* inst_iter = network_->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();

    // Check all output pins of this instance
    sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();

      // Only check output (driver) pins
      if (network_->isDriver(pin)) {
        // Skip pins on clock networks
        Vertex* vertex = graph_->pinDrvrVertex(pin);
        if (vertex && search_->isClock(vertex)) {
          continue;
        }

        // Get setup slack for this pin
        Slack slack = sta_->pinSlack(pin, max_);
        float slack_ps = sta::delayAsFloat(slack) * 1e12;

        // Only track pins with negative slack that haven't been considered
        if (slack_ps < 0.0 && !considered_pins_.contains(pin)) {
          never_considered.push_back(pin);
        }
      }
    }
    delete pin_iter;
  }
  delete inst_iter;

  // Sort by most negative slack first
  std::ranges::sort(never_considered.begin(),
                    never_considered.end(),
                    [this](const Pin* a, const Pin* b) {
                      Slack slack_a = sta_->pinSlack(a, max_);
                      Slack slack_b = sta_->pinSlack(b, max_);
                      return slack_a < slack_b;  // Most negative first
                    });

  // Limit to top 500 worst slack pins
  constexpr size_t max_lwns_pins = 500;
  if (never_considered.size() > max_lwns_pins) {
    never_considered.resize(max_lwns_pins);
  }

  // Now sort by load_delay (highest load delay first)
  std::ranges::sort(never_considered.begin(),
                    never_considered.end(),
                    [this](const Pin* a, const Pin* b) {
                      auto delays_a = getEffortDelays(a);
                      auto delays_b = getEffortDelays(b);
                      Delay load_delay_a = delays_a.first;
                      Delay load_delay_b = delays_b.first;
                      return load_delay_a
                             > load_delay_b;  // Highest load delay first
                    });

  return never_considered;
}

// Get overall startpoint WNS (worst slack across all start points)
Slack ViolatorCollector::getOverallStartpointWNS()
{
  Slack worst_slack = std::numeric_limits<float>::max();

  for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
    Slack sp_wns = getStartpointWNS(startpoint_pin);
    worst_slack = std::min(sp_wns, worst_slack);
  }

  return worst_slack == std::numeric_limits<float>::max() ? 0.0 : worst_slack;
}

// Get overall startpoint TNS (total negative slack across all startpoints)
Slack ViolatorCollector::getOverallStartpointTNS()
{
  Slack total_tns = 0.0;

  for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
    Slack sp_tns = getStartpointTNS(startpoint_pin);
    if (sp_tns < 0.0) {
      total_tns += sp_tns;
    }
  }

  return total_tns;
}

// Proxy method: return WNS (same for both startpoints and endpoints)
Slack ViolatorCollector::getWNS() const
{
  // WNS is the same regardless of whether we look at startpoints or endpoints
  // because the critical path is always from a startpoint to an endpoint
  Slack wns;
  Vertex* worst_vertex = nullptr;
  sta_->worstSlack(max_, wns, worst_vertex);
  return wns;
}

// Proxy method: return TNS based on whether we use startpoints or endpoints
Slack ViolatorCollector::getTNS(bool use_startpoints) const
{
  if (use_startpoints) {
    // For startpoints, sum TNS across all startpoints
    return const_cast<ViolatorCollector*>(this)->getOverallStartpointTNS();
  }
  // For endpoints, use STA's totalNegativeSlack
  return sta_->totalNegativeSlack(max_);
}

// Proxy method: return worst pin based on whether we use startpoints or
// endpoints
const Pin* ViolatorCollector::getWorstPin(bool use_startpoints) const
{
  if (use_startpoints) {
    // Find the startpoint with the worst WNS
    const Pin* worst_pin = nullptr;
    Slack worst_slack = std::numeric_limits<float>::max();

    for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
      Slack sp_wns = const_cast<ViolatorCollector*>(this)->getStartpointWNS(
          startpoint_pin);
      if (sp_wns < worst_slack) {
        worst_slack = sp_wns;
        worst_pin = startpoint_pin;
      }
    }

    return worst_pin;
  }
  // For endpoints, use STA's worstSlack to get the worst vertex
  Slack wns;
  Vertex* worst_vertex = nullptr;
  sta_->worstSlack(max_, wns, worst_vertex);
  return worst_vertex ? worst_vertex->pin() : nullptr;
}

// Unified wrapper methods for directional traversal (fanin vs fanout)
void ViolatorCollector::collectViolatingPoints(bool use_startpoints)
{
  if (use_startpoints) {
    collectViolatingStartpoints();
  } else {
    collectViolatingEndpoints();
  }
}

int ViolatorCollector::getMaxPointCount(bool use_startpoints) const
{
  if (use_startpoints) {
    return getMaxStartpointCount();
  }
  return getMaxEndpointCount();
}

void ViolatorCollector::setToPoint(int index, bool use_startpoints)
{
  if (use_startpoints) {
    setToStartpoint(index);
  } else {
    setToEndpoint(index);
  }
}

Vertex* ViolatorCollector::getCurrentPoint(bool use_startpoints) const
{
  if (use_startpoints) {
    return getCurrentStartpoint();
  }
  return getCurrentEndpoint();
}

const Pin* ViolatorCollector::getCurrentPointPin(bool use_startpoints) const
{
  Vertex* point = getCurrentPoint(use_startpoints);
  return point ? point->pin() : nullptr;
}

// Wrapper function for directional collection with state management.
// The point_index parameter sets the internal state (current startpoint or
// endpoint) then retrieves the vertex for the actual collection. This maintains
// consistency between the ViolatorCollector's iteration state and the
// collection being performed.
vector<const Pin*> ViolatorCollector::collectViolatorsByDirectionalTraversal(
    bool use_startpoints,
    int point_index,
    Slack slack_threshold,
    ViolatorSortType sort_type)
{
  if (use_startpoints) {
    // For startpoints, use fanout traversal
    setToStartpoint(point_index);
    Vertex* startpoint = getCurrentStartpoint();
    return collectViolatorsByFanoutTraversal(
        startpoint, sort_type, slack_threshold);
  }
  // For endpoints, use fanin (cone) traversal
  setToEndpoint(point_index);
  Vertex* endpoint = getCurrentEndpoint();
  return collectViolatorsByConeTraversal(endpoint, sort_type, slack_threshold);
}

}  // namespace rsz
