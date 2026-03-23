// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace sta {
class dbSta;
}  // namespace sta

namespace web {

struct ClockTreeNode
{
  int id = 0;
  int parent_id = -1;  // -1 for root
  std::string name;    // instance or port name
  std::string pin_name;

  enum Type
  {
    ROOT,
    BUFFER,
    INVERTER,
    CLOCK_GATE,
    REGISTER,
    MACRO,
    UNKNOWN
  };
  Type type = UNKNOWN;

  float arrival = 0.0f;  // input arrival (user time units)
  float delay = 0.0f;    // cell delay (user time units)
  int fanout = 0;
  int level = 0;

  int dbu_x = 0;
  int dbu_y = 0;

  static const char* typeToString(Type t);
};

struct ClockTreeData
{
  std::string clock_name;
  float min_arrival = 0.0f;
  float max_arrival = 0.0f;
  std::string time_unit;
  std::vector<ClockTreeNode> nodes;
};

class ClockTreeReport
{
 public:
  explicit ClockTreeReport(sta::dbSta* sta);

  std::vector<ClockTreeData> getReport() const;

 private:
  sta::dbSta* sta_;
};

}  // namespace web
