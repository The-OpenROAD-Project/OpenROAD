// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "gui/gui.h"
#include "shape.h"
#include "via.h"

namespace odb {
class Rect;
}  // namespace odb

namespace pdn {

class PdnGen;

// renderer for debugging, not intended for general use.
class PDNRenderer : public gui::Renderer
{
 public:
  explicit PDNRenderer(PdnGen* pdn);

  void update();

  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;
  void drawObjects(gui::Painter& painter) override;

  const char* getDisplayControlGroupName() override { return "Power Grid"; }

  void pause();

 private:
  PdnGen* pdn_;
  Shape::ShapeTreeMap shapes_;
  Shape::ObstructionTreeMap grid_obstructions_;
  Shape::ObstructionTreeMap initial_obstructions_;
  Via::ViaTree vias_;
  struct RepairChannel
  {
    odb::dbTechLayer* source;
    odb::dbTechLayer* target;
    odb::Rect rect;
    odb::Rect available_rect;
    std::string text;
  };
  std::vector<RepairChannel> repair_;

  static const gui::Painter::Color ring_color_;
  static const gui::Painter::Color strap_color_;
  static const gui::Painter::Color followpin_color_;
  static const gui::Painter::Color via_color_;
  static const gui::Painter::Color obstruction_color_;
  static const gui::Painter::Color repair_color_;
  static const gui::Painter::Color repair_outline_color_;

  static constexpr const char* grid_obs_text_ = "Grid obstructions";
  static constexpr const char* initial_obs_text_ = "Initial obstructions";
  static constexpr const char* obs_text_ = "Obstructions";
  static constexpr const char* rings_text_ = "Rings";
  static constexpr const char* straps_text_ = "Straps";
  static constexpr const char* followpins_text_ = "Followpin";
  static constexpr const char* vias_text_ = "Vias";
  static constexpr const char* repair_text_ = "Repair channels";
};

}  // namespace pdn
