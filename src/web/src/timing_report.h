// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <limits>
#include <string>
#include <vector>

#include "boost/json/object.hpp"

namespace odb {
class dbBlock;
class dbNet;
class dbITerm;
class dbBTerm;
class dbInst;
}  // namespace odb

namespace sta {
class dbSta;
class Path;
}  // namespace sta

namespace web {

struct TimingNode
{
  std::string pin_name;
  int fanout = 0;
  bool is_rising = false;
  bool is_clock = false;
  float time = 0.0f;   // arrival
  float delay = 0.0f;  // incremental
  float slew = 0.0f;
  float load = 0.0f;
};

struct TimingPathSummary
{
  std::string start_clk;
  std::string end_clk;
  float required = 0.0f;
  float arrival = 0.0f;
  float slack = 0.0f;
  float skew = 0.0f;
  float path_delay = 0.0f;
  int logic_depth = 0;
  int fanout = 0;
  std::string start_pin;
  std::string end_pin;
  std::vector<TimingNode> data_nodes;
  std::vector<TimingNode> capture_nodes;
};

struct SlackHistogramBin
{
  float lower;       // bin lower edge (user units)
  float upper;       // bin upper edge (user units)
  int count;         // number of endpoints in this bin
  bool is_negative;  // true if bin center < 0
};

// One path-group's per-bin endpoint counts, parallel to SlackHistogramResult
// ::bins.  Used to render a stacked slack histogram (Qt parity).
struct SlackHistogramSeries
{
  std::string name;
  std::vector<int> counts;
};

struct SlackHistogramResult
{
  std::vector<SlackHistogramBin> bins;
  int unconstrained_count = 0;
  int total_endpoints = 0;
  std::string time_unit;
  // Per-path-group breakdown over the same bin edges.  Empty for the single
  // aggregate histogram; populated when multiple path groups are requested.
  std::vector<SlackHistogramSeries> series;
};

struct ChartFilters
{
  std::vector<std::string> path_groups;
  std::vector<std::string> clocks;
};

struct FanoutHistogramBin
{
  int lower;  // bin lower edge (inclusive, in loads)
  int upper;  // bin upper edge (exclusive, in loads)
  int count;  // number of nets in this bin
};

struct FanoutHistogramResult
{
  std::vector<FanoutHistogramBin> bins;
  int total_nets = 0;
};

// Net-length (HPWL) histogram — a Web-GUI chart type with no Qt equivalent.
struct NetLengthHistogramBin
{
  float lower;  // bin edges in user units (µm) or DBU per `length_unit`
  float upper;
  int count;
};

struct NetLengthHistogramResult
{
  std::vector<NetLengthHistogramBin> bins;
  int total_nets = 0;
  std::string length_unit;  // "µm" or "DBU"
};

// One pin in a timing cone.  `iterm`/`bterm` are mutually exclusive (the pin is
// either an instance terminal or a top-level port).  `depth` is the logic level
// relative to the source pin: negative for fanin, positive for fanout, 0 for
// the source.  `slack` (user time units) is valid only when `has_slack`.
// `source_indices` lists the indices (into TimingConeResult::nodes) of the
// nodes one level closer to the source that drive this node — used to draw
// flight lines.
struct TimingConeNode
{
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbInst* inst = nullptr;  // null for a bterm
  int depth = 0;
  float slack = 0.0f;
  bool has_slack = false;
  std::vector<int> source_indices;
};

struct TimingConeResult
{
  std::vector<TimingConeNode> nodes;
  float min_slack = 0.0f;    // user time units; valid only when `constrained`
  float max_slack = 0.0f;    // user time units; valid only when `constrained`
  bool constrained = false;  // true when at least one node has finite slack
  std::string time_unit;     // e.g. "ps" — for legend formatting
  bool ok = false;
  std::string error;
};

class TimingReport
{
 public:
  explicit TimingReport(sta::dbSta* sta);

  std::vector<TimingPathSummary> getReport(
      bool is_setup,
      int max_paths = 100,
      float slack_min = -std::numeric_limits<float>::max(),
      float slack_max = std::numeric_limits<float>::max()) const;

  // `path_groups` (optional) requests a per-group stacked breakdown: when it
  // has entries, `result.series` is populated over the shared bin edges and the
  // aggregate `bins` cover the union of those groups.  The single `path_group`
  // arg is kept for the unfiltered / single-group case.
  SlackHistogramResult getSlackHistogram(
      bool is_setup,
      const std::string& path_group = "",
      const std::string& clock_name = "",
      const std::vector<std::string>& path_groups = {}) const;

  ChartFilters getChartFilters() const;

  // Compute the timing (logic) cone through `pin_name` (an ITerm "inst/pin" or
  // a BTerm port name).  Mirrors the Qt GUI's TimingConeRenderer: walks the STA
  // timing graph for fanin and/or fanout, assigns a signed logic depth per pin
  // and annotates the worst-slack at each pin so callers can color by slack.
  // `fanin_depth`/`fanout_depth` cap the traversal levels (0 = unlimited, as in
  // the Qt GUI); this per-direction limit is a Web-GUI addition.
  TimingConeResult computeTimingCone(const std::string& pin_name,
                                     bool fanin,
                                     bool fanout,
                                     int fanin_depth,
                                     int fanout_depth) const;

 private:
  void expandPath(sta::Path* path,
                  float offset,
                  bool clock_expanded,
                  std::vector<TimingNode>& nodes,
                  int& logic_depth,
                  int& total_fanout) const;

  sta::dbSta* sta_;
};

// ── JSON serialization helpers (shared by request_handler and saveReport) ──

boost::json::object serializeTimingNode(const TimingNode& n);
boost::json::object serializeTimingPath(const TimingPathSummary& p);
boost::json::object serializeTimingPaths(
    const std::vector<TimingPathSummary>& paths);
boost::json::object serializeSlackHistogram(const SlackHistogramResult& h);
boost::json::object serializeChartFilters(const ChartFilters& f);

// Net fanout histogram (loads = term_count - 1, skipping power/ground nets).
// Free function: depends only on odb, not STA.
FanoutHistogramResult computeFanoutHistogram(odb::dbBlock* block);
boost::json::object serializeFanoutHistogram(const FanoutHistogramResult& h);

// Half-perimeter wire length (HPWL) of a net's terminal bounding box, in DBU.
// Shared by the net-length histogram and the select-by-bin handler.
int netHpwlDbu(odb::dbNet* net);

// Net-length (HPWL) histogram: half-perimeter of each signal net's terminal
// bounding box, in microns (use_dbu=false) or DBU (use_dbu=true).  odb-only.
NetLengthHistogramResult computeNetLengthHistogram(odb::dbBlock* block,
                                                   bool use_dbu);
boost::json::object serializeNetLengthHistogram(
    const NetLengthHistogramResult& h);

}  // namespace web
