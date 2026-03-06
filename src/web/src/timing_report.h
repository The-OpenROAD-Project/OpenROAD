// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace sta {
class dbSta;
class DcalcAnalysisPt;
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

class TimingReport
{
 public:
  explicit TimingReport(sta::dbSta* sta);

  std::vector<TimingPathSummary> getReport(bool is_setup,
                                           int max_paths = 100) const;

 private:
  void expandPath(sta::Path* path,
                  sta::DcalcAnalysisPt* dcalc_ap,
                  float offset,
                  bool clock_expanded,
                  std::vector<TimingNode>& nodes,
                  int& logic_depth,
                  int& total_fanout) const;

  sta::dbSta* sta_;
};

}  // namespace web
