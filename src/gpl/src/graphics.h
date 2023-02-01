///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>

#include "gui/gui.h"
#include "gui/heatMap.h"

namespace utl {
class Logger;
}

namespace gpl {

class InitialPlace;
class NesterovBase;
class NesterovPlace;
class PlacerBase;
class GCell;

// This class draws debugging graphics on the layout
class Graphics : public gui::Renderer, public gui::HeatMapDataSource
{
 public:
  // Debug InitialPlace
  Graphics(utl::Logger* logger, std::shared_ptr<PlacerBase> pb);

  // Debug NesterovPlace
  Graphics(utl::Logger* logger,
           NesterovPlace* np,
           std::shared_ptr<PlacerBase> pb,
           std::shared_ptr<NesterovBase> nb,
           bool draw_bins,
           odb::dbInst* inst);

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  virtual gui::SelectionSet select(odb::dbTechLayer* layer,
                                   const odb::Rect& region) override;

  // From HeatMapDataSource
  virtual bool canAdjustGrid() const override { return false; }
  virtual double getGridXSize() const override;
  virtual double getGridYSize() const override;
  virtual odb::Rect getBounds() const override;
  virtual bool populateMap() override;
  virtual void combineMapData(bool base_has_value,
                              double& base,
                              const double new_data,
                              const double data_area,
                              const double intersection_area,
                              const double rect_area) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  enum HeatMapType
  {
    Density,
    Overflow
  };

  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  NesterovPlace* np_;
  GCell* selected_;
  bool draw_bins_;
  utl::Logger* logger_;
  HeatMapType heatmap_type_;

  void initHeatmap();
  void drawNesterov(gui::Painter& painter);
  void drawInitial(gui::Painter& painter);
  void drawBounds(gui::Painter& painter);
  void reportSelected();
};

}  // namespace gpl
