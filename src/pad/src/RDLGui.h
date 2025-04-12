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
  gui::Painter::Color snap_color_ = gui::Painter::white;

  static constexpr const char* draw_vertex_ = "Vertices";
  static constexpr const char* draw_edge_ = "Edges";
  static constexpr const char* draw_obs_ = "Obstructions";
  static constexpr const char* draw_fly_wires_ = "Routing fly wires";
  static constexpr const char* draw_targets_ = "Targets";
  static constexpr const char* draw_routes_ = "Routes";
  static constexpr const char* draw_route_obstructions_ = "Route obstructions";

  static constexpr int gui_timeout_ = 100;
};

}  // namespace pad
