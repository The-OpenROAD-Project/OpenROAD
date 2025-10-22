// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "gpl/AbstractGraphics.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace gpl {

// This is a implementation that disables all graphics.
class GraphicsNone : public AbstractGraphics
{
 public:
  std::unique_ptr<AbstractGraphics> MakeNew(utl::Logger* logger) const override
  {
    return std::make_unique<GraphicsNone>();
  }

  ~GraphicsNone() override;

  void debugForMbff() override {}
  void debugForInitialPlace(
      std::shared_ptr<PlacerBaseCommon> pbc,
      std::vector<std::shared_ptr<PlacerBase>>& pbVec) override
  {
  }
  void debugForNesterovPlace(NesterovPlace* np,
                             std::shared_ptr<PlacerBaseCommon> pbc,
                             std::shared_ptr<NesterovBaseCommon> nbc,
                             std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                             std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                             bool draw_bins,
                             odb::dbInst* inst) override {};

  void addIter(int iter, double overflow) override {}
  void addTimingDrivenIter(int iter) override {}
  void addRoutabilitySnapshot(int iter) override {}
  void addRoutabilityIter(int iter, bool revert) override {}

  void mbffMapping(const LineSegs& segs) override {}
  void mbffFlopClusters(const std::vector<odb::dbInst*>& ffs) override {}

  void status(std::string_view message) override {}

  bool enabled() override { return false; }
  void setDebugOn(bool set_on) override {}

  void gifStart(std::string_view path) override {};
  void deleteLabel(std::string_view label_name) override {}
  void gifEnd() override {}

 protected:
  void cellPlotImpl(bool pause) override {}

  void addFrameLabelImpl(const odb::Rect& bbox,
                         std::string_view label,
                         std::string_view label_name,
                         int image_width_px) override
  {
  }
  void saveLabeledImageImpl(std::string_view path,
                            std::string_view label,
                            bool select_buffers,
                            std::string_view heatmap_control,
                            int image_width_px) override
  {
  }
  void gifAddFrameImpl(const odb::Rect& region,
                       int width_px,
                       double dbu_per_pixel,
                       std::optional<int> delay) override
  {
  }
};

}  // namespace gpl
