// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "odb/db.h"
#include "shape.h"
#include "via.h"
#include "web/core.h"

namespace odb {
class Rect;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

class PdnGen;

// renderer for debugging, not intended for general use.
class PDNRenderer : public web::Renderer
{
 public:
  PDNRenderer(PdnGen* pdn, utl::Logger* logger);

  void update();

  void drawLayer(odb::dbTechLayer* layer, web::Painter& painter) override;
  void drawObjects(web::Painter& painter) override;

  const char* getDisplayControlGroupName() override { return "Power Grid"; }

  void setInitialObstructions(
      const Shape::ObstructionTreeMap& initial_obstructions)
  {
    initial_obstructions_ = initial_obstructions;
  }

  // Halt the flow so the current state can be inspected in the GUI.  The
  // reason is reported so the pause is actionable.
  void pause(const std::string& reason);

 private:
  PdnGen* pdn_;
  utl::Logger* logger_;
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

  static const web::Painter::Color kRingColor;
  static const web::Painter::Color kStrapColor;
  static const web::Painter::Color kFollowpinColor;
  static const web::Painter::Color kViaColor;
  static const web::Painter::Color kObstructionColor;
  static const web::Painter::Color kRepairColor;
  static const web::Painter::Color kRepairOutlineColor;

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
