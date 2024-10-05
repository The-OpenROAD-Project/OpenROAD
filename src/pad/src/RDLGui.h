/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

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

  void pause(bool timeout) const;

 private:
  RDLRouter* router_ = nullptr;

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
