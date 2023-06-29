// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

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
