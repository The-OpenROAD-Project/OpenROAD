// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
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

  void setInitialObstructions(
      const Shape::ObstructionTreeMap& initial_obstructions)
  {
    initial_obstructions_ = initial_obstructions;
  }

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

  static const gui::Painter::Color kRingColor;
  static const gui::Painter::Color kStrapColor;
  static const gui::Painter::Color kFollowpinColor;
  static const gui::Painter::Color kViaColor;
  static const gui::Painter::Color kObstructionColor;
  static const gui::Painter::Color kRepairColor;
  static const gui::Painter::Color kRepairOutlineColor;

  static constexpr const char* kGridObsText = "Grid obstructions";
  static constexpr const char* kInitialObsText = "Initial obstructions";
  static constexpr const char* kObsText = "Obstructions";
  static constexpr const char* kRingsText = "Rings";
  static constexpr const char* kStrapsText = "Straps";
  static constexpr const char* kFollowpinsText = "Followpin";
  static constexpr const char* kViasText = "Vias";
  static constexpr const char* kRepairText = "Repair channels";
};

}  // namespace pdn
