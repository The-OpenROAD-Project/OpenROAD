// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "gui/heatMap.h"

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
class Graphics : public gui::Renderer, public gui::HeatMapDataSource
{
 public:
  using LineSeg = std::pair<odb::Point, odb::Point>;
  using LineSegs = std::vector<LineSeg>;

  // Debug mbff
  Graphics(utl::Logger* logger);

  // Debug InitialPlace
  Graphics(utl::Logger* logger,
           std::shared_ptr<PlacerBaseCommon> pbc,
           std::vector<std::shared_ptr<PlacerBase>>& pbVec);

  // Debug NesterovPlace
  Graphics(utl::Logger* logger,
           NesterovPlace* np,
           std::shared_ptr<PlacerBaseCommon> pbc,
           std::shared_ptr<NesterovBaseCommon> nbc,
           std::vector<std::shared_ptr<PlacerBase>>& pbVec,
           std::vector<std::shared_ptr<NesterovBase>>& nbVec,
           bool draw_bins,
           odb::dbInst* inst,
           int start_iter);

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false);

  // Draw the MBFF mapping
  void mbffMapping(const LineSegs& segs);
  void mbffFlopClusters(const std::vector<odb::dbInst*>& ffs);

  // Show a message in the status bar
  void status(const std::string& message);

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

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
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

  void drawForce(gui::Painter& painter);
  void drawCells(const std::vector<GCell*>& cells, gui::Painter& painter);
  void drawCells(const std::vector<GCellHandle>& cells, gui::Painter& painter);
  void drawSingleGCell(const GCell* gCell, gui::Painter& painter);

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  NesterovPlace* np_ = nullptr;
  GCell* selected_ = nullptr;
  bool draw_bins_ = false;
  utl::Logger* logger_ = nullptr;
  HeatMapType heatmap_type_ = Density;
  LineSegs mbff_edges_;
  std::vector<odb::dbInst*> mbff_cluster_;
  Mode mode_;
  int start_iter_ = 0;

  void initHeatmap();
  void drawNesterov(gui::Painter& painter);
  void drawInitial(gui::Painter& painter);
  void drawMBFF(gui::Painter& painter);
  void drawBounds(gui::Painter& painter);
  void reportSelected();
};

}  // namespace gpl
