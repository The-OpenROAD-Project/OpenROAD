// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace sta {
class Pin;
class Sta;
class PathEnd;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rsz {

struct PinInfo
{
  const sta::Pin* endpoint;
  std::string gate_type;
  float load_delay;
  float intrinsic_delay;
  float pin_slack;
  float endpoint_slack;

  PinInfo()
      : endpoint(nullptr),
        gate_type("unknown"),
        load_delay(0.0),
        intrinsic_delay(0.0),
        pin_slack(0.0),
        endpoint_slack(0.0)
  {
  }

  PinInfo(const sta::Pin* ep,
          const std::string& gt,
          float ld,
          float id,
          float pin_slk,
          float ep_slk)
      : endpoint(ep),
        gate_type(gt),
        load_delay(ld),
        intrinsic_delay(id),
        pin_slack(pin_slk),
        endpoint_slack(ep_slk)
  {
  }
};

enum class MoveStateType
{
  ATTEMPT = 0,
  ATTEMPT_REJECT = 1,
  ATTEMPT_COMMIT = 2
};

struct MoveStateData
{
  const sta::Pin* pin;
  int order;
  std::string move_type;
  MoveStateType state;

  MoveStateData(const sta::Pin* p, int c, const std::string mt, MoveStateType s)
      : pin(p), order(c), move_type(mt), state(s)
  {
  }

  MoveStateData(const sta::Pin* p, const std::string mt, MoveStateType s)
      : pin(p), order(0), move_type(mt), state(s)
  {
  }
};

// Class to track optimization moves (attempts, commits, rejections) for pins.
// Provides statistics and summaries of which pins were attempted for
// optimization, which moves succeeded, and which failed.
class MoveTracker
{
 public:
  MoveTracker(utl::Logger* logger, sta::Sta* sta);

  // Set the current endpoint being optimized
  void setCurrentEndpoint(const sta::Pin* endpoint_pin);

  // Track all critical pins collected by ViolatorCollector
  void trackCriticalPins(const std::vector<const sta::Pin*>& critical_pins);

  // Track that a pin was identified as a violator for potential optimization
  void trackViolator(const sta::Pin* pin);

  // Track violator with detailed information for reporting
  void trackViolatorWithInfo(const sta::Pin* pin,
                             const std::string& gate_type,
                             float load_delay,
                             float intrinsic_delay,
                             float pin_slack,
                             float endpoint_slack);

  // Track an attempted move on a pin
  void trackMove(const sta::Pin* pin,
                 const std::string& move_type,
                 MoveStateType state);

  // Commit all pending moves (mark them as successful)
  void commitMoves();

  // Reject all pending moves (mark them as failed)
  void rejectMoves();

  // Print statistics summary for current pass and cumulative totals
  void printMoveSummary(const std::string& title);

  // Print per-endpoint optimization statistics
  void printEndpointSummary(const std::string& title);

  // Print three comprehensive optimization reports
  void printSuccessReport(const std::string& title);
  void printFailureReport(const std::string& title);
  void printMissedOpportunitiesReport(const std::string& title);

  // Print pre- and post-optimization slack distribution
  void printSlackDistribution(const std::string& title);

  // Print detailed analysis of endpoints in the top (most critical) bin
  void printTopBinEndpoints(const std::string& title, int max_endpoints = 20);

  // Print histogram of path slacks for the most critical endpoint
  void printCriticalEndpointPathHistogram(const std::string& title);

  // Capture initial slack for all pins (call at start of optimization)
  void captureInitialSlackDistribution();

  // Capture original slack for all endpoints (call once before any phases)
  void captureOriginalEndpointSlack();

  // Capture pre-phase slack for all endpoints (call at start of each phase)
  void capturePrePhaseSlack();

  // Get the visit count for a specific pin
  int getVisitCount(const sta::Pin* pin) const;

  // Clear all tracking data for the current pass
  void clear();

  // Get total statistics
  int getTotalAttempts() const { return total_attempt_count_; }
  int getTotalCommits() const { return total_commit_count_; }
  int getTotalRejects() const { return total_reject_count_; }

 private:
  void clearMoveSummary();

  // Helper function to enumerate paths to an endpoint and count negative slack
  // paths Returns a vector of (path_slack, path_end) pairs for all paths to the
  // endpoint
  std::vector<std::pair<float, const sta::PathEnd*>> enumerateEndpointPaths(
      const sta::Pin* endpoint_pin,
      int max_paths = 100);

  // Helper function to draw a histogram given bin labels and counts
  void drawHistogram(const std::string& title,
                     const std::vector<std::string>& bin_labels,
                     const std::vector<int>& bin_counts,
                     const std::string& value_label = "Slack (ns)",
                     const std::string& count_label = "Count");

  utl::Logger* logger_;
  sta::Sta* sta_;

  // Current endpoint being optimized
  const sta::Pin* current_endpoint_;

  // Current pass tracking
  int move_count_;
  std::map<const sta::Pin*, int> visit_count_;
  std::vector<MoveStateData> moves_;
  std::vector<MoveStateData> pending_moves_;

  // Cumulative statistics across all passes
  int total_move_count_;
  int total_no_attempt_count_;
  int total_attempt_count_;
  int total_reject_count_;
  int total_commit_count_;
  std::map<std::string, std::tuple<int, int, int>> total_move_type_counts_;

  // Per-endpoint tracking: endpoint_pin -> (attempts, rejects, commits)
  std::map<const sta::Pin*, std::tuple<int, int, int>> endpoint_move_counts_;

  // Per-endpoint slack tracking: endpoint_pin -> (original_slack,
  // pre_phase_slack, post_phase_slack)
  // - original_slack: slack at very start of optimization (before any phases)
  // - pre_phase_slack: slack at start of current phase
  // - post_phase_slack: slack at end of current phase
  std::map<const sta::Pin*, std::tuple<float, float, float>> endpoint_slack_;

  // Detailed tracking for final reports (persists across clear() calls)
  // Map: pin -> vector of (move_type, state)
  std::map<const sta::Pin*, std::vector<std::pair<std::string, MoveStateType>>>
      pin_move_history_;

  // Set of all pins that were visited (even if no moves attempted)
  std::set<const sta::Pin*> all_visited_pins_;

  // Set of all critical pins identified by ViolatorCollector
  std::set<const sta::Pin*> all_critical_pins_;

  // Map pin to detailed information (endpoint, gate type, delays)
  std::map<const sta::Pin*, PinInfo> pin_info_;

  // Initial slack distribution (captured at start of optimization)
  std::map<const sta::Pin*, float> initial_pin_slack_;
  std::map<const sta::Pin*, float> initial_endpoint_slack_;
};

}  // namespace rsz
