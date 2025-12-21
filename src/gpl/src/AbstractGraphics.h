// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "routeBase.h"

namespace gpl {

class PlacerBaseCommon;
class PlacerBase;
class NesterovBase;
class NesterovBaseCommon;
class NesterovPlace;

// This is an abstract interface for gpl to draw debugging graphics on the
// layout.
class AbstractGraphics
{
 public:
  using LineSeg = std::pair<odb::Point, odb::Point>;
  using LineSegs = std::vector<LineSeg>;

  virtual ~AbstractGraphics();

  // Create a new object of the same class.
  virtual std::unique_ptr<AbstractGraphics> MakeNew(utl::Logger* logger) const
      = 0;

  // Turns debug on and sets mode to Mbff.
  virtual void debugForMbff() = 0;

  // Turns debug on and sets mode to Initial(place).
  virtual void debugForInitialPlace(
      std::shared_ptr<PlacerBaseCommon> pbc,
      std::vector<std::shared_ptr<PlacerBase>>& pbVec)
      = 0;

  // Turns debug on and sets mode to Nesterov(place)
  virtual void debugForNesterovPlace(
      NesterovPlace* np,
      std::shared_ptr<PlacerBaseCommon> pbc,
      std::shared_ptr<NesterovBaseCommon> nbc,
      std::shared_ptr<RouteBase> rb,
      std::vector<std::shared_ptr<PlacerBase>>& pbVec,
      std::vector<std::shared_ptr<NesterovBase>>& nbVec,
      bool draw_bins,
      odb::dbInst* inst)
      = 0;

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false) { cellPlotImpl(pause); }

  // Update the chart for the current iter
  virtual void addIter(int iter, double overflow) = 0;
  virtual void addTimingDrivenIter(int iter) = 0;
  virtual void addRoutabilitySnapshot(int iter) = 0;
  virtual void addRoutabilityIter(int iter, bool revert) = 0;

  // Draw the MBFF mapping
  virtual void mbffMapping(const LineSegs& segs) = 0;
  virtual void mbffFlopClusters(const std::vector<odb::dbInst*>& ffs) = 0;

  // Show a message in the status bar
  virtual void status(std::string_view message) = 0;

  // Is the graphics enabled or not.
  //  Graphics could be disabled for
  //
  //  1. Debug not being on.
  //  2. Being used as a library (ex. in a test).
  //  3. The GUI is not displayed.
  virtual bool enabled() = 0;

  // Set the debug condition to `set_on`.
  //
  // Note that even if set, the graphics may still be disabled under conditions
  // #2 and #3 of enabled() above.
  virtual void setDebugOn(bool set_on) = 0;

  void addFrameLabel(const odb::Rect& bbox,
                     std::string_view label,
                     std::string_view label_name,
                     int image_width_px = 500)
  {
    addFrameLabelImpl(bbox, label, label_name, image_width_px);
  }

  void saveLabeledImage(std::string_view path,
                        std::string_view label,
                        bool select_buffers,
                        std::string_view heatmap_control = "",
                        int image_width_px = 500)
  {
    saveLabeledImageImpl(
        path, label, select_buffers, heatmap_control, image_width_px);
  }

  // Gui functions.
  // Gui functions.
  virtual int gifStart(std::string_view path) = 0;
  void gifAddFrame(int key,
                   const odb::Rect& region = odb::Rect(),
                   int width_px = 0,
                   double dbu_per_pixel = 0,
                   std::optional<int> delay = std::nullopt)
  {
    gifAddFrameImpl(key, region, width_px, dbu_per_pixel, delay);
  }
  virtual void deleteLabel(std::string_view label_name) = 0;
  virtual void gifEnd(int key) = 0;
  virtual void setDisplayControl(std::string_view name, bool value) = 0;

 protected:
  virtual void cellPlotImpl(bool pause) = 0;

  virtual void addFrameLabelImpl(const odb::Rect& bbox,
                                 std::string_view label,
                                 std::string_view label_name,
                                 int image_width_px)
      = 0;
  virtual void saveLabeledImageImpl(std::string_view path,
                                    std::string_view label,
                                    bool select_buffers,
                                    std::string_view heatmap_control,
                                    int image_width_px)
      = 0;
  virtual void gifAddFrameImpl(int key,
                               const odb::Rect& region,
                               int width_px,
                               double dbu_per_pixel,
                               std::optional<int> delay)
      = 0;
};

}  // namespace gpl
