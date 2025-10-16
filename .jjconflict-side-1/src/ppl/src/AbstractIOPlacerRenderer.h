// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "Netlist.h"

namespace ppl {

class AbstractIOPlacerRenderer
{
 public:
  virtual ~AbstractIOPlacerRenderer() = default;

  virtual void setCurrentIteration(const int& current_iteration) = 0;
  virtual void setPaintingInterval(const int& painting_interval) = 0;
  virtual void setPinAssignment(const std::vector<IOPin>& assignment) = 0;
  virtual void setSinks(const std::vector<std::vector<InstancePin>>& sinks) = 0;
  virtual void setIsNoPauseMode(const bool& is_no_pause_mode) = 0;

  virtual void redrawAndPause() = 0;
};

}  // namespace ppl
