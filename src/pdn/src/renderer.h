///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

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
