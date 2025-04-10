// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "Clock.h"

namespace cts {

class HTreeBuilder;
class SinkClustering;

class CtsObserver
{
 public:
  virtual ~CtsObserver() = default;

  virtual void initializeWithClock(HTreeBuilder* h_tree_builder, Clock& clock)
      = 0;
  virtual void initializeWithPoints(SinkClustering* sink_clustering,
                                    const std::vector<Point<double>>& points)
      = 0;
  virtual void clockPlot(bool pause) = 0;
  virtual void status(const std::string& message) = 0;
};

}  // namespace cts
