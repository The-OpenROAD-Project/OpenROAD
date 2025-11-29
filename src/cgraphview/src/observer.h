// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "odb/geom.h"

// This is an interface that allows the Example object to notify observers
// (i.e. graphics) that something interesting has happened without having
// to depend on them.  This allows Example to remain free of any dependency
// on the GUI which facilitates C++ unit testing.

namespace cgv {

class Observer
{
 public:
  virtual ~Observer() = default;

  virtual void addEdges(std::vector<std::pair<odb::Point, odb::Point>> edges) = 0;
};

}  // namespace exa
