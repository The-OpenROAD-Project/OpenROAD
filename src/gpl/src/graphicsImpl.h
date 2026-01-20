// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AbstractGraphics.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "routeBase.h"

namespace utl {
class Logger;
}

namespace gpl {

class InitialPlace;
class NesterovBaseCommon;
class NesterovBase;
class NesterovPlace;
class PlacerBaseCommon;
class PlacerBase;
class GCell;
class GCellHandle;

// This class draws debugging graphics on the layout
class GraphicsImpl : public gpl::AbstractGraphics,
                     public gui::Renderer,
                     public gui::HeatMapDataSource
{
 public:
  using LineSeg = std::pair<odb::Point, odb::Point>;
  using LineSegs = std::vector<LineSeg>;

  GraphicsImpl(utl::Logger* logger);
  ~GraphicsImpl() override;

  std::unique_ptr<AbstractGraphics> MakeNew(utl::Logger* logger) const override;

  void debugForMbff() override;
  void debugForInitialPlace(
      std::shared_ptr<PlacerBaseCommon> pbc,
      std::vector<std::shared_ptr<PlacerBase>>& pbVec) override;
  void debugForNesterovPlace(NesterovPlace* np,
                             std::shared_ptr<PlacerBaseCommon> pbc,
                             std::shared_ptr<NesterovBaseCommon> nbc,
                             std::shared_ptr<RouteBase> rb,
                             std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                             std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                             bool draw_bins,
                             odb::dbInst* inst) override;

  void addIter(int iter, double overflow) override;
  void addTimingDrivenIter(int iter) override;
  void addRoutabilitySnapshot(int iter) override;
  void addRoutabilityIter(int iter, bool revert) override;

  void mbffMapping(const LineSegs& segs) override;
  void mbffFlopClusters(const std::vector<odb::dbInst*>& ffs) override;

  void status(std::string_view message) override;

  bool enabled() override;

  void setDebugOn(bool set_on) override { debug_on_ = set_on; }

  void setDisplayControl(std::string_view name, bool value) override;

  int gifStart(std::string_view path) override;
  void deleteLabel(std::string_view label_name) override;
  void gifEnd(int key) override;

 protected:
  void cellPlotImpl(bool pause) override;

  void addFrameLabelImpl(const odb::Rect& bbox,
                         std::string_view label,
                         std::string_view label_name,
                         int image_width_px) override;
  void saveLabeledImageImpl(std::string_view path,
                            std::string_view label,
                            bool select_buffers,
                            std::string_view heatmap_control,
                            int image_width_px) override;
  void gifAddFrameImpl(int key,
                       const odb::Rect& region,
                       int width_px,
                       double dbu_per_pixel,
                       std::optional<int> delay) override;

 private:
  // From Renderer API
  void drawObjects(gui::Painter& painter) override;
  gui::SelectionSet select(odb::dbTechLayer* layer,
                           const odb::Rect& region) override;

  // From HeatMapDataSource
  bool canAdjustGrid() const override { return false; }
  double getGridXSize() const override;
  double getGridYSize() const override;
  odb::Rect getBounds() const override;
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;
  void populateXYGrid() override;

  enum HeatMapType
  {
    Density,
    Overflow,
    OverflowMinMax
  };

  enum Mode
  {
    Mbff,
    Initial,
    Nesterov
  };

  // These are used for coloring each instance based on its group
  std::vector<gui::Painter::Color> instances_colors_ = {
      gui::Painter::kDarkGreen,
      gui::Painter::kDarkBlue,
      gui::Painter::kBrown,
      gui::Painter::kDarkYellow,
  };

  // These are used for bin forces, fillers, and dummies (lighter) for each
  // region.
  std::vector<gui::Painter::Color> region_colors_ = {
      gui::Painter::kDarkMagenta,
      gui::Painter::kYellow,
      gui::Painter::kBlue,
      gui::Painter::kCyan,

  };

  void drawForce(gui::Painter& painter);
  void drawCells(const std::vector<GCell*>& cells, gui::Painter& painter);
  void drawCells(const std::vector<GCellHandle>& cells,
                 gui::Painter& painter,
                 size_t nb_index);
  void drawSingleGCell(const GCell* gCell,
                       gui::Painter& painter,
                       size_t nb_index = 0);

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::shared_ptr<RouteBase> rb_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  NesterovPlace* np_ = nullptr;
  static constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();
  size_t selected_ = kInvalidIndex;
  size_t nb_selected_index_ = kInvalidIndex;
  bool draw_bins_ = false;
  utl::Logger* logger_ = nullptr;
  HeatMapType heatmap_type_ = Density;
  LineSegs mbff_edges_;
  std::vector<odb::dbInst*> mbff_cluster_;
  Mode mode_;
  gui::Chart* main_chart_{nullptr};
  gui::Chart* density_chart_{nullptr};
  gui::Chart* stepLength_chart_{nullptr};
  gui::Chart* routing_chart_{nullptr};
  bool debug_on_{false};

  void initHeatmap();
  void drawNesterov(gui::Painter& painter);
  void drawInitial(gui::Painter& painter);
  void drawMBFF(gui::Painter& painter);
  void drawBounds(gui::Painter& painter);
  void reportSelected();
};

}  // namespace gpl
