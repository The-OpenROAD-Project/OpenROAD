// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace est {
class EstimateParasitics;
}

namespace sta {
class Vertex;
class Corner;
class Edge;
class Path;
}  // namespace sta

namespace rsz {

using sta::Arrival;
using sta::Cell;
using sta::Corner;
using sta::dbNetwork;
using sta::Delay;
using sta::Edge;
using sta::Path;
using sta::Pin;
using sta::Vertex;
using utl::Logger;

struct SizingProblem;
struct SizingPin;
struct SizingOption;

class Resizer;
class BottleneckAnalysis : public sta::dbStaState
{
  struct DriverPinData
  {
    sta::Pin* pin;
    std::vector<std::pair<int, double>> fanins;
    float fwd_accumulator;
    float bwd_accumulator;
    bool launcher = false;
  };

  struct EndpointData
  {
    int fanin_index;
    double required;
  };

 public:
  BottleneckAnalysis(Resizer* resizer);
  ~BottleneckAnalysis() override;

  void analyze(double alpha, int npins, bool verbose);
  void pinRemoved(Pin* pin);
  float pinValue(Pin* pin);

 protected:
  void init();
  void initOnCorner(Corner* corner);

  bool launchesPath(Vertex* drvr);
  Arrival referenceTime(Path* path);
  Edge* prevEdge(Vertex* vertex);
  void encodePropagation(Vertex* drvr);
  void encodeLaunching(Vertex* drvr);
  void encodeSampling(Vertex* vertex);
  bool isTimingUnconstrained(Vertex* vertex);

  Corner* corner_ = nullptr;
  Resizer* resizer_ = nullptr;
  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;

  std::vector<DriverPinData> driver_pins_;
  std::vector<EndpointData> endpoints_;
  std::map<Vertex*, int> drvr_indices_;
};

};  // namespace rsz
