// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "Graphics.h"

#include "dpl/Opendp.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"

namespace dpl {

using odb::dbBox;

Graphics::Graphics(Opendp* dp,
                   float min_displacement,
                   const dbInst* debug_instance)
    : dp_(dp),
      debug_instance_(debug_instance),
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

void Graphics::binSearch(const Node* cell,
                         GridX xl,
                         GridY yl,
                         GridX xh,
                         GridY yh)
{
  if (!debug_instance_ || cell->getDbInst() != debug_instance_) {
    return;
  }
  Rect core = dp_->grid_->getCore();
  int xl_dbu = core.xMin() + gridToDbu(xl, dp_->grid_->getSiteWidth()).v;
  int yl_dbu = core.yMin() + dp_->grid_->gridYToDbu(yl).v;
  int xh_dbu = core.xMin() + gridToDbu(xh, dp_->grid_->getSiteWidth()).v;
  int yh_dbu = core.yMin() + dp_->grid_->gridYToDbu(yh).v;
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

  for (const auto& cell : dp_->network_->getNodes()) {
    if (!cell->isPlaced()) {
      continue;
    }
    // Compare the squared distances to save calling sqrt
    float min_length = min_displacement_ * dp_->grid_->gridHeight(cell.get()).v;
    min_length *= min_length;
    DbuX lx{core.xMin() + cell->getLeft()};
    DbuY ly{core.yMin() + cell->getBottom()};

    auto color = cell->getDbInst() ? gui::Painter::gray : gui::Painter::red;
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawRect(Rect(
        lx.v, ly.v, lx.v + cell->getWidth().v, ly.v + cell->getHeight().v));

    if (!cell->getDbInst()) {
      continue;
    }

    dbBox* bbox = cell->getDbInst()->getBBox();
    Point initial_location(bbox->xMin(), bbox->yMin());
    Point final_location(lx.v, ly.v);
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
