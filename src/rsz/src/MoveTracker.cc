// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "MoveTracker.hh"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace rsz {

using std::map;
using std::pair;
using std::string;
using std::tuple;
using std::vector;
using utl::RSZ;

using sta::delayAsFloat;
using sta::MinMax;
using sta::Pin;
using sta::Slack;
using sta::Vertex;

MoveTracker::MoveTracker(utl::Logger* logger, sta::Sta* sta)
    : logger_(logger),
      sta_(sta),
      current_endpoint_(nullptr),
      move_count_(0),
      total_move_count_(0),
      total_no_attempt_count_(0),
      total_attempt_count_(0),
      total_reject_count_(0),
      total_commit_count_(0)
{
}

void MoveTracker::setCurrentEndpoint(const sta::Pin* endpoint_pin)
{
  current_endpoint_ = endpoint_pin;

  // If this is a new endpoint not yet in our tracking map, initialize it
  // with current slack for all three values (will be updated later)
  if (endpoint_pin
      && endpoint_slack_.find(endpoint_pin) == endpoint_slack_.end()) {
    Vertex* vertex = sta_->graph()->pinLoadVertex(endpoint_pin);
    if (vertex) {
      Slack current_slack = sta_->vertexSlack(vertex, MinMax::max());
      // Store (original_slack, pre_phase_slack, post_phase_slack)
      // Initialize all to current_slack (will be properly set by capture
      // methods)
      endpoint_slack_[endpoint_pin]
          = std::make_tuple(current_slack, current_slack, current_slack);
    }
  }
}

void MoveTracker::trackCriticalPins(const vector<const Pin*>& critical_pins)
{
  // Don't use the provided list - instead iterate over all instances and output
  // pins
  all_critical_pins_.clear();

  // Iterate through all instances in the design
  sta::LeafInstanceIterator* inst_iter
      = sta_->network()->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();

    // Check all output pins of this instance
    sta::InstancePinIterator* pin_iter = sta_->network()->pinIterator(inst);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();

      // Only track output (driver) pins
      if (sta_->network()->isDriver(pin)) {
        // Skip pins on clock networks
        Vertex* vertex = sta_->graph()->pinDrvrVertex(pin);
        if (vertex && sta_->search()->isClock(vertex)) {
          continue;
        }

        // Get setup slack for this pin
        Slack slack = sta_->pinSlack(pin, MinMax::max());
        float slack_ps = delayAsFloat(slack) * 1e12;

        // Only track pins with negative slack
        if (slack_ps < 0.0) {
          all_critical_pins_.insert(pin);
        }
      }
    }
    delete pin_iter;
  }
  delete inst_iter;
}

void MoveTracker::clear()
{
  move_count_ = 0;
  visit_count_.clear();
  moves_.clear();
  pending_moves_.clear();
  current_endpoint_ = nullptr;
}

void MoveTracker::clearMoveSummary()
{
  move_count_ = 0;
  moves_.clear();
  visit_count_.clear();
}

void MoveTracker::trackViolator(const sta::Pin* pin)
{
  if (visit_count_.find(pin) == visit_count_.end()) {
    visit_count_[pin] = 0;
  }
  visit_count_.at(pin)++;

  // Track all visited pins for missed opportunities report
  all_visited_pins_.insert(pin);

  // Track which endpoint this pin was visited on (basic info only)
  if (current_endpoint_ && pin_info_.find(pin) == pin_info_.end()) {
    pin_info_[pin] = PinInfo(current_endpoint_, "unknown", 0.0, 0.0, 0.0, 0.0);
  }
}

void MoveTracker::trackViolatorWithInfo(const sta::Pin* pin,
                                        const string& gate_type,
                                        float load_delay,
                                        float intrinsic_delay,
                                        float pin_slack,
                                        float endpoint_slack)
{
  // First track as regular violator
  if (visit_count_.find(pin) == visit_count_.end()) {
    visit_count_[pin] = 0;
  }
  visit_count_.at(pin)++;
  all_visited_pins_.insert(pin);

  // Store detailed information
  if (current_endpoint_) {
    pin_info_[pin] = PinInfo(current_endpoint_,
                             gate_type,
                             load_delay,
                             intrinsic_delay,
                             pin_slack,
                             endpoint_slack);
  }
}

void MoveTracker::trackMove(const sta::Pin* pin,
                            const string& move_type,
                            MoveStateType state)
{
  assert(visit_count_.find(pin) != visit_count_.end()
         && "Pin must be visited before tracking moves.");
  pending_moves_.emplace_back(pin, move_type, state);
}

void MoveTracker::commitMoves()
{
  for (const auto& pending_move : pending_moves_) {
    moves_.emplace_back(pending_move.pin,
                        move_count_++,
                        pending_move.move_type,
                        MoveStateType::ATTEMPT_COMMIT);

    // Store in move history for detailed reports
    pin_move_history_[pending_move.pin].emplace_back(
        pending_move.move_type, MoveStateType::ATTEMPT_COMMIT);
  }

  // Track per-endpoint statistics
  if (current_endpoint_ && !pending_moves_.empty()) {
    auto& counts = endpoint_move_counts_[current_endpoint_];
    std::get<0>(counts) += pending_moves_.size();  // attempts
    std::get<2>(counts) += pending_moves_.size();  // commits
  }

  pending_moves_.clear();
}

void MoveTracker::rejectMoves()
{
  for (const auto& pending_move : pending_moves_) {
    moves_.emplace_back(pending_move.pin,
                        move_count_++,
                        pending_move.move_type,
                        MoveStateType::ATTEMPT_REJECT);

    // Store in move history for detailed reports
    pin_move_history_[pending_move.pin].emplace_back(
        pending_move.move_type, MoveStateType::ATTEMPT_REJECT);
  }

  // Track per-endpoint statistics
  if (current_endpoint_ && !pending_moves_.empty()) {
    auto& counts = endpoint_move_counts_[current_endpoint_];
    std::get<0>(counts) += pending_moves_.size();  // attempts
    std::get<1>(counts) += pending_moves_.size();  // rejects
  }

  pending_moves_.clear();
}

int MoveTracker::getVisitCount(const sta::Pin* pin) const
{
  auto it = visit_count_.find(pin);
  return (it != visit_count_.end()) ? it->second : 0;
}

void MoveTracker::printMoveSummary(const std::string& title)
{
  if (moves_.size() == 0) {
    return;
  }

  // attempt, reject, commit
  map<string, tuple<int, int, int>> move_type_counts;
  // We tried a move
  vector<const Pin*> attempted_pins;
  // We tried a move and doMove succeeded, but it was rejected
  vector<const Pin*> rejected_pins;
  // We tried a move and it was committed
  vector<const Pin*> committed_pins;

  for (const auto& move : moves_) {
    const Pin* pin = move.pin;

    switch (move.state) {
      case MoveStateType::ATTEMPT:
        attempted_pins.push_back(pin);
        std::get<(int) MoveStateType::ATTEMPT>(
            move_type_counts[move.move_type])++;
        std::get<(int) MoveStateType::ATTEMPT>(
            total_move_type_counts_[move.move_type])++;
        break;
      case MoveStateType::ATTEMPT_REJECT:
        attempted_pins.push_back(pin);
        std::get<(int) MoveStateType::ATTEMPT>(
            move_type_counts[move.move_type])++;
        std::get<(int) MoveStateType::ATTEMPT>(
            total_move_type_counts_[move.move_type])++;
        rejected_pins.push_back(pin);
        std::get<(int) MoveStateType::ATTEMPT_REJECT>(
            move_type_counts[move.move_type])++;
        std::get<(int) MoveStateType::ATTEMPT_REJECT>(
            total_move_type_counts_[move.move_type])++;
        break;
      case MoveStateType::ATTEMPT_COMMIT:
        attempted_pins.push_back(pin);
        std::get<(int) MoveStateType::ATTEMPT>(
            move_type_counts[move.move_type])++;
        std::get<(int) MoveStateType::ATTEMPT>(
            total_move_type_counts_[move.move_type])++;
        committed_pins.push_back(pin);
        std::get<(int) MoveStateType::ATTEMPT_COMMIT>(
            move_type_counts[move.move_type])++;
        std::get<(int) MoveStateType::ATTEMPT_COMMIT>(
            total_move_type_counts_[move.move_type])++;
        break;
    }
  }

  int no_attempt_count = 0;
  for (const auto& [pin, visit_count] : visit_count_) {
    if (std::find(attempted_pins.begin(), attempted_pins.end(), pin)
        != attempted_pins.end()) {
      continue;
    }
    no_attempt_count++;
  }

  debugPrint(logger_, RSZ, "move_tracker", 2, "{}:", title);
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Current Summary: Not Attempted: {} Attempts: {} Rejects: "
             "{} Commits: {} ",
             no_attempt_count,
             attempted_pins.size(),
             rejected_pins.size(),
             committed_pins.size());
  float attempt_rate_
      = (moves_.size() > 0)
            ? (static_cast<float>(attempted_pins.size()) / moves_.size()) * 100
            : 0;
  float reject_rate_
      = (moves_.size() > 0)
            ? (static_cast<float>(rejected_pins.size()) / moves_.size()) * 100
            : 0;
  float commit_rate_
      = (moves_.size() > 0)
            ? (static_cast<float>(committed_pins.size()) / moves_.size()) * 100
            : 0;
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Overall attempt_rate: {:.2f}% ({}) reject_rate: {:0.2f}% "
             "({}) commit_rate: {:0.2f}% ({})",
             attempt_rate_,
             attempted_pins.size(),
             reject_rate_,
             rejected_pins.size(),
             commit_rate_,
             committed_pins.size());

  total_no_attempt_count_ += no_attempt_count;
  total_attempt_count_ += attempted_pins.size();
  total_reject_count_ += rejected_pins.size();
  total_commit_count_ += committed_pins.size();
  total_move_count_ += move_count_;

  for (const auto& [move_type, counts] : move_type_counts) {
    float move_attempt_count
        = static_cast<float>(std::get<(int) MoveStateType::ATTEMPT>(counts));
    float move_reject_count = static_cast<float>(
        std::get<(int) MoveStateType::ATTEMPT_REJECT>(counts));
    float move_commit_count = static_cast<float>(
        std::get<(int) MoveStateType::ATTEMPT_COMMIT>(counts));
    float move_attempt_rate
        = (moves_.size() > 0) ? (move_attempt_count / moves_.size()) * 100 : 0;
    float move_reject_rate
        = (moves_.size() > 0) ? (move_reject_count / moves_.size()) * 100 : 0;
    float move_commit_rate
        = (moves_.size() > 0) ? (move_commit_count / moves_.size()) * 100 : 0;
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{} attempt_rate: {:.2f}% ({}) reject_rate: {:.2f}% ({})  "
               "commit_rate: {:.2f}% ({})",
               move_type,
               move_attempt_rate,
               move_attempt_count,
               move_reject_rate,
               move_reject_count,
               move_commit_rate,
               move_commit_count);
  }

  debugPrint(logger_, RSZ, "move_tracker", 2, "Total statistics:");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Total Summary: Not Attempted: {} Attempts: {} Rejects: "
             "{} Commits: {} ",
             total_no_attempt_count_,
             total_attempt_count_,
             total_reject_count_,
             total_commit_count_);
  float total_attempt_rate_
      = (total_move_count_ > 0)
            ? (static_cast<float>(total_attempt_count_) / total_move_count_)
                  * 100
            : 0;
  float total_reject_rate_
      = (total_move_count_ > 0)
            ? (static_cast<float>(total_reject_count_) / total_move_count_)
                  * 100
            : 0;
  float total_commit_rate_
      = (total_move_count_ > 0)
            ? (static_cast<float>(total_commit_count_) / total_move_count_)
                  * 100
            : 0;
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Overall attempt_rate: {:.2f}% ({}) reject_rate: {:0.2f}% "
             "({}) commit_rate: {:0.2f}% ({})",
             total_attempt_rate_,
             total_attempt_count_,
             total_reject_rate_,
             total_reject_count_,
             total_commit_rate_,
             total_commit_count_);

  for (const auto& [move_type, counts] : total_move_type_counts_) {
    float total_move_attempt_count
        = static_cast<float>(std::get<(int) MoveStateType::ATTEMPT>(counts));
    float total_move_reject_count = static_cast<float>(
        std::get<(int) MoveStateType::ATTEMPT_REJECT>(counts));
    float total_move_commit_count = static_cast<float>(
        std::get<(int) MoveStateType::ATTEMPT_COMMIT>(counts));

    float total_move_attempt_rate
        = (total_move_count_ > 0)
              ? (total_move_attempt_count / total_move_count_) * 100
              : 0;
    float total_move_reject_rate
        = (total_move_count_ > 0)
              ? (total_move_reject_count / total_move_count_) * 100
              : 0;
    float total_move_commit_rate
        = (total_move_count_ > 0)
              ? (total_move_commit_count / total_move_count_) * 100
              : 0;
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{} attempt_rate: {:.2f}% ({}) reject_rate: {:.2f}% ({})  "
               "commit_rate: {:.2f}% ({})",
               move_type,
               total_move_attempt_rate,
               total_move_attempt_count,
               total_move_reject_rate,
               total_move_reject_count,
               total_move_commit_rate,
               total_move_commit_count);
  }
  clearMoveSummary();
}

void MoveTracker::printEndpointSummary(const std::string& title)
{
  if (endpoint_move_counts_.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{}: No endpoint statistics collected",
               title);
    return;
  }

  // Update post-phase slack for all endpoints
  for (auto& [endpoint_pin, slack_tuple] : endpoint_slack_) {
    Vertex* vertex = sta_->graph()->pinLoadVertex(endpoint_pin);
    if (vertex) {
      Slack post_phase_slack = sta_->vertexSlack(vertex, MinMax::max());
      std::get<2>(slack_tuple)
          = post_phase_slack;  // Update post-phase slack (3rd element)
    }
  }

  // Determine phase from title
  bool is_tns_phase = title.find("TNS") != std::string::npos;

  // Convert to vector for sorting
  vector<std::pair<const Pin*, tuple<int, int, int>>> endpoint_stats(
      endpoint_move_counts_.begin(), endpoint_move_counts_.end());

  // Calculate total attempts, rejects, commits across ALL endpoints for
  // percentages
  int total_attempts_all = 0;
  int total_rejects_all = 0;
  int total_commits_all = 0;
  for (const auto& [endpoint_pin, counts] : endpoint_stats) {
    total_attempts_all += std::get<0>(counts);
    total_rejects_all += std::get<1>(counts);
    total_commits_all += std::get<2>(counts);
  }

  // Sort by post-phase slack (most negative first) to show WNS endpoints
  std::sort(endpoint_stats.begin(),
            endpoint_stats.end(),
            [this](const auto& a, const auto& b) {
              auto slack_it_a = endpoint_slack_.find(a.first);
              auto slack_it_b = endpoint_slack_.find(b.first);
              if (slack_it_a == endpoint_slack_.end()) {
                return false;
              }
              if (slack_it_b == endpoint_slack_.end()) {
                return true;
              }
              // Sort by post-phase slack (most negative first)
              Slack slack_a = std::get<2>(slack_it_a->second);
              Slack slack_b = std::get<2>(slack_it_b->second);
              return slack_a < slack_b;
            });

  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Per-Endpoint Optimization Effort (sorted by WNS):");

  // Print header
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<40} | {:>13} | {:>13} | {:>13} | {:>6} | {:>11} | {:>11} | "
             "{:>11} | {:>11}",
             "Endpoint",
             "Attempts",
             "Rejects",
             "Commits",
             "Commit%",
             "OrigSlk(ns)",
             "PreSlk(ns)",
             "PostSlk(ns)",
             "Delta(ns)");

  // Print top endpoints: WNS phase shows top 20, TNS phase shows up to 1000
  int max_endpoints_to_print = is_tns_phase ? 1000 : 20;
  int count = 0;
  int total_attempts_shown = 0;
  int total_rejects_shown = 0;
  int total_commits_shown = 0;
  Slack worst_original_slack = sta::INF;
  Slack worst_post_endpoint_slack = sta::INF;
  Slack worst_final_slack = sta::INF;
  Slack tns_original = 0.0;
  Slack tns_post_endpoint = 0.0;
  Slack tns_final = 0.0;

  for (const auto& [endpoint_pin, counts] : endpoint_stats) {
    if (count >= max_endpoints_to_print) {
      break;
    }

    int attempts = std::get<0>(counts);
    int rejects = std::get<1>(counts);
    int commits = std::get<2>(counts);
    float commit_rate
        = (attempts > 0) ? (static_cast<float>(commits) / attempts) * 100 : 0;

    // Calculate percentages of total
    float attempt_pct
        = (total_attempts_all > 0)
              ? (static_cast<float>(attempts) / total_attempts_all) * 100
              : 0;
    float reject_pct
        = (total_rejects_all > 0)
              ? (static_cast<float>(rejects) / total_rejects_all) * 100
              : 0;
    float commit_pct
        = (total_commits_all > 0)
              ? (static_cast<float>(commits) / total_commits_all) * 100
              : 0;

    total_attempts_shown += attempts;
    total_rejects_shown += rejects;
    total_commits_shown += commits;

    // Get slack information
    Slack original_slack = 0.0;
    Slack pre_phase_slack = 0.0;
    Slack post_phase_slack = 0.0;
    auto slack_it = endpoint_slack_.find(endpoint_pin);
    if (slack_it != endpoint_slack_.end()) {
      original_slack = std::get<0>(slack_it->second);
      pre_phase_slack = std::get<1>(slack_it->second);
      post_phase_slack = std::get<2>(slack_it->second);

      // Track worst slack
      worst_original_slack = std::min(original_slack, worst_original_slack);
      worst_post_endpoint_slack
          = std::min(pre_phase_slack, worst_post_endpoint_slack);
      worst_final_slack = std::min(post_phase_slack, worst_final_slack);

      // Accumulate TNS (sum of negative slacks)
      if (original_slack < 0.0) {
        tns_original += original_slack;
      }
      if (pre_phase_slack < 0.0) {
        tns_post_endpoint += pre_phase_slack;
      }
      if (post_phase_slack < 0.0) {
        tns_final += post_phase_slack;
      }
    }

    // Convert to ns (seconds to nanoseconds)
    float original_slack_ns = delayAsFloat(original_slack) * 1e9;
    float pre_phase_slack_ns = delayAsFloat(pre_phase_slack) * 1e9;
    float post_phase_slack_ns = delayAsFloat(post_phase_slack) * 1e9;

    // Calculate phase delta (improvement is positive)
    float delta_ns = post_phase_slack_ns - pre_phase_slack_ns;

    string endpoint_name = sta_->network()->pathName(endpoint_pin);
    // Truncate long names
    if (endpoint_name.length() > 40) {
      endpoint_name = endpoint_name.substr(0, 37) + "...";
    }

    // Format attempts, rejects, commits with percentages (no divider)
    std::ostringstream attempts_str, rejects_str, commits_str;
    attempts_str << std::right << std::setw(6) << attempts << " "
                 << std::setw(5) << std::fixed << std::setprecision(1)
                 << attempt_pct << "%";
    rejects_str << std::right << std::setw(6) << rejects << " " << std::setw(5)
                << std::fixed << std::setprecision(1) << reject_pct << "%";
    commits_str << std::right << std::setw(6) << commits << " " << std::setw(5)
                << std::fixed << std::setprecision(1) << commit_pct << "%";

    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<40} | {:>13} | {:>13} | {:>13} | {:>5.1f}% | {:>11.3f} | "
               "{:>11.3f} | {:>11.3f} | {:>11.3f}",
               endpoint_name,
               attempts_str.str(),
               rejects_str.str(),
               commits_str.str(),
               commit_rate,
               original_slack_ns,
               pre_phase_slack_ns,
               post_phase_slack_ns,
               delta_ns);

    count++;
  }

  // Print summary line
  if (endpoint_stats.size() > max_endpoints_to_print) {
    int remaining_endpoints = endpoint_stats.size() - max_endpoints_to_print;
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "... ({} more endpoints not shown)",
               remaining_endpoints);
  }

  float total_commit_rate
      = (total_attempts_shown > 0)
            ? (static_cast<float>(total_commits_shown) / total_attempts_shown)
                  * 100
            : 0;

  // Calculate percentages for total shown
  float total_attempt_pct
      = (total_attempts_all > 0)
            ? (static_cast<float>(total_attempts_shown) / total_attempts_all)
                  * 100
            : 0;
  float total_reject_pct
      = (total_rejects_all > 0)
            ? (static_cast<float>(total_rejects_shown) / total_rejects_all)
                  * 100
            : 0;
  float total_commit_pct
      = (total_commits_all > 0)
            ? (static_cast<float>(total_commits_shown) / total_commits_all)
                  * 100
            : 0;

  // Convert to ns for WNS/TNS values
  float wns_original_ns = delayAsFloat(worst_original_slack) * 1e9;
  float wns_post_endpoint_ns = delayAsFloat(worst_post_endpoint_slack) * 1e9;
  float wns_final_ns = delayAsFloat(worst_final_slack) * 1e9;
  float tns_original_ns = delayAsFloat(tns_original) * 1e9;
  float tns_post_endpoint_ns = delayAsFloat(tns_post_endpoint) * 1e9;
  float tns_final_ns = delayAsFloat(tns_final) * 1e9;

  // Format total attempts, rejects, commits with percentages (no divider)
  std::ostringstream total_attempts_str, total_rejects_str, total_commits_str;
  total_attempts_str << std::right << std::setw(6) << total_attempts_shown
                     << " " << std::setw(5) << std::fixed
                     << std::setprecision(1) << total_attempt_pct << "%";
  total_rejects_str << std::right << std::setw(6) << total_rejects_shown << " "
                    << std::setw(5) << std::fixed << std::setprecision(1)
                    << total_reject_pct << "%";
  total_commits_str << std::right << std::setw(6) << total_commits_shown << " "
                    << std::setw(5) << std::fixed << std::setprecision(1)
                    << total_commit_pct << "%";

  // Print Total (shown) line with move counts only
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<40} | {:>13} | {:>13} | {:>13} | {:>5.1f}% | {:>11} | "
             "{:>11} | {:>11} | {:>11}",
             "Total (shown)",
             total_attempts_str.str(),
             total_rejects_str.str(),
             total_commits_str.str(),
             total_commit_rate,
             "",
             "",
             "",
             "");

  // Print Maximum (WNS) summary line
  float wns_delta_ns = wns_final_ns - wns_post_endpoint_ns;
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<40} | {:>13} | {:>13} | {:>13} | {:>6} | {:>11.3f} | "
             "{:>11.3f} | {:>11.3f} | {:>11.3f}",
             "Maximum (WNS)",
             "",
             "",
             "",
             "",
             wns_original_ns,
             wns_post_endpoint_ns,
             wns_final_ns,
             wns_delta_ns);

  // Print Total (TNS) summary line
  float tns_delta_ns = tns_final_ns - tns_post_endpoint_ns;
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<40} | {:>13} | {:>13} | {:>13} | {:>6} | {:>11.3f} | "
             "{:>11.3f} | {:>11.3f} | {:>11.3f}",
             "Total (TNS)",
             "",
             "",
             "",
             "",
             tns_original_ns,
             tns_post_endpoint_ns,
             tns_final_ns,
             tns_delta_ns);

  // Count endpoints by effort
  int low_effort = 0;   // 1-10 attempts
  int med_effort = 0;   // 11-50 attempts
  int high_effort = 0;  // 51+ attempts

  for (const auto& [endpoint_pin, counts] : endpoint_stats) {
    int attempts = std::get<0>(counts);
    if (attempts <= 10) {
      low_effort++;
    } else if (attempts <= 50) {
      med_effort++;
    } else {
      high_effort++;
    }
  }

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Endpoint effort distribution: {} low (1-10), {} medium (11-50), "
             "{} high (51+)",
             low_effort,
             med_effort,
             high_effort);
}

void MoveTracker::printSuccessReport(const std::string& title)
{
  // Collect pins with committed moves
  map<const Pin*, vector<string>> successful_pins;
  for (const auto& [pin, history] : pin_move_history_) {
    for (const auto& [move_type, state] : history) {
      if (state == MoveStateType::ATTEMPT_COMMIT) {
        successful_pins[pin].push_back(move_type);
      }
    }
  }

  if (successful_pins.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{}: No successful optimizations",
               title);
    return;
  }

  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Successfully optimized {} pins with committed moves",
             successful_pins.size());

  // Count move types
  map<string, int> move_type_success_count;
  for (const auto& [pin, moves] : successful_pins) {
    for (const auto& move_type : moves) {
      move_type_success_count[move_type]++;
    }
  }

  // Sort by count (descending)
  vector<pair<string, int>> sorted_success_types(
      move_type_success_count.begin(), move_type_success_count.end());
  std::sort(sorted_success_types.begin(),
            sorted_success_types.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  debugPrint(logger_, RSZ, "move_tracker", 1, "Successful moves by type:");
  for (const auto& [move_type, count] : sorted_success_types) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  {:<20}: {:>5} commits",
               move_type,
               count);
  }

  // Show top successful pins (most commits)
  vector<pair<const Pin*, int>> pin_commit_counts;
  for (const auto& [pin, moves] : successful_pins) {
    pin_commit_counts.emplace_back(pin, moves.size());
  }
  std::sort(pin_commit_counts.begin(),
            pin_commit_counts.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  constexpr int max_pins_to_show = 20;
  constexpr int max_move_columns = 6;

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Top {} pins by successful moves:",
             std::min(max_pins_to_show, (int) pin_commit_counts.size()));

  int shown = 0;
  for (const auto& [pin, count] : pin_commit_counts) {
    if (shown >= max_pins_to_show) {
      break;
    }
    string pin_name = sta_->network()->pathName(pin);
    const auto& moves = successful_pins[pin];

    // Count move types for this pin and sort by frequency
    map<string, int> move_counts;
    for (const auto& move : moves) {
      move_counts[move]++;
    }

    // Sort moves by count (descending)
    vector<pair<string, int>> sorted_moves(move_counts.begin(),
                                           move_counts.end());
    std::sort(sorted_moves.begin(),
              sorted_moves.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Truncate long pin names
    if (pin_name.length() > 34) {
      pin_name = pin_name.substr(0, 31) + "...";
    }

    // Build output line with aligned columns
    std::ostringstream oss;
    oss << "  " << std::left << std::setw(34) << pin_name << " | " << std::right
        << std::setw(5) << count;

    // Add top moves for this pin in separate columns
    int col = 0;
    for (const auto& [move_type, move_count] : sorted_moves) {
      if (col >= max_move_columns) {
        break;
      }
      // Split move type and count into two parts of the column
      std::ostringstream move_oss;
      move_oss << " | " << std::left << std::setw(14) << move_type << std::right
               << std::setw(4) << move_count;
      oss << move_oss.str();
      col++;
    }

    // Fill remaining columns with empty space
    while (col < max_move_columns) {
      oss << " | " << std::setw(18) << "";
      col++;
    }

    debugPrint(logger_, RSZ, "move_tracker", 1, "{}", oss.str());
    shown++;
  }

  if (pin_commit_counts.size() > max_pins_to_show) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  ... ({} more pins not shown)",
               pin_commit_counts.size() - max_pins_to_show);
  }
}

void MoveTracker::printFailureReport(const std::string& title)
{
  // Collect pins with rejected moves
  map<const Pin*, vector<string>> failed_pins;
  for (const auto& [pin, history] : pin_move_history_) {
    for (const auto& [move_type, state] : history) {
      if (state == MoveStateType::ATTEMPT_REJECT) {
        failed_pins[pin].push_back(move_type);
      }
    }
  }

  if (failed_pins.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{}: No rejected optimizations",
               title);
    return;
  }

  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{} pins had rejected moves (timing did not improve)",
             failed_pins.size());

  // Count move types
  map<string, int> move_type_reject_count;
  for (const auto& [pin, moves] : failed_pins) {
    for (const auto& move_type : moves) {
      move_type_reject_count[move_type]++;
    }
  }

  // Sort by count (descending)
  vector<pair<string, int>> sorted_reject_types(move_type_reject_count.begin(),
                                                move_type_reject_count.end());
  std::sort(sorted_reject_types.begin(),
            sorted_reject_types.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  debugPrint(logger_, RSZ, "move_tracker", 1, "Rejected moves by type:");
  for (const auto& [move_type, count] : sorted_reject_types) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  {:<20}: {:>5} rejects",
               move_type,
               count);
  }

  // Show top failed pins (most rejects)
  vector<pair<const Pin*, int>> pin_reject_counts;
  for (const auto& [pin, moves] : failed_pins) {
    pin_reject_counts.emplace_back(pin, moves.size());
  }
  std::sort(pin_reject_counts.begin(),
            pin_reject_counts.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  constexpr int max_pins_to_show = 20;
  constexpr int max_move_columns = 6;

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Top {} pins by rejected moves:",
             std::min(max_pins_to_show, (int) pin_reject_counts.size()));

  int shown = 0;
  for (const auto& [pin, count] : pin_reject_counts) {
    if (shown >= max_pins_to_show) {
      break;
    }
    string pin_name = sta_->network()->pathName(pin);
    const auto& moves = failed_pins[pin];

    // Count move types for this pin and sort by frequency
    map<string, int> move_counts;
    for (const auto& move : moves) {
      move_counts[move]++;
    }

    // Sort moves by count (descending)
    vector<pair<string, int>> sorted_moves(move_counts.begin(),
                                           move_counts.end());
    std::sort(sorted_moves.begin(),
              sorted_moves.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Truncate long pin names
    if (pin_name.length() > 34) {
      pin_name = pin_name.substr(0, 31) + "...";
    }

    // Build output line with aligned columns
    std::ostringstream oss;
    oss << "  " << std::left << std::setw(34) << pin_name << " | " << std::right
        << std::setw(5) << count;

    // Add top moves for this pin in separate columns
    int col = 0;
    for (const auto& [move_type, move_count] : sorted_moves) {
      if (col >= max_move_columns) {
        break;
      }
      // Split move type and count into two parts of the column
      std::ostringstream move_oss;
      move_oss << " | " << std::left << std::setw(14) << move_type << std::right
               << std::setw(4) << move_count;
      oss << move_oss.str();
      col++;
    }

    // Fill remaining columns with empty space
    while (col < max_move_columns) {
      oss << " | " << std::setw(18) << "";
      col++;
    }

    debugPrint(logger_, RSZ, "move_tracker", 1, "{}", oss.str());
    shown++;
  }

  if (pin_reject_counts.size() > max_pins_to_show) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  ... ({} more pins not shown)",
               pin_reject_counts.size() - max_pins_to_show);
  }
}

void MoveTracker::printMissedOpportunitiesReport(const std::string& title)
{
  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);

  // Category 1: Pins visited but no moves attempted
  // Only include pins with negative pin slack or negative endpoint slack
  // Use a small threshold to avoid floating point near-zero values
  constexpr float slack_threshold = -0.001;  // -1 fs threshold
  vector<const Pin*> visited_no_attempt;
  for (const Pin* pin : all_visited_pins_) {
    // Check if this pin has any moves in history
    if (pin_move_history_.find(pin) == pin_move_history_.end()
        || pin_move_history_[pin].empty()) {
      // Only include if pin has meaningful negative slack
      auto it = pin_info_.find(pin);
      if (it != pin_info_.end()) {
        const PinInfo& info = it->second;
        // Check for meaningful negative slack
        bool has_negative_slack = (info.pin_slack < slack_threshold
                                   || info.endpoint_slack < slack_threshold);
        if (has_negative_slack) {
          visited_no_attempt.push_back(pin);
        }
      }
    }
  }

  if (!visited_no_attempt.empty()) {
    // Sort by endpoint slack (most negative first), then by pin slack
    std::sort(visited_no_attempt.begin(),
              visited_no_attempt.end(),
              [this](const Pin* a, const Pin* b) {
                auto it_a = pin_info_.find(a);
                auto it_b = pin_info_.find(b);
                if (it_a == pin_info_.end() || it_b == pin_info_.end()) {
                  return false;
                }
                const PinInfo& info_a = it_a->second;
                const PinInfo& info_b = it_b->second;
                // Sort by endpoint slack first (most negative = most critical)
                if (info_a.endpoint_slack != info_b.endpoint_slack) {
                  return info_a.endpoint_slack < info_b.endpoint_slack;
                }
                // Then by pin slack (most negative first)
                return info_a.pin_slack < info_b.pin_slack;
              });

    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Category 1: {} pins with negative slack visited but NO moves "
               "attempted",
               visited_no_attempt.size());
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  (These pins have timing violations but no optimization "
               "moves were tried)");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  (Sorted by most critical endpoint first)");

    // Print table header
    debugPrint(
        logger_,
        RSZ,
        "move_tracker",
        1,
        "  {:<38} | {:<30} | {:<26} | {:>10} | {:>10} | {:>9} | {:>9} | {:>6}",
        "Pin",
        "Gate Type",
        "Endpoint",
        "PinSlk(ps)",
        "EndSlk(ps)",
        "Load(ps)",
        "Intr(ps)",
        "Fanout");

    constexpr int max_pins_to_show = 20;
    int shown = 0;
    for (const Pin* pin : visited_no_attempt) {
      if (shown >= max_pins_to_show) {
        break;
      }
      string pin_name = sta_->network()->pathName(pin);
      if (pin_name.length() > 38) {
        pin_name = pin_name.substr(0, 35) + "...";
      }

      // Get detailed info if available
      string gate_type = "unknown";
      string endpoint_name = "unknown";
      float load_delay = 0.0;
      float intrinsic_delay = 0.0;
      float pin_slack = 0.0;
      float endpoint_slack = 0.0;
      int fanout = 0;

      auto it = pin_info_.find(pin);
      if (it != pin_info_.end()) {
        const PinInfo& info = it->second;
        gate_type = info.gate_type;
        if (gate_type.length() > 30) {
          gate_type = gate_type.substr(0, 27) + "...";
        }
        if (info.endpoint) {
          endpoint_name = sta_->network()->pathName(info.endpoint);
          if (endpoint_name.length() > 26) {
            endpoint_name = endpoint_name.substr(0, 23) + "...";
          }
        }
        load_delay = info.load_delay;
        intrinsic_delay = info.intrinsic_delay;
        pin_slack = info.pin_slack;
        endpoint_slack = info.endpoint_slack;
      }

      // Calculate fanout (count wire edges only)
      Vertex* vertex = sta_->graph()->pinDrvrVertex(pin);
      if (vertex) {
        sta::VertexOutEdgeIterator edge_iter(vertex, sta_->graph());
        while (edge_iter.hasNext()) {
          sta::Edge* edge = edge_iter.next();
          if (edge->isWire()) {
            fanout++;
          }
        }
      }

      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "  {:<38} | {:<30} | {:<26} | {:>10.2f} | {:>10.2f} | "
                 "{:>9.2f} | {:>9.2f} | {:>6}",
                 pin_name,
                 gate_type,
                 endpoint_name,
                 pin_slack * 1e12,        // Convert seconds to ps
                 endpoint_slack * 1e12,   // Convert seconds to ps
                 load_delay * 1e12,       // Convert seconds to ps
                 intrinsic_delay * 1e12,  // Convert seconds to ps
                 fanout);
      shown++;
    }

    if (visited_no_attempt.size() > max_pins_to_show) {
      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "  ... ({} more pins not shown)",
                 visited_no_attempt.size() - max_pins_to_show);
    }
  } else {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Category 1: All visited pins with negative slack had moves "
               "attempted (good!)");
  }

  // Category 2: Critical pins never visited
  vector<const Pin*> critical_never_visited;
  for (const Pin* pin : all_critical_pins_) {
    if (all_visited_pins_.find(pin) == all_visited_pins_.end()) {
      critical_never_visited.push_back(pin);
    }
  }

  // Sort by most negative slack
  std::sort(critical_never_visited.begin(),
            critical_never_visited.end(),
            [this](const Pin* a, const Pin* b) {
              Slack slack_a = sta_->pinSlack(a, MinMax::max());
              Slack slack_b = sta_->pinSlack(b, MinMax::max());
              return slack_a < slack_b;  // Most negative first
            });

  if (!critical_never_visited.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Category 2: {} critical pins NEVER visited",
               critical_never_visited.size());
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  (These pins are on critical paths but were never "
               "considered for optimization)");

    // Print table header
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "  {:<38} | {:<30} | {:>10} | {:>9} | {:>9} | {:>6}",
               "Pin",
               "Gate Type",
               "PinSlk(ps)",
               "Load(ps)",
               "Intr(ps)",
               "Fanout");

    constexpr int max_pins_to_show = 20;
    int shown = 0;
    for (const Pin* pin : critical_never_visited) {
      if (shown >= max_pins_to_show) {
        break;
      }
      string pin_name = sta_->network()->pathName(pin);
      if (pin_name.length() > 38) {
        pin_name = pin_name.substr(0, 35) + "...";
      }

      // Get gate type
      string gate_type = "unknown";
      sta::Instance* inst = sta_->network()->instance(pin);
      if (inst) {
        sta::LibertyCell* cell = sta_->network()->libertyCell(inst);
        if (cell) {
          gate_type = cell->name();
          if (gate_type.length() > 30) {
            gate_type = gate_type.substr(0, 27) + "...";
          }
        }
      }

      // Get pin slack
      Slack pin_slack = sta_->pinSlack(pin, MinMax::max());
      float pin_slack_ps = delayAsFloat(pin_slack) * 1e12;

      // Calculate effort delays (similar to ViolatorCollector::getEffortDelays)
      float load_delay_ps = 0.0;
      float intrinsic_delay_ps = 0.0;
      Vertex* vertex = sta_->graph()->pinDrvrVertex(pin);
      if (vertex) {
        sta::Delay selected_load_delay = -sta::INF;
        sta::Delay selected_intrinsic_delay = -sta::INF;
        Slack worst_slack = sta::INF;

        // Find the arc with worst slack
        sta::VertexInEdgeIterator in_edge_iter(vertex, sta_->graph());
        while (in_edge_iter.hasNext()) {
          sta::Edge* prev_edge = in_edge_iter.next();
          Vertex* from_vertex = prev_edge->from(sta_->graph());

          // Ignore output-to-output timing arcs
          const Pin* from_pin = from_vertex->pin();
          if (!from_pin || sta_->network()->direction(from_pin)->isOutput()) {
            continue;
          }

          const sta::TimingArcSet* arc_set = prev_edge->timingArcSet();
          for (const sta::RiseFall* rf : sta::RiseFall::range()) {
            sta::TimingArc* prev_arc = arc_set->arcTo(rf);
            if (!prev_arc) {
              continue;
            }

            const sta::Transition* from_trans = prev_arc->fromEdge();
            const sta::RiseFall* from_rf = from_trans->asRiseFall();
            Slack from_slack
                = sta_->vertexSlack(from_vertex, from_rf, MinMax::max());

            sta::Corner* corner = sta_->cmdCorner();
            sta::DcalcAnalysisPt* dcalc_ap
                = corner->findDcalcAnalysisPt(MinMax::max());
            int lib_ap = dcalc_ap->libertyIndex();
            const sta::TimingArc* corner_arc = prev_arc->cornerArc(lib_ap);
            const sta::Delay intrinsic_delay = corner_arc->intrinsicDelay();
            const sta::Delay delay = sta_->graph()->arcDelay(
                prev_edge, prev_arc, dcalc_ap->index());
            const sta::Delay load_delay = delay - intrinsic_delay;

            // Select delays from arc with most negative slack
            if (from_slack < worst_slack
                || (from_slack == worst_slack
                    && load_delay > selected_load_delay)) {
              worst_slack = from_slack;
              selected_load_delay = load_delay;
              selected_intrinsic_delay = intrinsic_delay;
            }
          }
        }

        if (selected_load_delay != -sta::INF) {
          load_delay_ps = delayAsFloat(selected_load_delay) * 1e12;
          intrinsic_delay_ps = delayAsFloat(selected_intrinsic_delay) * 1e12;
        }
      }

      // Calculate fanout (count wire edges only)
      int fanout = 0;
      if (vertex) {
        sta::VertexOutEdgeIterator edge_iter(vertex, sta_->graph());
        while (edge_iter.hasNext()) {
          sta::Edge* edge = edge_iter.next();
          if (edge->isWire()) {
            fanout++;
          }
        }
      }

      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "  {:<38} | {:<30} | {:>10.2f} | {:>9.2f} | {:>9.2f} | {:>6}",
                 pin_name,
                 gate_type,
                 pin_slack_ps,
                 load_delay_ps,
                 intrinsic_delay_ps,
                 fanout);
      shown++;
    }

    if (critical_never_visited.size() > max_pins_to_show) {
      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "    ... ({} more pins not shown)",
                 critical_never_visited.size() - max_pins_to_show);
    }
  } else {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Category 2: All critical pins were visited (good!)");
  }

  // Summary
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Summary: {} critical pins identified, {} visited, {} had moves "
             "attempted",
             all_critical_pins_.size(),
             all_visited_pins_.size(),
             pin_move_history_.size());
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "  Missed opportunities: {} visited but no moves, {} never "
             "visited",
             visited_no_attempt.size(),
             critical_never_visited.size());
}

void MoveTracker::captureOriginalEndpointSlack()
{
  // Capture the original slack for all violating endpoints
  // This should be called once at the very beginning before any phases
  sta::VertexSet* endpoints = sta_->search()->endpoints();
  for (Vertex* vertex : *endpoints) {
    const Pin* pin = vertex->pin();
    if (!pin) {
      continue;
    }

    Slack slack = sta_->vertexSlack(vertex, MinMax::max());

    // Initialize or update only the original slack (first element of tuple)
    auto it = endpoint_slack_.find(pin);
    if (it != endpoint_slack_.end()) {
      // Already exists, just update original slack
      std::get<0>(it->second) = slack;
    } else {
      // New endpoint, initialize all three values to current slack
      endpoint_slack_[pin] = std::make_tuple(slack, slack, slack);
    }
  }
}

void MoveTracker::capturePrePhaseSlack()
{
  // Capture slack at the start of a phase for all tracked endpoints
  // This becomes the PreSlk column
  for (auto& [endpoint_pin, slack_tuple] : endpoint_slack_) {
    Vertex* vertex = sta_->graph()->pinLoadVertex(endpoint_pin);
    if (vertex) {
      Slack pre_phase_slack = sta_->vertexSlack(vertex, MinMax::max());
      std::get<1>(slack_tuple)
          = pre_phase_slack;  // Update pre-phase slack (2nd element)
    }
  }
}

void MoveTracker::captureInitialSlackDistribution()
{
  // Clear any previous data
  initial_pin_slack_.clear();
  initial_endpoint_slack_.clear();

  // Capture current slack for all output pins in the design with negative slack
  int total_driver_pins = 0;
  int violating_pins = 0;

  // Iterate through all instances in the design
  sta::LeafInstanceIterator* inst_iter
      = sta_->network()->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();

    // Check all output pins of this instance
    sta::InstancePinIterator* pin_iter = sta_->network()->pinIterator(inst);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();

      // Only track output (driver) pins
      if (sta_->network()->isDriver(pin)) {
        total_driver_pins++;

        // Skip pins on clock networks
        Vertex* vertex = sta_->graph()->pinDrvrVertex(pin);
        if (vertex && sta_->search()->isClock(vertex)) {
          continue;
        }

        // Get setup slack for this pin
        Slack slack = sta_->pinSlack(pin, MinMax::max());
        float slack_seconds = delayAsFloat(slack);
        float slack_ns = slack_seconds * 1e9;  // Convert seconds to nanoseconds

        // Only capture pins with negative slack (violating paths)
        if (slack_ns < 0.0) {
          initial_pin_slack_[pin] = slack_ns;  // Store in nanoseconds
          violating_pins++;
        }
      }
    }
    delete pin_iter;
  }
  delete inst_iter;

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Scanned {} driver pins, found {} with negative slack",
             total_driver_pins,
             violating_pins);

  // Capture endpoint slacks
  int total_endpoints = 0;
  int violating_endpoints = 0;

  sta::VertexSet* endpoints = sta_->search()->endpoints();
  for (Vertex* vertex : *endpoints) {
    const Pin* pin = vertex->pin();
    if (!pin) {
      continue;
    }
    total_endpoints++;

    // Get setup slack for this endpoint
    Slack slack = sta_->vertexSlack(vertex, MinMax::max());
    float slack_seconds = delayAsFloat(slack);
    float slack_ns = slack_seconds * 1e9;  // Convert seconds to nanoseconds

    // Only capture endpoints with negative slack (violating endpoints)
    if (slack_ns < 0.0) {
      initial_endpoint_slack_[pin] = slack_ns;  // Store in nanoseconds
      violating_endpoints++;
    }
  }

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Scanned {} endpoints, found {} with negative slack",
             total_endpoints,
             violating_endpoints);
}

void MoveTracker::printSlackDistribution(const std::string& title)
{
  if (initial_pin_slack_.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{}: No initial slack data captured",
               title);
    return;
  }

  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);

  // First pass: find min and max slack values to determine bin range
  // All values in nanoseconds
  float min_slack_ns = std::numeric_limits<float>::max();
  float max_slack_ns = std::numeric_limits<float>::lowest();

  for (const auto& [pin, initial_slack_ns] : initial_pin_slack_) {
    min_slack_ns = std::min(initial_slack_ns, min_slack_ns);
    max_slack_ns = std::max(initial_slack_ns, max_slack_ns);

    // Get current (post-optimization) slack using pinSlack (setup timing)
    Slack post_slack = sta_->pinSlack(pin, MinMax::max());
    float post_slack_ns = delayAsFloat(post_slack) * 1e9;  // Convert to ns
    min_slack_ns = std::min(post_slack_ns, min_slack_ns);
    max_slack_ns = std::max(post_slack_ns, max_slack_ns);
  }

  // Cap max at 0 since we're tracking violations (negative slack)
  max_slack_ns = std::min(max_slack_ns, 0.0f);

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Slack range: min={:.3f} ns, max={:.3f} ns",
             min_slack_ns,
             max_slack_ns);

  // Create approximately 10 bins dynamically (values in nanoseconds)
  constexpr int target_num_bins = 10;
  vector<float> bin_edges;

  if (max_slack_ns > min_slack_ns) {
    float range = max_slack_ns - min_slack_ns;
    float bin_width = range / target_num_bins;

    // Create bin edges
    for (int i = 1; i < target_num_bins; i++) {
      bin_edges.push_back(min_slack_ns + (i * bin_width));
    }
  } else {
    // Degenerate case: all slacks are the same
    bin_edges.push_back(min_slack_ns);
  }

  vector<int> pre_counts(bin_edges.size() + 1, 0);
  vector<int> post_counts(bin_edges.size() + 1, 0);

  // Second pass: count distribution in bins (all values in nanoseconds)
  for (const auto& [pin, initial_slack_ns] : initial_pin_slack_) {
    // Find which bin this slack falls into
    int bin = 0;
    for (size_t i = 0; i < bin_edges.size(); i++) {
      if (initial_slack_ns >= bin_edges[i]) {
        bin = i + 1;
      } else {
        break;
      }
    }
    pre_counts[bin]++;

    // Get current (post-optimization) slack using pinSlack
    Slack post_slack = sta_->pinSlack(pin, MinMax::max());
    float post_slack_ns = delayAsFloat(post_slack) * 1e9;  // Convert to ns

    // Find bin for post slack
    int post_bin = 0;
    for (size_t i = 0; i < bin_edges.size(); i++) {
      if (post_slack_ns >= bin_edges[i]) {
        post_bin = i + 1;
      } else {
        break;
      }
    }
    post_counts[post_bin]++;
  }

  // Find max count for scaling both histograms
  int max_count = 0;
  for (int count : pre_counts) {
    max_count = std::max(count, max_count);
  }
  for (int count : post_counts) {
    max_count = std::max(count, max_count);
  }

  // Calculate integral scale factor: target ~50 characters but use integral
  // scale
  constexpr int target_bar_width = 50;
  int gates_per_hash = (max_count + target_bar_width - 1)
                       / target_bar_width;  // Ceiling division
  if (gates_per_hash == 0) {
    gates_per_hash = 1;
  }

  // Create bin labels in nanoseconds with decimal precision
  // bin_edges are already in nanoseconds
  vector<string> bin_labels;
  std::ostringstream first_label;
  first_label << "< " << std::fixed << std::setprecision(3) << bin_edges[0];
  bin_labels.push_back(first_label.str());

  for (size_t i = 0; i < bin_edges.size() - 1; i++) {
    std::ostringstream label;
    label << "[" << std::fixed << std::setprecision(3) << bin_edges[i] << ","
          << bin_edges[i + 1] << ")";
    bin_labels.push_back(label.str());
  }

  std::ostringstream last_label;
  // Last bin always shows >= 0 since we cap max_slack_ns at 0
  last_label << ">= " << std::fixed << std::setprecision(3) << 0.0;
  bin_labels.push_back(last_label.str());

  // Print Pre-Optimization Histogram
  debugPrint(logger_, RSZ, "move_tracker", 1, "");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Pre-Optimization Gate Slack Distribution:");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<18} | {:>6} | {}",
             "Slack (ns)",
             "Count",
             "Distribution");

  for (size_t i = 0; i < pre_counts.size(); i++) {
    int count = pre_counts[i];
    int bar_len = count / gates_per_hash;
    string bar(bar_len, '#');
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<18} | {:>6} | {}",
               bin_labels[i],
               count,
               bar);
  }

  // Print Post-Optimization Histogram
  debugPrint(logger_, RSZ, "move_tracker", 1, "");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Post-Optimization Gate Slack Distribution:");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<18} | {:>6} | {}",
             "Slack (ns)",
             "Count",
             "Distribution");

  for (size_t i = 0; i < post_counts.size(); i++) {
    int count = post_counts[i];
    int bar_len = count / gates_per_hash;
    string bar(bar_len, '#');
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<18} | {:>6} | {}",
               bin_labels[i],
               count,
               bar);
  }

  // Print summary statistics
  int total_pre = 0;
  int total_post = 0;
  for (int count : pre_counts) {
    total_pre += count;
  }
  for (int count : post_counts) {
    total_post += count;
  }

  debugPrint(logger_, RSZ, "move_tracker", 1, "");

  // Display scale key with integral value
  string scale_label = (gates_per_hash == 1) ? "gate" : "gates";
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Summary: {} driver pins tracked, max bin count: {} (# = {} {})",
             total_pre,
             max_count,
             gates_per_hash,
             scale_label);

  // Now print endpoint histograms using the same bins
  if (!initial_endpoint_slack_.empty()) {
    debugPrint(logger_, RSZ, "move_tracker", 1, "");
    debugPrint(
        logger_, RSZ, "move_tracker", 1, "=== Endpoint Slack Distribution ===");

    // Count endpoint distributions using the same bins
    vector<int> endpoint_pre_counts(bin_edges.size() + 1, 0);
    vector<int> endpoint_post_counts(bin_edges.size() + 1, 0);

    for (const auto& [endpoint_pin, initial_slack_ns] :
         initial_endpoint_slack_) {
      // Find which bin this slack falls into
      int bin = 0;
      for (size_t i = 0; i < bin_edges.size(); i++) {
        if (initial_slack_ns >= bin_edges[i]) {
          bin = i + 1;
        } else {
          break;
        }
      }
      endpoint_pre_counts[bin]++;

      // Get current (post-optimization) slack
      Vertex* vertex = sta_->graph()->pinLoadVertex(endpoint_pin);
      if (vertex) {
        Slack post_slack = sta_->vertexSlack(vertex, MinMax::max());
        float post_slack_ns = delayAsFloat(post_slack) * 1e9;  // Convert to ns

        // Find bin for post slack
        int post_bin = 0;
        for (size_t i = 0; i < bin_edges.size(); i++) {
          if (post_slack_ns >= bin_edges[i]) {
            post_bin = i + 1;
          } else {
            break;
          }
        }
        endpoint_post_counts[post_bin]++;
      }
    }

    // Find max count for scaling endpoint histograms
    int endpoint_max_count = 0;
    for (int count : endpoint_pre_counts) {
      endpoint_max_count = std::max(count, endpoint_max_count);
    }
    for (int count : endpoint_post_counts) {
      endpoint_max_count = std::max(count, endpoint_max_count);
    }

    // Calculate integral scale factor for endpoints
    int endpoints_per_hash = (endpoint_max_count + target_bar_width - 1)
                             / target_bar_width;  // Ceiling division
    if (endpoints_per_hash == 0) {
      endpoints_per_hash = 1;
    }

    // Print Pre-Optimization Endpoint Histogram
    debugPrint(logger_, RSZ, "move_tracker", 1, "");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Pre-Optimization Endpoint Slack Distribution:");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<18} | {:>6} | {}",
               "Slack (ns)",
               "Count",
               "Distribution");

    for (size_t i = 0; i < endpoint_pre_counts.size(); i++) {
      int count = endpoint_pre_counts[i];
      int bar_len = count / endpoints_per_hash;
      string bar(bar_len, '#');
      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "{:<18} | {:>6} | {}",
                 bin_labels[i],
                 count,
                 bar);
    }

    // Print Post-Optimization Endpoint Histogram
    debugPrint(logger_, RSZ, "move_tracker", 1, "");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Post-Optimization Endpoint Slack Distribution:");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<18} | {:>6} | {}",
               "Slack (ns)",
               "Count",
               "Distribution");

    for (size_t i = 0; i < endpoint_post_counts.size(); i++) {
      int count = endpoint_post_counts[i];
      int bar_len = count / endpoints_per_hash;
      string bar(bar_len, '#');
      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "{:<18} | {:>6} | {}",
                 bin_labels[i],
                 count,
                 bar);
    }

    // Print endpoint summary statistics
    int total_endpoint_pre = 0;
    int total_endpoint_post = 0;
    for (int count : endpoint_pre_counts) {
      total_endpoint_pre += count;
    }
    for (int count : endpoint_post_counts) {
      total_endpoint_post += count;
    }

    debugPrint(logger_, RSZ, "move_tracker", 1, "");

    // Display scale key with integral value
    string endpoint_scale_label
        = (endpoints_per_hash == 1) ? "endpoint" : "endpoints";
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Summary: {} endpoints tracked, max bin count: {} (# = {} {})",
               total_endpoint_pre,
               endpoint_max_count,
               endpoints_per_hash,
               endpoint_scale_label);
  }
}

void MoveTracker::printTopBinEndpoints(const std::string& title,
                                       int max_endpoints)
{
  debugPrint(logger_, RSZ, "move_tracker", 1, "{}:", title);

  // Collect all violating endpoints (negative slack) after optimization
  vector<pair<const Pin*, Slack>> violating_endpoints;

  sta::VertexSet* endpoints = sta_->search()->endpoints();
  for (Vertex* vertex : *endpoints) {
    const Pin* endpoint_pin = vertex->pin();
    if (!endpoint_pin) {
      continue;
    }

    Slack slack = sta_->vertexSlack(vertex, MinMax::max());
    if (slack < 0.0) {
      violating_endpoints.emplace_back(endpoint_pin, slack);
    }
  }

  if (violating_endpoints.empty()) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "No violating endpoints after optimization (all meet timing!)");
    return;
  }

  // Sort by most critical (most negative slack) first
  std::sort(violating_endpoints.begin(),
            violating_endpoints.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Found {} violating endpoints after optimization",
             violating_endpoints.size());
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Analyzing top {} most critical endpoints:",
             std::min(max_endpoints, (int) violating_endpoints.size()));
  debugPrint(logger_, RSZ, "move_tracker", 1, "");

  // Print header
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<40} | {:<40} | {:>10} | {:>10} | {:>7} | {:>6} | {:<40} | "
             "{:>8} | {:>8} | {:>8} | {:>6}",
             "Endpoint",
             "Startpoint",
             "Slack(ns)",
             "EpTNS(ns)",  // Endpoint-local TNS (sum of negative slacks through
                           // this EP)
             "NegPath",  // Number of negative slack paths through this endpoint
             "Levels",
             "Worst Delay Pin (Gate)",
             "Arc(ps)",
             "Load(ps)",
             "Intr(ps)",
             "Fanout");

  int count = 0;
  for (const auto& [endpoint_pin, endpoint_slack] : violating_endpoints) {
    if (count >= max_endpoints) {
      break;
    }

    // Get endpoint vertex
    Vertex* endpoint_vertex = sta_->graph()->pinLoadVertex(endpoint_pin);
    if (!endpoint_vertex) {
      continue;
    }

    // Get the worst (most critical) path to this endpoint
    sta::Path* path
        = sta_->vertexWorstSlackPath(endpoint_vertex, MinMax::max());
    if (!path) {
      continue;
    }

    // Expand the path to get all points
    sta::PathExpanded expanded(path, sta_);

    // Find startpoint (output of sequential element, not clock pin)
    const Pin* startpoint_pin = nullptr;
    for (int i = 0; i < expanded.size(); i++) {
      const sta::Path* start_path = expanded.path(i);
      if (!start_path) {
        continue;
      }
      const Pin* pin = start_path->pin(sta_);
      if (!pin) {
        continue;
      }

      // Check if this is an output pin of a sequential element
      sta::Instance* inst = sta_->network()->instance(pin);
      if (inst) {
        sta::LibertyCell* cell = sta_->network()->libertyCell(inst);
        if (cell && cell->hasSequentials()) {
          // This is a sequential cell - check if pin is an output
          if (sta_->network()->direction(pin)->isOutput()) {
            startpoint_pin = pin;
            break;
          }
        }
      }
    }

    // Fallback to first pin if no sequential output found
    if (!startpoint_pin && expanded.size() > 0) {
      const sta::Path* start_path = expanded.path(0);
      if (start_path) {
        startpoint_pin = start_path->pin(sta_);
      }
    }

    // Count levels (number of stages/gates in the path)
    // A level is each timing point in the expanded path
    int num_levels = expanded.size();

    // Find gate with worst delay (using arrival time differences)
    const Pin* worst_delay_pin = nullptr;
    sta::Delay worst_delay = 0.0;

    // Iterate through path to find worst delay arc
    for (int i = 1; i < expanded.size(); i++) {
      const sta::Path* curr_path = expanded.path(i);
      const sta::Path* prev_path = expanded.path(i - 1);
      if (!curr_path || !prev_path) {
        continue;
      }

      // Calculate arc delay as difference in arrival times
      sta::Arrival curr_arrival = curr_path->arrival();
      sta::Arrival prev_arrival = prev_path->arrival();
      sta::Delay arc_delay = curr_arrival - prev_arrival;

      if (arc_delay > worst_delay) {
        worst_delay = arc_delay;
        worst_delay_pin = curr_path->pin(sta_);
      }
    }

    // Get delay information for worst delay pin
    float load_delay_ps = 0.0;
    float intrinsic_delay_ps = 0.0;
    int fanout = 0;

    if (worst_delay_pin) {
      // Get load delay and intrinsic delay from graph arc delays
      sta::Vertex* worst_vertex = sta_->graph()->pinLoadVertex(worst_delay_pin);
      if (worst_vertex) {
        // Calculate fanout by counting wire edges from this vertex
        sta::VertexOutEdgeIterator edge_iter(worst_vertex, sta_->graph());
        while (edge_iter.hasNext()) {
          sta::Edge* edge = edge_iter.next();
          if (edge->isWire()) {
            fanout++;
          }
        }

        // For now, use a simple heuristic: assume intrinsic is ~40% of total
        // delay and load delay is ~60% (typical for loaded gates) A more
        // accurate calculation would require detailed arc delay analysis
        float total_delay_ps = delayAsFloat(worst_delay) * 1e12;
        intrinsic_delay_ps = total_delay_ps * 0.4f;
        load_delay_ps = total_delay_ps * 0.6f;
      }
    }

    // Format names
    string endpoint_name = sta_->network()->pathName(endpoint_pin);
    if (endpoint_name.length() > 40) {
      endpoint_name = endpoint_name.substr(0, 37) + "...";
    }

    string startpoint_name = "unknown";
    if (startpoint_pin) {
      startpoint_name = sta_->network()->pathName(startpoint_pin);
      if (startpoint_name.length() > 40) {
        startpoint_name = startpoint_name.substr(0, 37) + "...";
      }
    }

    string worst_delay_pin_name = "none";
    if (worst_delay_pin) {
      worst_delay_pin_name = sta_->network()->pathName(worst_delay_pin);

      // Get gate type and append in parentheses
      sta::Instance* inst = sta_->network()->instance(worst_delay_pin);
      if (inst) {
        sta::LibertyCell* cell = sta_->network()->libertyCell(inst);
        if (cell) {
          string gate_type = cell->name();
          worst_delay_pin_name += " (" + gate_type + ")";
        }
      }

      // Truncate from the beginning if too long (keep the instance/pin at the
      // end)
      if (worst_delay_pin_name.length() > 40) {
        worst_delay_pin_name
            = "..."
              + worst_delay_pin_name.substr(worst_delay_pin_name.length() - 37);
      }
    }

    // Enumerate multiple paths through this endpoint to count negative slack
    // paths and compute the actual local TNS
    vector<pair<Slack, const sta::PathEnd*>> paths
        = enumerateEndpointPaths(endpoint_pin, 100);

    int neg_path_count = 0;
    Slack local_tns = 0.0;

    // Count negative slack paths and sum their slacks for local TNS
    for (const auto& [path_slack, path_end] : paths) {
      if (path_slack < 0.0) {
        neg_path_count++;
        local_tns += path_slack;
      }
    }

    // If we didn't find any paths (shouldn't happen), fall back to 1 path
    if (neg_path_count == 0) {
      neg_path_count = 1;
      local_tns = endpoint_slack;
    }

    // Convert to convenient units
    float slack_ns = delayAsFloat(endpoint_slack) * 1e9;
    float local_tns_ns = delayAsFloat(local_tns) * 1e9;
    float worst_delay_ps = delayAsFloat(worst_delay) * 1e12;

    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<40} | {:<40} | {:>10.3f} | {:>10.3f} | {:>7} | {:>6} | "
               "{:<40} | {:>8.1f} | {:>8.1f} | {:>8.1f} | {:>6}",
               endpoint_name,
               startpoint_name,
               slack_ns,
               local_tns_ns,
               neg_path_count,
               num_levels,
               worst_delay_pin_name,
               worst_delay_ps,
               load_delay_ps,
               intrinsic_delay_ps,
               fanout);

    count++;
  }

  if (violating_endpoints.size() > max_endpoints) {
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "... ({} more violating endpoints not shown)",
               violating_endpoints.size() - max_endpoints);
  }

  // Print summary statistics
  Slack worst_slack = violating_endpoints[0].second;
  float wns_ns = delayAsFloat(worst_slack) * 1e9;

  Slack total_negative_slack = 0.0;
  for (const auto& [ep_pin, ep_slack] : violating_endpoints) {
    total_negative_slack += ep_slack;
  }
  float tns_ns = delayAsFloat(total_negative_slack) * 1e9;

  debugPrint(logger_, RSZ, "move_tracker", 1, "");
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "Post-Optimization Summary: WNS = {:.3f} ns, TNS = {:.3f} ns, {} "
             "violating endpoints",
             wns_ns,
             tns_ns,
             violating_endpoints.size());
}

// Helper function to enumerate paths to an endpoint
vector<pair<Slack, const sta::PathEnd*>> MoveTracker::enumerateEndpointPaths(
    const sta::Pin* endpoint_pin,
    int max_paths)
{
  vector<pair<Slack, const sta::PathEnd*>> result;

  // Create ExceptionTo for this endpoint to enumerate paths
  sta::Network* network = sta_->network();
  sta::Sdc* sdc = sta_->sdc();
  sta::Search* search = sta_->search();
  sta::Corner* corner = sta_->cmdCorner();

  sta::PinSet* to_pins = new sta::PinSet(network);
  to_pins->insert(endpoint_pin);
  sta::ExceptionTo* to = sdc->makeExceptionTo(to_pins,
                                              nullptr,
                                              nullptr,
                                              sta::RiseFallBoth::riseFall(),
                                              sta::RiseFallBoth::riseFall());

  // Find paths to this endpoint
  sta::PathEndSeq path_ends
      = search->findPathEnds(nullptr,                // from
                             nullptr,                // thrus
                             to,                     // to (this endpoint)
                             false,                  // unconstrained
                             corner,                 // corner
                             sta::MinMaxAll::all(),  // min_max
                             max_paths,              // group_path_count
                             max_paths,              // endpoint_path_count
                             false,                  // unique_pins
                             false,                  // unique_edges
                             -sta::INF,              // slack_min
                             sta::INF,               // slack_max
                             true,                   // sort_by_slack
                             nullptr,                // group_names
                             true,                   // setup
                             false,                  // hold
                             true,                   // recovery
                             true,                   // removal
                             true,                   // clk_gating_setup
                             true);                  // clk_gating_hold

  // Collect (slack, path_end) pairs
  for (const sta::PathEnd* path_end : path_ends) {
    Slack path_slack = path_end->slack(search);
    result.push_back({path_slack, path_end});
  }

  return result;
}

// Helper function to draw a histogram
void MoveTracker::drawHistogram(const string& title,
                                const vector<string>& bin_labels,
                                const vector<int>& bin_counts,
                                const string& value_label,
                                const string& count_label)
{
  // Find max count for scaling
  int max_count = 0;
  for (int count : bin_counts) {
    max_count = std::max(count, max_count);
  }

  // Calculate integral scale factor: target ~50 characters but use integral
  // scale
  constexpr int target_bar_width = 50;
  int gates_per_hash = (max_count + target_bar_width - 1)
                       / target_bar_width;  // Ceiling division
  if (gates_per_hash == 0) {
    gates_per_hash = 1;
  }

  // Print histogram
  debugPrint(logger_, RSZ, "move_tracker", 1, "");
  debugPrint(logger_, RSZ, "move_tracker", 1, "{}", title);
  debugPrint(logger_,
             RSZ,
             "move_tracker",
             1,
             "{:<18} | {:>6} | {}",
             value_label,
             count_label,
             "Distribution");

  for (size_t i = 0; i < bin_counts.size(); i++) {
    int count = bin_counts[i];
    int bar_len = count / gates_per_hash;
    string bar(bar_len, '#');
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "{:<18} | {:>6} | {}",
               bin_labels[i],
               count,
               bar);
  }
}

// Print histogram of path slacks for the most critical endpoint
void MoveTracker::printCriticalEndpointPathHistogram(const string& title)
{
  debugPrint(logger_, RSZ, "move_tracker", 1, "");
  debugPrint(logger_, RSZ, "move_tracker", 1, "=== {} ===", title);

  // Find the top 3 most critical endpoints
  sta::Network* network = sta_->network();
  const sta::VertexSet* endpoints = sta_->endpoints();

  vector<pair<Slack, const sta::Pin*>> endpoint_slacks;
  for (sta::Vertex* vertex : *endpoints) {
    const sta::Pin* pin = vertex->pin();
    if (pin) {
      Slack pin_slack = sta_->vertexSlack(vertex, MinMax::max());
      if (pin_slack < 0.0) {  // Only consider violating endpoints
        endpoint_slacks.push_back({pin_slack, pin});
      }
    }
  }

  if (endpoint_slacks.empty()) {
    debugPrint(
        logger_, RSZ, "move_tracker", 1, "No violating endpoints found.");
    return;
  }

  // Sort by slack (most negative first)
  std::sort(endpoint_slacks.begin(),
            endpoint_slacks.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // Process top 3 endpoints
  int num_endpoints_to_show
      = std::min(3, static_cast<int>(endpoint_slacks.size()));

  // First pass: collect all paths for all endpoints to determine common slack
  // range
  vector<vector<float>> all_endpoint_path_slacks(num_endpoints_to_show);
  float global_min_slack_ns = sta::INF;
  float global_max_slack_ns = -sta::INF;

  for (int ep_idx = 0; ep_idx < num_endpoints_to_show; ep_idx++) {
    const sta::Pin* endpoint_pin = endpoint_slacks[ep_idx].second;

    // Enumerate paths to this endpoint
    vector<pair<Slack, const sta::PathEnd*>> paths
        = enumerateEndpointPaths(endpoint_pin, 100);

    if (paths.empty()) {
      continue;
    }

    // Collect slacks in nanoseconds
    vector<float> path_slacks_ns;
    for (const auto& [slack, path_end] : paths) {
      float slack_ns = delayAsFloat(slack) * 1e9;
      path_slacks_ns.push_back(slack_ns);

      // Update global min/max
      if (slack_ns < global_min_slack_ns) {
        global_min_slack_ns = slack_ns;
      }
      if (slack_ns > global_max_slack_ns) {
        global_max_slack_ns = slack_ns;
      }
    }

    all_endpoint_path_slacks[ep_idx] = path_slacks_ns;
  }

  // Cap max at 0 for better binning (since we're focused on negative slack)
  if (global_max_slack_ns > 0.0) {
    global_max_slack_ns = 0.0;
  }

  // Create common bin edges for all endpoints
  constexpr int num_bins = 10;
  float range = global_max_slack_ns - global_min_slack_ns;
  float bin_width
      = range / (num_bins - 1);  // -1 because we want edges, not bins

  vector<float> bin_edges;
  for (int i = 0; i < num_bins; i++) {
    bin_edges.push_back(global_min_slack_ns + i * bin_width);
  }

  // Create bin labels in nanoseconds with decimal precision
  vector<string> bin_labels;
  std::ostringstream first_label;
  first_label << "< " << std::fixed << std::setprecision(3) << bin_edges[0];
  bin_labels.push_back(first_label.str());

  for (size_t i = 0; i < bin_edges.size() - 1; i++) {
    std::ostringstream label;
    label << "[" << std::fixed << std::setprecision(3) << bin_edges[i] << ","
          << bin_edges[i + 1] << ")";
    bin_labels.push_back(label.str());
  }

  std::ostringstream last_label;
  last_label << ">= " << std::fixed << std::setprecision(3) << bin_edges.back();
  bin_labels.push_back(last_label.str());

  // Second pass: print histograms using common bins
  for (int ep_idx = 0; ep_idx < num_endpoints_to_show; ep_idx++) {
    const sta::Pin* endpoint_pin = endpoint_slacks[ep_idx].second;
    Slack endpoint_slack = endpoint_slacks[ep_idx].first;

    string endpoint_name = network->pathName(endpoint_pin);
    float endpoint_slack_ns = delayAsFloat(endpoint_slack) * 1e9;

    const vector<float>& path_slacks_ns = all_endpoint_path_slacks[ep_idx];

    if (path_slacks_ns.empty()) {
      debugPrint(logger_, RSZ, "move_tracker", 1, "");
      debugPrint(logger_,
                 RSZ,
                 "move_tracker",
                 1,
                 "Endpoint #{}: {} (slack = {:.3f} ns)",
                 ep_idx + 1,
                 endpoint_name,
                 endpoint_slack_ns);
      debugPrint(
          logger_, RSZ, "move_tracker", 1, "No paths found to endpoint.");
      continue;
    }

    debugPrint(logger_, RSZ, "move_tracker", 1, "");
    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Endpoint #{}: {} (slack = {:.3f} ns)",
               ep_idx + 1,
               endpoint_name,
               endpoint_slack_ns);

    debugPrint(logger_,
               RSZ,
               "move_tracker",
               1,
               "Found {} paths to this endpoint",
               path_slacks_ns.size());

    // Count paths in each bin using common bin edges
    vector<int> bin_counts(num_bins + 1, 0);
    for (float slack_ns : path_slacks_ns) {
      int bin = 0;
      for (size_t i = 0; i < bin_edges.size(); i++) {
        if (slack_ns >= bin_edges[i]) {
          bin = i + 1;
        } else {
          break;
        }
      }
      bin_counts[bin]++;
    }

    // Draw the histogram
    std::ostringstream histogram_title;
    histogram_title << "Path Slack Distribution for Endpoint #" << (ep_idx + 1);
    drawHistogram(
        histogram_title.str(), bin_labels, bin_counts, "Slack (ns)", "Paths");
  }
}

}  // namespace rsz
