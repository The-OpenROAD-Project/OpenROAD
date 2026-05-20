// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include "AbstractRoutingCongestionDataSource.h"
#include "gui/heatMap.h"

namespace grt {

// Adapts a gui::HeatMapSourceHandle to the gui-free abstract interface
// consumed by GlobalRouter. Lives in :ui so that :grt does not link or
// reference any gui symbols.
class HeatMapHandleAdapter : public AbstractRoutingCongestionDataSource
{
 public:
  explicit HeatMapHandleAdapter(gui::HeatMapSourceHandle handle)
      : handle_(std::move(handle))
  {
  }

  void invalidate() override
  {
    if (handle_) {
      handle_->invalidateInstances();
    }
  }

 private:
  gui::HeatMapSourceHandle handle_;
};

}  // namespace grt
