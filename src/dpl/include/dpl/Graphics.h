/*
 * Copyright (c) 2021, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "gui/gui.h"
#include "odb/db.h"

namespace dpl {

class Opendp;
struct Cell;

// Decorates the gui::Renderer with DPL specific routines.
class Graphics : public gui::Renderer
{
 public:
  Graphics(Opendp* dp,
           float min_displacement,
           const odb::dbInst* debug_instance);

  virtual void startPlacement(odb::dbBlock* block);
  virtual void placeInstance(odb::dbInst* instance);
  virtual void binSearch(const Cell* cell, int xl, int yl, int xh, int yh);
  virtual void endPlacement();

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive() { return gui::Gui::enabled(); }

  static std::unique_ptr<Graphics> makeDplGraphics(
      Opendp* opendp,
      float min_displacement,
      const odb::dbInst* debug_instance)
  {
    return std::make_unique<Graphics>(opendp, min_displacement, debug_instance);
  }

 private:
  Opendp* dp_;
  const odb::dbInst* debug_instance_;
  odb::dbBlock* block_;
  float min_displacement_;  // in row height
  std::vector<odb::Rect> searched_;
};

}  // namespace dpl
