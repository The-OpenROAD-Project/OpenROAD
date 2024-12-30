/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "AbstractIOPlacerRenderer.h"
#include "SimulatedAnnealing.h"
#include "gui/gui.h"
#include "ppl/IOPlacer.h"

namespace ppl {

class IOPlacerRenderer : public gui::Renderer, public AbstractIOPlacerRenderer
{
 public:
  IOPlacerRenderer();
  ~IOPlacerRenderer() override;
  void setCurrentIteration(const int& current_iteration) override;
  void setPaintingInterval(const int& painting_interval) override;
  void setPinAssignment(const std::vector<IOPin>& assignment) override;
  void setSinks(const std::vector<std::vector<InstancePin>>& sinks) override;
  void setIsNoPauseMode(const bool& is_no_pause_mode) override;

  void redrawAndPause() override;

  void drawObjects(gui::Painter& painter) override;

 private:
  bool isDrawingNeeded() const;

  std::vector<ppl::IOPin> pin_assignment_;
  std::vector<std::vector<InstancePin>> sinks_;

  int painting_interval_;
  int current_iteration_;
  bool is_no_pause_mode_;
};

}  // namespace ppl
