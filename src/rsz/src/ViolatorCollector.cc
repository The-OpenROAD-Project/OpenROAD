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

#include "BufferMove.hh"
#include "CloneMove.hh"
#include "Rebuffer.hh"
#include "RepairSetup.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::map;
using std::set;
using std::string;
using utl::RSZ;

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
  for (const sta::Pin* pin : violating_pins_) {
    if (count++ >= numPrint && numPrint > 0) {
      break;
    }
    sta::LibertyPort* port = network_->libertyPort(pin);
    sta::LibertyCell* cell = port->libertyCell();
    float slack = pin_data_.at(pin).slack;
    float tns = pin_data_.at(pin).tns;
    float load_delay = pin_data_.at(pin).load_delay;
    float intrinsic_delay = pin_data_.at(pin).intrinsic_delay;
    float total_delay = load_delay + intrinsic_delay;
    sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "{} ({}) slack={} tns={} level={} delay={} "
               "(load_delay={} + "
               "intrinsic_delay={})",
               vertex->name(network_),
               cell->name(),
               delayAsString(slack, 3, sta_),
               delayAsString(tns, 3, sta_),
               pin_data_.at(pin).level,
               delayAsString(total_delay, 3, sta_),
               delayAsString(load_delay, 3, sta_),
               delayAsString(intrinsic_delay, 3, sta_));
  }
}

int ViolatorCollector::getTotalViolations() const
{
  return violating_endpoints_.size();
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
  for (const sta::Pin* pin : violating_pins_) {
    float slack = sta_->slack(
        pin, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
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
             delayAsString(min_slack, 3, sta_),
             delayAsString(max_slack, 3, sta_),
             delayAsString(bucket_size, 3, sta_));

  // Initialize the bins
  int min_bin_index = static_cast<int>(std::floor(min_slack / bucket_size));
  int max_bin_index = static_cast<int>(std::floor(max_slack / bucket_size));
  for (int i = min_bin_index; i <= max_bin_index; ++i) {
    slack_histogram[i] = 0;
  }

  // Track the slack counts in each bin
  for (const sta::Pin* pin : violating_pins_) {
    float slack = sta_->slack(
        pin, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
    int slack_bucket = static_cast<int>(std::floor(slack / bucket_size));
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               "Pin: {} Slack: {} Bucket: {}",
               network_->pathName(pin),
               delayAsString(slack, 3, sta_),
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
               delayAsString(bin_slack, 3, sta_),
               count,
               bar);
  }
}

// Must be called after STA is initialized
void ViolatorCollector::init(float slack_margin)
{
  graph_ = sta_->graph();
  search_ = sta_->search();
  sdc_ = sta_->cmdMode()->sdc();
  report_ = sta_->report();
  // IMPROVE ME: always looks at cmd scene
  scene_ = sta_->cmdScene();
  dcalc_ap_ = scene_->dcalcAnalysisPtIndex(sta::MinMax::max());
  lib_ap_ = scene_->libertyIndex(sta::MinMax::max());

  slack_margin_ = slack_margin;

  collectViolatingEndpoints();
  collectViolatingStartpoints();

  pin_data_.clear();
  violating_pins_.clear();

  // Initialize endpoint iteration and reset pass tracking
  current_endpoint_index_ = 0;
  current_pass_count_ = 0;
  endpoint_times_considered_.clear();
  if (!violating_endpoints_.empty()) {
    setToEndpoint(0);
  }
  iteration_began_ = false;
}

void ViolatorCollector::collectBySlack()
{
  violating_pins_.clear();

  sta::InstanceSeq insts = network_->leafInstances();
  for (auto inst : insts) {
    auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
        network_->pinIterator(inst));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if (!network_->direction(pin)->isOutput() || network_->isTopLevelPort(pin)
          || sta_->isClock(pin, sta_->cmdMode())) {
        continue;
      }
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      float slack = sta_->slack(
          pin, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
      if (sta::fuzzyLess(slack, slack_margin_)) {
        sta::LibertyPort* port = network_->libertyPort(pin);
        sta::LibertyCell* cell = port->libertyCell();
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   4,
                   "Found violating instance: {} ({}) slack={} level={}",
                   network_->pathName(pin),
                   cell->name(),
                   delayAsString(slack, 3, sta_),
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

std::pair<sta::Delay, sta::Delay> ViolatorCollector::getEffortDelays(
    const sta::Pin* pin)
{
  sta::Vertex* path_vertex = graph_->pinDrvrVertex(pin);
  sta::Delay selected_load_delay = -sta::INF;
  sta::Delay selected_intrinsic_delay = -sta::INF;
  sta::Slack worst_slack = sta::INF;

  // Find the arc (specific edge + rise/fall) with worst slack
  sta::VertexInEdgeIterator edge_iter(path_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* prev_edge = edge_iter.next();
    sta::Vertex* from_vertex = prev_edge->from(graph_);

    // Ignore output-to-output timing arcs (e.g., in asap7 multi-output gates)
    const sta::Pin* from_pin = from_vertex->pin();
    if (!from_pin) {
      continue;
    }
    if (network_->direction(from_pin)->isOutput()) {
      continue;
    }

    const sta::TimingArcSet* arc_set = prev_edge->timingArcSet();
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      sta::TimingArc* prev_arc = arc_set->arcTo(rf);
      // This can happen for flops with only one transition type arc.
      if (!prev_arc) {
        continue;
      }

      // Get the input transition for this arc
      const sta::Transition* from_trans = prev_arc->fromEdge();
      const sta::RiseFall* from_rf = from_trans->asRiseFall();

      // Get slack for the input transition
      sta::Slack from_slack = sta_->slack(from_vertex, from_rf, max_);

      const sta::TimingArc* scene_arc = prev_arc->sceneArc(lib_ap_);
      const sta::Delay intrinsic_delay = scene_arc->intrinsicDelay();
      const sta::Delay delay = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap_);
      const sta::Delay load_delay = delay - intrinsic_delay;

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
                 delayAsString(load_delay, 3, sta_),
                 delayAsString(intrinsic_delay, 3, sta_),
                 delayAsString(from_slack, 3, sta_));

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

void ViolatorCollector::updatePinData(const sta::Pin* pin, pinData& pd)
{
  pd.name = network_->pathName(pin);

  auto [load_delay, intrinsic_delay] = getEffortDelays(pin);
  pd.load_delay = load_delay;
  pd.intrinsic_delay = intrinsic_delay;

  sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
  pd.level = vertex->level();
  pd.slack = sta_->slack(
      pin, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
  pd.tns = getLocalPinTns(pin);
}

int ViolatorCollector::repairsPerPass(int max_repairs_per_pass)
{
  sta::Slack min_viol_ = -sta::INF;
  sta::Slack max_viol_ = 0;
  if (!violating_endpoints_.empty()) {
    min_viol_ = -violating_endpoints_.back().second;
    max_viol_ = -violating_endpoints_.front().second;
  }

  sta::Slack path_slack = getCurrentEndpointSlack();
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
  std::ranges::sort(violating_pins_,
                    [this](const sta::Pin* a, const sta::Pin* b) {
                      sta::Delay load_delay1 = pin_data_[a].load_delay;
                      sta::Delay load_delay2 = pin_data_[b].load_delay;
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
        violating_pins_, [this, load_delay_threshold](const sta::Pin* pin) {
          sta::Delay load_delay = pin_data_[pin].load_delay;
          sta::Delay intrinsic_delay = pin_data_[pin].intrinsic_delay;
          return load_delay <= load_delay_threshold * intrinsic_delay;
        });
    violating_pins_.erase(it.begin(), violating_pins_.end());
  }

  for (auto pin : violating_pins_) {
    sta::Delay worst_load_delay = pin_data_[pin].load_delay;
    sta::Delay worst_intrinsic_delay = pin_data_[pin].intrinsic_delay;
    sta::Vertex* vertex = graph_->pinDrvrVertex(pin);

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               "{} load_delay = {} intrinsic_delay = {}",
               vertex->name(network_),
               delayAsString(worst_load_delay, 3, sta_),
               delayAsString(worst_intrinsic_delay, 3, sta_));
  }
}

// Helper to get the slack of a specific path (by index) for an endpoint
sta::Slack ViolatorCollector::getPathSlackByIndex(const sta::Pin* endpoint_pin,
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
  sta::StringSeq group_names;
  int num_paths_needed = path_index + 1;
  sta::PathEndSeq path_ends
      = search_->findPathEnds(nullptr,                // from
                              nullptr,                // thrus
                              to,                     // to
                              false,                  // unconstrained
                              sta_->scenes(),         // scene
                              sta::MinMaxAll::all(),  // min_max
                              num_paths_needed,       // group_path_count
                              num_paths_needed,       // endpoint_path_count
                              false,                  // unique_pins
                              false,                  // unique_edges
                              -sta::INF,              // slack_min
                              sta::INF,               // slack_max
                              true,                   // sort_by_slack
                              group_names,            // group_names
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

vector<const sta::Pin*> ViolatorCollector::collectViolators(
    int numPathsPerEndpoint,
    int numPins,
    ViolatorSortType sort_type)
{
  violating_pins_.clear();
  set<const sta::Pin*> collected_pins;

  // If current_endpoint_ is set, collect only from that endpoint
  if (current_endpoint_) {
    const sta::Pin* endpoint_pin = current_endpoint_->pin();
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Collecting from current endpoint {} (numPaths={})",
               network_->pathName(endpoint_pin),
               numPathsPerEndpoint);

    // Collect pins from the specified number of paths for this endpoint
    set<const sta::Pin*> new_pins
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
               violating_endpoints_.size());

    // Track which endpoints we've already processed and how many paths
    map<const sta::Pin*, int> endpoint_paths_used;

    // Collect pins by comparing actual path slacks across all endpoints
    // If numPins is -1, collect all pins without limiting
    while (
        (numPins == -1 || collected_pins.size() < static_cast<size_t>(numPins))
        && !violating_endpoints_.empty()) {
      // Find the worst actual path slack among all available paths
      const sta::Pin* best_endpoint = nullptr;
      sta::Slack best_slack = sta::INF;
      int best_path_num = 0;

      // Check each violating endpoint
      for (const auto& [endpoint_pin, endpoint_slack] : violating_endpoints_) {
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
          sta::Slack path_slack = getPathSlackByIndex(endpoint_pin, paths_used);

          // Skip if no path is available (shouldn't happen, but be defensive)
          if (sta::fuzzyGreaterEqual(path_slack, slack_margin_)) {
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
      set<const sta::Pin*> new_pins
          = collectPinsByPathEndpoint(best_endpoint, 1);

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
          delayAsString(best_slack, 3, sta_));

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
  for (const sta::Pin* pin : violating_pins_) {
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
      sortByLocalTns();
      break;
    case ViolatorSortType::SORT_BY_WNS:
      sortByWns();
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

void ViolatorCollector::sortByWns()
{
  std::ranges::sort(
      violating_pins_, [this](const sta::Pin* a, const sta::Pin* b) {
        float slack1 = pin_data_[a].slack;
        float slack2 = pin_data_[b].slack;
        int level1 = pin_data_[a].level;
        int level2 = pin_data_[b].level;

        return slack1 < slack2 || (slack1 == slack2 && level1 > level2)
               || (slack1 == slack2 && level1 == level2
                   && network_->pathNameLess(a, b));
      });
}

sta::Delay ViolatorCollector::getLocalPinTns(const sta::Pin* pin) const
{
  sta::Delay tns = 0;
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(pin);
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* fanout_vertex = edge->to(graph_);
    // Watch out for problematic asap7 output->output timing arcs.
    const sta::Pin* fanout_pin = fanout_vertex->pin();
    if (!fanout_pin) {
      continue;
    }
    if (network_->direction(fanout_pin)->isOutput()) {
      continue;
    }
    const sta::Slack fanout_slack = sta_->slack(fanout_vertex, resizer_->max_);
    if (fanout_slack < 0) {
      tns = sta::delayAsFloat(tns) + sta::delayAsFloat(fanout_slack);
    }
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               4,
               " pin {} fanout {} slack: {} tns: {}",
               network_->pathName(pin),
               network_->pathName(fanout_vertex->pin()),
               delayAsString(fanout_slack, 3, sta_),
               delayAsString(tns, 3, sta_));
  }
  return tns;
}

map<const sta::Pin*, sta::Delay> ViolatorCollector::getLocalTns() const
{
  map<const sta::Pin*, sta::Delay> local_tns;
  for (auto pin : violating_pins_) {
    local_tns[pin] = getLocalPinTns(pin);
  }
  return local_tns;
}

void ViolatorCollector::sortByLocalTns()
{
  std::ranges::sort(
      violating_pins_, [this](const sta::Pin* a, const sta::Pin* b) {
        float tns1 = pin_data_[a].tns;
        float tns2 = pin_data_[b].tns;
        sta::Delay load_delay1 = pin_data_[a].load_delay;
        sta::Delay load_delay2 = pin_data_[b].load_delay;
        return tns1 < tns2 || (tns1 == tns2 && load_delay1 > load_delay2)
               || (tns1 == tns2 && load_delay1 == load_delay2
                   && network_->pathNameLess(a, b));
      });
}

void ViolatorCollector::sortByHeuristic(float load_delay_threshold)
{
  std::ranges::sort(
      violating_pins_, [this](const sta::Pin* a, const sta::Pin* b) {
        // Heuristic: 0.5 * |slack| + 0.5 * load_delay
        sta::Slack slack_a = pin_data_[a].slack;
        sta::Slack slack_b = pin_data_[b].slack;
        sta::Delay load_delay_a = pin_data_[a].load_delay;
        sta::Delay load_delay_b = pin_data_[b].load_delay;

        float score_a = (0.5 * std::abs(slack_a)) + (0.5 * load_delay_a);
        float score_b = (0.5 * std::abs(slack_b)) + (0.5 * load_delay_b);

        return score_a > score_b
               || (score_a == score_b && network_->pathNameLess(a, b));
      });

  // Filter: only keep pins where load_delay > load_delay_threshold *
  // intrinsic_delay If threshold is 0.0 or negative, skip filtering
  if (load_delay_threshold > 0.0) {
    auto it = std::ranges::remove_if(
        violating_pins_, [this, load_delay_threshold](const sta::Pin* pin) {
          sta::Delay load_delay = pin_data_[pin].load_delay;
          sta::Delay intrinsic_delay = pin_data_[pin].intrinsic_delay;
          return load_delay <= load_delay_threshold * intrinsic_delay;
        });
    violating_pins_.erase(it.begin(), violating_pins_.end());
  }
}

void ViolatorCollector::collectViolatingEndpoints()
{
  violating_endpoints_.clear();

  const sta::VertexSet& endpoints = sta_->endpoints();
  for (sta::Vertex* end : endpoints) {
    const sta::Slack end_slack = sta_->slack(end, max_);
    if (sta::fuzzyLess(end_slack, slack_margin_)) {
      violating_endpoints_.emplace_back(end->pin(), end_slack);
    }
  }
  std::ranges::stable_sort(violating_endpoints_,
                           [](const auto& end_slack1, const auto& end_slack2) {
                             return end_slack1.second < end_slack2.second;
                           });

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             1,
             "Violating endpoints {}/{} {}%",
             violating_endpoints_.size(),
             endpoints.size(),
             int(violating_endpoints_.size() / double(endpoints.size()) * 100));
}

void ViolatorCollector::collectViolatingStartpoints()
{
  violating_startpoints_.clear();

  // Collect all startpoints (register outputs + primary inputs, excluding
  // clocks)
  std::vector<std::pair<const sta::Pin*, sta::Slack>> all_startpoints;

  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    const sta::Pin* pin = vertex->pin();

    // Skip clock pins
    if (sta_->isClock(pin, sta_->cmdMode())) {
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
      const sta::Slack start_slack = sta_->slack(vertex, max_);
      all_startpoints.emplace_back(pin, start_slack);
    }
  }

  // Filter for violating startpoints and sort by slack
  for (const auto& start_pair : all_startpoints) {
    if (sta::fuzzyLess(start_pair.second, slack_margin_)) {
      violating_startpoints_.push_back(start_pair);
    }
  }

  std::ranges::stable_sort(
      violating_startpoints_,
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

sta::Slack ViolatorCollector::getStartpointWns(
    const sta::Pin* startpoint_pin) const
{
  // Return worst negative slack for this startpoint
  sta::Vertex* vertex = graph_->pinLoadVertex(startpoint_pin);
  if (!vertex) {
    return 0.0;
  }
  return sta_->slack(vertex, max_);
}

sta::Slack ViolatorCollector::getStartpointTns(
    const sta::Pin* startpoint_pin) const
{
  // Calculate TNS (Total Negative Slack) for all paths from this startpoint
  sta::Vertex* vertex = graph_->pinLoadVertex(startpoint_pin);
  if (!vertex) {
    return 0.0;
  }

  // Sum up all negative slacks in fanout cone of this startpoint
  sta::Slack tns = 0.0;
  std::set<sta::Vertex*> visited;
  std::queue<sta::Vertex*> to_visit;
  to_visit.push(vertex);
  visited.insert(vertex);

  while (!to_visit.empty()) {
    sta::Vertex* v = to_visit.front();
    to_visit.pop();

    sta::Slack v_slack = sta_->slack(v, max_);
    if (v_slack < 0.0) {
      tns = sta::delayAsFloat(tns) + sta::delayAsFloat(v_slack);
    }

    // Traverse fanout
    sta::VertexOutEdgeIterator edge_iter(v, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* to_vertex = edge->to(graph_);
      if (!visited.contains(to_vertex)) {
        visited.insert(to_vertex);
        to_visit.push(to_vertex);
      }
    }
  }

  return tns;
}

sta::Slack ViolatorCollector::getEndpointWns(const sta::Pin* endpoint_pin) const
{
  // Return worst negative slack for this endpoint
  sta::Vertex* vertex = graph_->pinLoadVertex(endpoint_pin);
  if (vertex) {
    return sta_->slack(vertex, max_);
  }
  return 0.0;
}

sta::Slack ViolatorCollector::getEndpointTns(const sta::Pin* endpoint_pin) const
{
  // Calculate TNS (Total Negative Slack) for all paths to this endpoint
  sta::Vertex* vertex = graph_->pinDrvrVertex(endpoint_pin);
  if (!vertex) {
    return 0.0;
  }

  // Sum up all negative slacks in fanin cone of this endpoint
  sta::Slack tns = 0.0;
  std::set<sta::Vertex*> visited;
  std::queue<sta::Vertex*> to_visit;
  to_visit.push(vertex);
  visited.insert(vertex);

  while (!to_visit.empty()) {
    sta::Vertex* v = to_visit.front();
    to_visit.pop();

    sta::Slack v_slack = sta_->slack(v, max_);
    if (v_slack < 0.0) {
      tns = sta::delayAsFloat(tns) + sta::delayAsFloat(v_slack);
    }

    // Traverse fanin
    sta::VertexInEdgeIterator edge_iter(v, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* from_vertex = edge->from(graph_);
      if (!visited.contains(from_vertex)) {
        visited.insert(from_vertex);
        to_visit.push(from_vertex);
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
  set<const sta::Pin*> viol_pins;

  size_t old_size = 0;
  int endpoint_count = 0;
  for (int i = endPointIndex; i < violating_endpoints_.size(); i++) {
    const auto end_original_slack = violating_endpoints_[i];
    // Only count the critical endpoints
    if (sta::fuzzyLess(end_original_slack.second, slack_margin_)) {
      const sta::Pin* endpoint_pin = end_original_slack.first;
      set<const sta::Pin*> end_pins
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

set<const sta::Pin*> ViolatorCollector::collectPinsByPathEndpoint(
    const sta::Pin* endpoint_pin,
    size_t paths_per_endpoint)

{
  // Create a set to remove duplciates
  set<const sta::Pin*> viol_pins;

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
  sta::StringSeq group_names;
  sta::PathEndSeq path_ends
      = search_->findPathEnds(nullptr,                // from
                              nullptr,                // thrus
                              to,                     // to
                              false,                  // unconstrained
                              sta_->scenes(),         // scene
                              sta::MinMaxAll::all(),  // min_max
                              paths_per_endpoint,     // group_path_count
                              paths_per_endpoint,     // endpoint_path_count
                              false,                  // unique_pins
                              false,                  // unique_edges
                              -sta::INF,              // slack_min
                              sta::INF,               // slack_max
                              true,                   // sort_by_slack
                              group_names,            // group_names
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
               delayAsString(path_slack, 3, sta_),
               expanded.size());

    // 4. Iterate over the nodes (path segments) in the expanded path.
    for (size_t i = 0; i < expanded.size(); i++) {
      const sta::Path* path = expanded.path(i);
      const sta::Pin* pin = path->pin(graph_);
      if (!network_->direction(pin)->isOutput() || network_->isTopLevelPort(pin)
          || sta_->isClock(pin, sta_->cmdMode())) {
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
                     delayAsString(pin_slack, 3, sta_));
          viol_pins.insert(pin);
        }
      }
    }
    debugPrint(logger_, RSZ, "violator_collector", 5, "\n");
  }

  return viol_pins;
}

vector<const sta::Pin*> ViolatorCollector::collectViolatorsFromEndpoints(
    const vector<sta::Vertex*>& endpoints,
    int numPathsPerEndpoint,
    int numPins,
    ViolatorSortType sort_type)
{
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
  for (sta::Vertex* endpoint : endpoints) {
    sta::Pin* endpoint_pin = endpoint->pin();

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               3,
               "Processing endpoint: {} (slack={})",
               endpoint->name(network_),
               delayAsString(sta_->slack(endpoint, max_), 3, sta_));

    // Collect pins from paths through this endpoint
    set<const sta::Pin*> endpoint_pins
        = collectPinsByPathEndpoint(endpoint_pin, numPathsPerEndpoint);

    for (const sta::Pin* pin : endpoint_pins) {
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
  for (const sta::Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const sta::Pin*> ViolatorCollector::collectViolatorsByPin(
    int numPins,
    ViolatorSortType sort_type)
{
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
  for (const sta::Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const sta::Pin*> ViolatorCollector::collectViolatorsBySlackMargin(
    float slack_margin)
{
  // Use efficient fanin cone traversal instead of whole-design scan
  return collectViolatorsByFaninTraversal(slack_margin);
}

vector<const sta::Pin*> ViolatorCollector::collectViolatorsByFaninTraversal(
    float slack_margin,
    ViolatorSortType sort_type)
{
  // Get worst endpoint slack as reference
  sta::Slack worst_slack;
  sta::Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);

  // Calculate threshold: pins with slack < worst_slack + slack_margin
  sta::Slack slack_threshold = worst_slack + slack_margin;

  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators by fanin traversal: worst_slack={}, "
             "slack_threshold={}, slack_margin={}",
             delayAsString(worst_slack, 3, sta_),
             delayAsString(slack_threshold, 3, sta_),
             slack_margin);

  // Find critical endpoints (within slack_margin of worst)
  const sta::VertexSet& all_endpoints = sta_->endpoints();
  std::vector<sta::Vertex*> critical_endpoints;

  for (sta::Vertex* endpoint : all_endpoints) {
    sta::Slack endpoint_slack = sta_->slack(endpoint, max_);
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
             all_endpoints.size());

  // Traverse fanin cones and collect pins
  std::set<sta::Vertex*> visited_vertices;
  std::set<const sta::Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<sta::Vertex*> to_visit;

  // Initialize queue with critical endpoints
  for (sta::Vertex* endpoint : critical_endpoints) {
    to_visit.push(endpoint);
    visited_vertices.insert(endpoint);
  }

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    sta::Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse input edges (fanin)
    sta::VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* from_vertex = edge->from(graph_);

      // Skip if already visited
      if (visited_vertices.contains(from_vertex)) {
        continue;
      }

      const sta::Pin* from_pin = from_vertex->pin();

      // Safety check: skip if no pin
      if (!from_pin) {
        continue;
      }

      // Safety check: stop at clock pins
      if (sta_->isClock(from_pin, sta_->cmdMode())) {
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
        sta::Slack pin_slack
            = sta_->slack(from_pin,
                          sta::RiseFall::rise()->asRiseFallBoth(),
                          sta_->scenes(),
                          max_);
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
  for (const sta::Pin* pin : collected_pins_set) {
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
  for (const sta::Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const sta::Pin*> ViolatorCollector::collectViolatorsByFanoutTraversal(
    sta::Vertex* startpoint,
    ViolatorSortType sort_type,
    sta::Slack slack_threshold)
{
  debugPrint(logger_,
             RSZ,
             "violator_collector",
             2,
             "Collecting violators by fanout traversal from startpoint: {}",
             network_->pathName(startpoint->pin()));

  // Traverse fanout cone and collect pins
  std::set<sta::Vertex*> visited_vertices;
  std::set<const sta::Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<sta::Vertex*> to_visit;

  // Include the startpoint pin itself (e.g., flip-flop output)
  const sta::Pin* startpoint_pin = startpoint->pin();
  if (startpoint_pin) {
    sta::Slack pin_slack = sta_->slack(startpoint_pin,
                                       sta::RiseFall::rise()->asRiseFallBoth(),
                                       sta_->scenes(),
                                       max_);
    if (pin_slack < slack_threshold) {
      collected_pins_set.insert(startpoint_pin);
      debugPrint(logger_,
                 RSZ,
                 "violator_collector",
                 3,
                 "Including startpoint pin: {} slack={}",
                 network_->pathName(startpoint_pin),
                 delayAsString(pin_slack, 3, sta_));
    }
  }

  // Initialize queue with startpoint
  to_visit.push(startpoint);
  visited_vertices.insert(startpoint);

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    sta::Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse output edges (fanout)
    sta::VertexOutEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* to_vertex = edge->to(graph_);

      // Skip if already visited
      if (visited_vertices.contains(to_vertex)) {
        continue;
      }

      const sta::Pin* to_pin = to_vertex->pin();

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
        sta::Instance* inst = network_->instance(to_pin);
        if (inst) {
          // Find the output pin of this instance
          sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
          while (pin_iter->hasNext()) {
            const sta::Pin* inst_pin = pin_iter->next();
            if (network_->direction(inst_pin)->isOutput()) {
              sta::Vertex* out_vertex = graph_->pinDrvrVertex(inst_pin);
              if (out_vertex) {
                // Check slack criterion
                sta::Slack pin_slack
                    = sta_->slack(inst_pin,
                                  sta::RiseFall::rise()->asRiseFallBoth(),
                                  sta_->scenes(),
                                  max_);
                if (pin_slack < slack_threshold) {
                  // Add to collected pins
                  collected_pins_set.insert(inst_pin);

                  debugPrint(logger_,
                             RSZ,
                             "violator_collector",
                             4,
                             "Collected pin: {} slack={}",
                             network_->pathName(inst_pin),
                             delayAsString(pin_slack, 3, sta_));
                }

                // Continue traversing through this output
                if (!visited_vertices.contains(out_vertex)) {
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
  for (const sta::Pin* pin : collected_pins_set) {
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
  for (const sta::Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

vector<const sta::Pin*>
ViolatorCollector::collectViolatorsByFaninTraversalForEndpoint(
    sta::Vertex* endpoint,
    float slack_margin,
    ViolatorSortType sort_type)
{
  // Get worst endpoint slack as reference
  sta::Slack worst_slack;
  sta::Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);

  // Calculate threshold: pins with slack < worst_slack + slack_margin
  sta::Slack slack_threshold = worst_slack + slack_margin;

  const sta::Pin* endpoint_pin = endpoint->pin();
  sta::Slack endpoint_slack = sta_->slack(endpoint, max_);

  debugPrint(
      logger_,
      RSZ,
      "violator_collector",
      3,
      "Collecting violators by fanin traversal for endpoint {}: "
      "endpoint_slack={}, worst_slack={}, slack_threshold={}, slack_margin={}",
      network_->pathName(endpoint_pin),
      delayAsString(endpoint_slack, 3, sta_),
      delayAsString(worst_slack, 3, sta_),
      delayAsString(slack_threshold, 3, sta_),
      slack_margin);

  // Traverse fanin cone and collect pins
  std::set<sta::Vertex*> visited_vertices;
  std::set<const sta::Pin*> collected_pins_set;  // Use set to avoid duplicates
  std::queue<sta::Vertex*> to_visit;

  // Initialize queue with single endpoint
  to_visit.push(endpoint);
  visited_vertices.insert(endpoint);

  int vertices_processed = 0;

  while (!to_visit.empty()) {
    sta::Vertex* current_vertex = to_visit.front();
    to_visit.pop();
    vertices_processed++;

    // Traverse input edges (fanin)
    sta::VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* from_vertex = edge->from(graph_);

      // Skip if already visited
      if (visited_vertices.contains(from_vertex)) {
        continue;
      }

      const sta::Pin* from_pin = from_vertex->pin();

      // Safety check: skip if no pin
      if (!from_pin) {
        continue;
      }

      // Safety check: stop at clock pins
      if (sta_->isClock(from_pin, sta_->cmdMode())) {
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
        sta::Slack pin_slack
            = sta_->slack(from_pin,
                          sta::RiseFall::rise()->asRiseFallBoth(),
                          sta_->scenes(),
                          max_);
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
  for (const sta::Pin* pin : collected_pins_set) {
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
  for (const sta::Pin* pin : violating_pins_) {
    markPinConsidered(pin);
  }

  return violating_pins_;
}

// Helper function: BFS traversal of fanin cone to collect pins worse than
// threshold
void ViolatorCollector::traverseFaninCone(
    sta::Vertex* endpoint,
    std::vector<std::pair<const sta::Pin*, sta::Slack>>& pins_with_slack,
    sta::Slack slack_threshold)
{
  std::set<sta::Vertex*> visited_vertices;
  std::queue<sta::Vertex*> to_visit;

  to_visit.push(endpoint);
  visited_vertices.insert(endpoint);

  while (!to_visit.empty()) {
    sta::Vertex* current_vertex = to_visit.front();
    to_visit.pop();

    sta::VertexInEdgeIterator edge_iter(current_vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* from_vertex = edge->from(graph_);

      if (visited_vertices.contains(from_vertex)) {
        continue;
      }

      const sta::Pin* from_pin = from_vertex->pin();
      if (!from_pin) {
        continue;
      }

      if (sta_->isClock(from_pin, sta_->cmdMode())) {
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
        sta::Slack pin_slack
            = sta_->slack(from_pin,
                          sta::RiseFall::rise()->asRiseFallBoth(),
                          sta_->scenes(),
                          max_);

        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   3,
                   "Visiting gate: {} slack={} threshold={}",
                   network_->pathName(from_pin),
                   delayAsString(pin_slack, 3, sta_),
                   delayAsString(slack_threshold, 3, sta_));

        // Stop traversal at pins with slack >= threshold (not critical enough)
        if (pin_slack >= slack_threshold) {
          debugPrint(logger_,
                     RSZ,
                     "violator_collector",
                     3,
                     "  PRUNED: slack {} >= threshold {} - not critical enough",
                     delayAsString(pin_slack, 3, sta_),
                     delayAsString(slack_threshold, 3, sta_));
          continue;  // Don't collect, don't traverse further
        }

        // Collect pins worse than threshold
        pins_with_slack.emplace_back(from_pin, pin_slack);
        debugPrint(logger_,
                   RSZ,
                   "violator_collector",
                   3,
                   "  COLLECTED: slack {} < threshold {}",
                   delayAsString(pin_slack, 3, sta_),
                   delayAsString(slack_threshold, 3, sta_));

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
        sta::Instance* inst = network_->instance(from_pin);
        if (inst) {
          sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
          while (pin_iter->hasNext()) {
            sta::Pin* inst_pin = pin_iter->next();
            if (network_->direction(inst_pin)->isInput()) {
              sta::Vertex* input_vertex = graph_->pinLoadVertex(inst_pin);
              if (input_vertex && !visited_vertices.contains(input_vertex)) {
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
sta::Slack ViolatorCollector::computeAdaptiveThreshold(
    const std::vector<std::pair<const sta::Pin*, sta::Slack>>& pins_with_slack,
    sta::Slack endpoint_slack,
    int& pin_count)
{
  const float margin_percentages[] = {0.10, 0.20, 0.30, 0.50};
  const int min_target_pins = 50;
  const int max_target_pins = 1000;
  int cone_size = pins_with_slack.size();

  sta::Slack chosen_threshold = endpoint_slack;
  pin_count = 0;

  if (cone_size > 0) {
    for (float margin_pct : margin_percentages) {
      sta::Slack margin = std::abs(endpoint_slack) * margin_pct;
      sta::Slack threshold = endpoint_slack + margin;

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
                 delayAsString(endpoint_slack, 3, sta_),
                 delayAsString(margin, 3, sta_),
                 delayAsString(threshold, 3, sta_),
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
            delayAsString(chosen_threshold, 3, sta_),
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
                  delayAsString(chosen_threshold, 3, sta_),
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
    const std::vector<std::pair<const sta::Pin*, sta::Slack>>& pins_with_slack,
    sta::Slack threshold)
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

vector<const sta::Pin*> ViolatorCollector::collectViolatorsByConeTraversal(
    sta::Vertex* endpoint,
    ViolatorSortType sort_type,
    std::optional<sta::Slack> explicit_threshold)
{
  const sta::Pin* endpoint_pin = endpoint->pin();
  sta::Slack endpoint_slack = sta_->slack(endpoint, max_);

  // EXPLICIT THRESHOLD MODE: Use provided threshold directly
  if (explicit_threshold.has_value()) {
    sta::Slack threshold = explicit_threshold.value();

    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Using explicit cone threshold: {} for endpoint {}",
               delayAsString(threshold, 3, sta_),
               network_->pathName(endpoint_pin));

    // Traverse fanin cone with explicit threshold
    std::vector<std::pair<const sta::Pin*, sta::Slack>> cone_pins_with_slack;
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
        delayAsString(endpoint_slack, 3, sta_));

    // Step 1: Traverse fanin cone to collect all pins with negative slack
    // Start with 0.0 threshold to get all critical pins in the cone
    sta::Slack initial_threshold = 0.0;

    std::vector<std::pair<const sta::Pin*, sta::Slack>> cone_pins_with_slack;
    traverseFaninCone(endpoint, cone_pins_with_slack, initial_threshold);

    // Sort by slack (worst first)
    std::ranges::sort(cone_pins_with_slack, [](const auto& a, const auto& b) {
      return a.second < b.second;
    });

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
               delayAsString(cached_cone_threshold_, 3, sta_),
               network_->pathName(endpoint_pin));

    // Traverse fanin cone stopping at pins >= cached threshold
    std::vector<std::pair<const sta::Pin*, sta::Slack>> cone_pins_with_slack;
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
  for (const sta::Pin* pin : violating_pins_) {
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
           < static_cast<int>(violating_endpoints_.size());
  }
  return !violating_endpoints_.empty();
}

void ViolatorCollector::setToEndpoint(int index)
{
  iteration_began_ = true;
  current_endpoint_index_ = index;
  const auto& end_slack_pair = violating_endpoints_[current_endpoint_index_];
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
    debugPrint(logger_,
               RSZ,
               "violator_collector",
               2,
               "Advancing to next endpoint {}/{} ({})",
               current_endpoint_index_,
               violating_endpoints_.size(),
               network_->pathName(
                   violating_endpoints_[current_endpoint_index_].first));
  }
}

sta::Slack ViolatorCollector::getCurrentEndpointSlack() const
{
  if (current_endpoint_) {
    return sta_->slack(current_endpoint_, max_);
  }
  return 0.0;
}

void ViolatorCollector::useWorstEndpoint(sta::Vertex* end)
{
  current_endpoint_ = end;
}

int ViolatorCollector::getCurrentPass() const
{
  return current_pass_count_;
}

void ViolatorCollector::markEndpointVisitedInWns(const sta::Pin* endpoint)
{
  wns_visited_endpoints_.insert(endpoint);
}

bool ViolatorCollector::wasEndpointVisitedInWns(const sta::Pin* endpoint) const
{
  return wns_visited_endpoints_.contains(endpoint);
}

void ViolatorCollector::clearWnsVisitedEndpoints()
{
  wns_visited_endpoints_.clear();
}

void ViolatorCollector::markPinConsidered(const sta::Pin* pin)
{
  considered_pins_.insert(pin);
}

bool ViolatorCollector::wasPinConsidered(const sta::Pin* pin) const
{
  return considered_pins_.contains(pin);
}

void ViolatorCollector::clearConsideredPins()
{
  considered_pins_.clear();
}

const std::set<const sta::Pin*>& ViolatorCollector::getConsideredPins() const
{
  return considered_pins_;
}

std::vector<const sta::Pin*> ViolatorCollector::getCriticalPinsNeverConsidered()
{
  std::vector<const sta::Pin*> never_considered;

  // Iterate through all instances in the design
  sta::LeafInstanceIterator* inst_iter = network_->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();

    // Check all output pins of this instance
    sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();

      // Only check output (driver) pins
      if (network_->isDriver(pin)) {
        // Skip pins on clock networks
        sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
        if (vertex && sta_->isClock(vertex->pin(), sta_->cmdMode())) {
          continue;
        }

        // Get setup slack for this pin
        sta::Slack slack = sta_->slack(
            pin, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
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
  std::ranges::sort(
      never_considered, [this](const sta::Pin* a, const sta::Pin* b) {
        sta::Slack slack_a = sta_->slack(
            a, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
        sta::Slack slack_b = sta_->slack(
            b, sta::RiseFall::rise()->asRiseFallBoth(), sta_->scenes(), max_);
        return slack_a < slack_b;  // Most negative first
      });

  // Limit to top 500 worst slack pins
  constexpr size_t max_lwns_pins = 500;
  if (never_considered.size() > max_lwns_pins) {
    never_considered.resize(max_lwns_pins);
  }

  // Now sort by load_delay (highest load delay first)
  std::ranges::sort(
      never_considered, [this](const sta::Pin* a, const sta::Pin* b) {
        auto delays_a = getEffortDelays(a);
        auto delays_b = getEffortDelays(b);
        sta::Delay load_delay_a = delays_a.first;
        sta::Delay load_delay_b = delays_b.first;
        return load_delay_a > load_delay_b;  // Highest load delay first
      });

  return never_considered;
}

// Get overall startpoint WNS (worst slack across all start points)
sta::Slack ViolatorCollector::getOverallStartpointWns() const
{
  sta::Slack worst_slack = std::numeric_limits<float>::max();

  for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
    sta::Slack sp_wns = getStartpointWns(startpoint_pin);
    worst_slack = std::min(sp_wns, worst_slack);
  }

  return worst_slack == std::numeric_limits<float>::max() ? sta::Slack(0.0)
                                                          : worst_slack;
}

// Get overall startpoint TNS (total negative slack across all startpoints)
sta::Slack ViolatorCollector::getOverallStartpointTns(bool use_cone) const
{
  sta::Slack total_tns = 0.0;

  for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
    sta::Slack sp_tns;
    if (use_cone) {
      sp_tns = getStartpointTns(startpoint_pin);
    } else {
      sp_tns = getStartpointWns(startpoint_pin);
    }
    if (sp_tns < 0.0) {
      total_tns = sta::delayAsFloat(total_tns) + sta::delayAsFloat(sp_tns);
    }
  }

  return total_tns;
}

sta::Slack ViolatorCollector::getOverallEndpointWns() const
{
  sta::Slack worst_slack = std::numeric_limits<float>::max();

  for (const auto& [endpoint_pin, slack] : violating_endpoints_) {
    sta::Slack sp_wns = getEndpointWns(endpoint_pin);
    worst_slack = std::min(sp_wns, worst_slack);
  }

  return worst_slack == std::numeric_limits<float>::max() ? sta::Slack(0.0)
                                                          : worst_slack;
}

sta::Slack ViolatorCollector::getOverallEndpointTns(bool use_cone) const
{
  if (!use_cone) {
    return sta_->totalNegativeSlack(max_);
  }

  sta::Slack total_tns = 0.0;

  for (const auto& [endpoint_pin, slack] : violating_endpoints_) {
    sta::Slack sp_tns = getEndpointTns(endpoint_pin);
    if (sp_tns < 0.0) {
      total_tns = sta::delayAsFloat(total_tns) + sta::delayAsFloat(sp_tns);
    }
  }

  return total_tns;
}

// Proxy method: return WNS (same for both startpoints and endpoints)
sta::Slack ViolatorCollector::getWns() const
{
  // WNS is the same regardless of whether we look at startpoints or endpoints
  // because the critical path is always from a startpoint to an endpoint
  sta::Slack wns;
  sta::Vertex* worst_vertex = nullptr;
  sta_->worstSlack(max_, wns, worst_vertex);
  return wns;
}

// Proxy method: return TNS based on whether we use startpoints or endpoints
sta::Slack ViolatorCollector::getTns(bool use_startpoints, bool use_cone) const
{
  if (use_startpoints) {
    // For startpoints, sum TNS across all startpoints
    return getOverallStartpointTns(use_cone);
  }
  // For endpoints, use STA's totalNegativeSlack
  return getOverallEndpointTns(use_cone);
}

// Proxy method: return worst pin based on whether we use startpoints or
// endpoints
const sta::Pin* ViolatorCollector::getWorstPin(bool use_startpoints) const
{
  if (use_startpoints) {
    // Find the startpoint with the worst WNS
    const sta::Pin* worst_pin = nullptr;
    sta::Slack worst_slack = std::numeric_limits<float>::max();

    for (const auto& [startpoint_pin, slack] : violating_startpoints_) {
      sta::Slack sp_wns = getStartpointWns(startpoint_pin);
      if (sp_wns < worst_slack) {
        worst_slack = sp_wns;
        worst_pin = startpoint_pin;
      }
    }

    return worst_pin;
  }
  // For endpoints, use STA's worstSlack to get the worst vertex
  sta::Slack wns;
  sta::Vertex* worst_vertex = nullptr;
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

sta::Vertex* ViolatorCollector::getCurrentPoint(bool use_startpoints) const
{
  if (use_startpoints) {
    return getCurrentStartpoint();
  }
  return getCurrentEndpoint();
}

const sta::Pin* ViolatorCollector::getCurrentPointPin(
    bool use_startpoints) const
{
  sta::Vertex* point = getCurrentPoint(use_startpoints);
  return point ? point->pin() : nullptr;
}

// Wrapper function for directional collection with state management.
// The point_index parameter sets the internal state (current startpoint or
// endpoint) then retrieves the vertex for the actual collection. This maintains
// consistency between the ViolatorCollector's iteration state and the
// collection being performed.
vector<const sta::Pin*>
ViolatorCollector::collectViolatorsByDirectionalTraversal(
    bool use_startpoints,
    int point_index,
    sta::Slack slack_threshold,
    ViolatorSortType sort_type)
{
  if (use_startpoints) {
    // For startpoints, use fanout traversal
    setToStartpoint(point_index);
    sta::Vertex* startpoint = getCurrentStartpoint();
    return collectViolatorsByFanoutTraversal(
        startpoint, sort_type, slack_threshold);
  }
  // For endpoints, use fanin (cone) traversal
  setToEndpoint(point_index);
  sta::Vertex* endpoint = getCurrentEndpoint();
  return collectViolatorsByConeTraversal(endpoint, sort_type, slack_threshold);
}

}  // namespace rsz
