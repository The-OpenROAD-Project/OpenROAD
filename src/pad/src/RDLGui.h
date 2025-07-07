// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <utility>

#include "gui/gui.h"

namespace pad {

class RDLRouter;

class RDLGui : public gui::Renderer
{
 public:
  RDLGui();
  ~RDLGui() override;

  void setRouter(RDLRouter* router);

  void drawObjects(gui::Painter& painter) override;

  const char* getDisplayControlGroupName() override { return "RDL Router"; }

  void clearSnap();
  void addSnap(const odb::Point& pt0, const odb::Point& pt1);
  void zoomToSnap(bool preview);

  void pause(bool timeout) const;

 private:
  RDLRouter* router_ = nullptr;

  std::set<std::pair<odb::Point, odb::Point>> snap_;
  gui::Painter::Color snap_color_ = gui::Painter::kWhite;

  static constexpr const char* kDrawVertex = "Vertices";
  static constexpr const char* kDrawEdge = "Edges";
  static constexpr const char* kDrawObs = "Obstructions";
  static constexpr const char* kDrawFlyWires = "Routing fly wires";
  static constexpr const char* kDrawTargets = "Targets";
  static constexpr const char* kDrawRoutes = "Routes";
  static constexpr const char* kDrawRouteObstructions = "Route obstructions";

  static constexpr int kGuiTimeout = 100;
};

}  // namespace pad
