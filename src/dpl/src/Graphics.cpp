/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021, The Regents of the University of California
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
///////////////////////////////////////////////////////////////////////////////

#include "Graphics.h"

#include "dpl/Opendp.h"

namespace dpl {

using odb::dbBox;

Graphics::Graphics(Opendp* dp,
                   float min_displacement,
                   const dbInst* debug_instance)
    : dp_(dp),
      debug_instance_(debug_instance),
      block_(nullptr),
      min_displacement_(min_displacement)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::startPlacement(dbBlock* block)
{
  block_ = block;
}

void Graphics::placeInstance(dbInst* instance)
{
  if (!instance || instance != debug_instance_) {
    return;
  }

  auto gui = gui::Gui::get();

  auto selected = gui->makeSelected(instance);
  gui->setSelected(selected);
  gui->redraw();
  gui->pause();
}

void Graphics::binSearch(const Cell* cell, int xl, int yl, int xh, int yh)
{
  if (!debug_instance_ || cell->db_inst_ != debug_instance_) {
    return;
  }
  Rect core = dp_->getCore();
  int xl_dbu = core.xMin() + xl * dp_->getSiteWidth();
  int yl_dbu = core.yMin() + yl * dp_->getRowHeight(cell);
  int xh_dbu = core.xMin() + xh * dp_->getSiteWidth();
  int yh_dbu = core.yMin() + yh * dp_->getRowHeight(cell);
  searched_.emplace_back(xl_dbu, yl_dbu, xh_dbu, yh_dbu);
}

void Graphics::endPlacement()
{
  auto gui = gui::Gui::get();
  gui->redraw();
  gui->pause();
}

void Graphics::drawObjects(gui::Painter& painter)
{
  if (!block_) {
    return;
  }

  odb::Rect core = block_->getCoreArea();

  for (const auto& cell : dp_->getCells()) {
    if (!cell.is_placed_) {
      continue;
    }
    // Compare the squared distances to save calling sqrt
    float min_length = min_displacement_ * dp_->getRowHeight(&cell);
    min_length *= min_length;
    int lx = core.xMin() + cell.x_;
    int ly = core.yMin() + cell.y_;

    auto color = cell.db_inst_ ? gui::Painter::gray : gui::Painter::red;
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawRect(Rect(lx, ly, lx + cell.width_, ly + cell.height_));

    if (!cell.db_inst_) {
      continue;
    }

    dbBox* bbox = cell.db_inst_->getBBox();
    Point initial_location(bbox->xMin(), bbox->yMin());
    Point final_location(lx, ly);
    float len = Point::squaredDistance(initial_location, final_location);
    if (len < min_length) {
      continue;
    }

    painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
    painter.drawLine(initial_location.x(),
                     initial_location.y(),
                     final_location.x(),
                     final_location.y());
    painter.drawCircle(final_location.x(), final_location.y(), 100);
  }

  auto color = gui::Painter::cyan;
  painter.setPen(color);
  painter.setBrush(color);
  for (auto& rect : searched_) {
    painter.drawRect(rect);
  }
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}
}  // namespace dpl
