///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include "graphics.h"

#include "utility/Logger.h"

namespace mpl {

Graphics::Graphics(odb::dbDatabase* db, utl::Logger* logger)
    : partition_relative_coords_(false), db_(db), logger_(logger)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::drawObjects(gui::Painter& painter)
{
  odb::dbTech* tech = db_->getTech();
  const int dbu = tech->getDbUnitsPerMicron();

  painter.setPen(gui::Painter::white, /* cosmetic */ true);

  for (auto& partition : partitions_) {
    painter.setBrush(gui::Painter::transparent);
    int lx = partition.lx * dbu;
    int ly = partition.ly * dbu;
    int ux = (partition.lx + partition.width) * dbu;
    int uy = (partition.ly + partition.height) * dbu;
    painter.drawRect({lx, ly, ux, uy});

    // Macro coordinates are partition relative pre-anneal
    // but not post-anneal
    double x_offset = 0;
    double y_offset = 0;
    if (partition_relative_coords_) {
      x_offset = partition.lx;
      y_offset = partition.ly;
    }
    for (auto& macro : partition.macros_) {
      painter.setBrush(gui::Painter::gray);
      lx = (x_offset + macro.lx) * dbu;
      ly = (y_offset + macro.ly) * dbu;
      ux = (x_offset + macro.lx + macro.w) * dbu;
      uy = (y_offset + macro.ly + macro.h) * dbu;
      painter.drawRect({lx, ly, ux, uy});
    }
  }
}

void Graphics::set_partitions(const std::vector<Partition>& partitions,
                              bool partition_relative_coords)
{
  partitions_ = partitions;
  partition_relative_coords_ = partition_relative_coords;
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
}

void Graphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::get() != nullptr;
}

}  // namespace mpl
