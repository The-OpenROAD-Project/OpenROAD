// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <limits>
#include <string>
#include <vector>

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

struct SlackHistogramResult
{
  std::vector<SlackHistogramBin> bins;
  int unconstrained_count = 0;
  int total_endpoints = 0;
  std::string time_unit;
};

struct ChartFilters
{
  std::vector<std::string> path_groups;
  std::vector<std::string> clocks;
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

  SlackHistogramResult getSlackHistogram(bool is_setup,
                                         const std::string& path_group = "",
                                         const std::string& clock_name
                                         = "") const;

  ChartFilters getChartFilters() const;

 private:
  void expandPath(sta::Path* path,
                  float offset,
                  bool clock_expanded,
                  std::vector<TimingNode>& nodes,
                  int& logic_depth,
                  int& total_fanout) const;

  sta::dbSta* sta_;
};

}  // namespace web
