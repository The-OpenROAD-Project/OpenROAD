// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "Graphics.h"

#include <any>
#include <cstdlib>
#include <set>

#include "dpl/Opendp.h"
#include "gui/gui.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

Graphics::Graphics(Opendp* dp,
                   float min_displacement,
                   const odb::dbInst* debug_instance)
    : dp_(dp),
      debug_instance_(debug_instance),
      min_displacement_(min_displacement)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::startPlacement(odb::dbBlock* block)
{
  block_ = block;
}

void Graphics::placeInstance(odb::dbInst* instance)
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
  odb::Rect core = dp_->grid_->getCore();
  int xl_dbu = core.xMin() + gridToDbu(xl, dp_->grid_->getSiteWidth()).v;
  int yl_dbu = core.yMin() + dp_->grid_->gridYToDbu(yl).v;
  int xh_dbu = core.xMin() + gridToDbu(xh, dp_->grid_->getSiteWidth()).v;
  int yh_dbu = core.yMin() + dp_->grid_->gridYToDbu(yh).v;
  searched_.emplace_back(xl_dbu, yl_dbu, xh_dbu, yh_dbu);
}

void Graphics::redrawAndPause()
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

  // Create a set of selected instances for fast lookup
  std::set<odb::dbInst*> selected_insts;
  auto selection = gui::Gui::get()->selection();
  for (const auto& selected : selection) {
    if (selected.isInst()) {
      selected_insts.insert(std::any_cast<odb::dbInst*>(selected.getObject()));
    }
  }

  for (const auto& cell : dp_->network_->getNodes()) {
    if (!cell->isPlaced() || !cell->getDbInst()) {
      continue;
    }

    odb::Point initial_location = dp_->getOdbLocation(cell.get());
    odb::Point final_location = dp_->getDplLocation(cell.get());
    float len = odb::Point::squaredDistance(initial_location, final_location);
    if (len <= 0) {
      continue;
    }

    int dx = final_location.x() - initial_location.x();
    int dy = final_location.y() - initial_location.y();
    gui::Painter::Color line_color;

    // Check if the instance is selected
    if (selected_insts.contains(cell->getDbInst())) {
      line_color = gui::Painter::kYellow;
    } else if (std::abs(dx) > std::abs(dy)) {
      line_color = (dx > 0) ? gui::Painter::kGreen : gui::Painter::kRed;
    } else {
      line_color = (dy > 0) ? gui::Painter::kMagenta : gui::Painter::kBlue;
    }

    painter.setPen(line_color, /* cosmetic */ true);
    painter.setBrush(line_color);
    painter.drawLine(initial_location.x(),
                     initial_location.y(),
                     final_location.x(),
                     final_location.y());
    painter.drawCircle(final_location.x(), final_location.y(), 100);
  }

  auto color = gui::Painter::kCyan;
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
