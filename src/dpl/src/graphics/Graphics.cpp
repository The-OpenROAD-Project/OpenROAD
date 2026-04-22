// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "Graphics.h"

#include <algorithm>
#include <any>
#include <cstdlib>
#include <set>
#include <vector>

#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "gui/gui.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

Graphics::Graphics(Opendp* dp,
                   const odb::dbInst* debug_instance,
                   bool paint_pixels,
                   bool paint_negotiation_pixels)
    : dp_(dp),
      debug_instance_(debug_instance),
      paint_pixels_(paint_pixels),
      paint_negotiation_pixels_(paint_negotiation_pixels)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::startPlacement(odb::dbBlock* block)
{
  block_ = block;
}

void Graphics::drawSelected(odb::dbInst* instance, bool force)
{
  // When force is true always select and pause
  if (!instance || (!force && instance != debug_instance_)) {
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
    if (!cell->getDbInst()) {
      continue;
    }

    if (!cell->isPlaced()) {
      auto color = gui::Painter::kDarkMagenta;
      painter.setPen(color);
      painter.setBrush(color);
      odb::Rect bbox;
      bbox = cell->getDbInst()->getBBox()->getBox();
      painter.drawRect(bbox);
      continue;
    }

    if (cell->getDbInst()->isFixed()) {
      auto color = gui::Painter::kGray;
      color.a = 100;
      painter.setPen(color);
      painter.setBrush(color);
      odb::Rect bbox = cell->getDbInst()->getBBox()->getBox();
      painter.drawRect(bbox);
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

      // Draw outline of instance at target location
      odb::Rect bbox = cell->getDbInst()->getBBox()->getBox();
      int width = bbox.dx();
      int height = bbox.dy();
      odb::Rect target_bbox(final_location.x(),
                            final_location.y(),
                            final_location.x() + width,
                            final_location.y() + height);
      auto outline_color = gui::Painter::kCyan;
      // outline_color.a = 150;
      painter.setPen(outline_color, /* cosmetic */ true);
      painter.setBrush(gui::Painter::kTransparent);
      painter.drawRect(target_bbox);

      // Indicate orientation change at the target location with a corner notch
      // (mirroring the ODB orientation marker style)
      painter.setPen(outline_color, /* cosmetic */ true);
      const int tag_size = std::min(width / 4, height / 8);
      painter.drawLine(target_bbox.xMin() + tag_size,
                       target_bbox.yMin(),
                       target_bbox.xMin(),
                       target_bbox.yMin() + tag_size * 2);
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

  // Diamond search range
  auto color = gui::Painter::kCyan;
  color.a = 100;
  painter.setPen(color);
  painter.setBrush(color);
  for (auto& rect : searched_) {
    painter.drawRect(rect);
  }

  if (paint_pixels_) {
    const Grid* grid = dp_->grid_.get();
    if (grid) {
      const odb::Rect core = grid->getCore();
      const DbuX site_width = grid->getSiteWidth();

      auto color = gui::Painter::kWhite;
      color.a = 100;
      painter.setPen(color);
      painter.setBrush(color);

      for (GridY y{0}; y < grid->getRowCount(); y++) {
        const DbuY y_dbu = grid->gridYToDbu(y);
        const DbuY next_y_dbu = grid->gridYToDbu(y + 1);
        for (GridX x{0}; x < grid->getRowSiteCount(); x++) {
          const Pixel& pixel = grid->pixel(y, x);
          if (pixel.cell != nullptr) {
            const DbuX x_dbu = gridToDbu(x, site_width);
            const DbuX next_x_dbu = gridToDbu(x + 1, site_width);

            odb::Rect rect(core.xMin() + x_dbu.v,
                           core.yMin() + y_dbu.v,
                           core.xMin() + next_x_dbu.v,
                           core.yMin() + next_y_dbu.v);
            painter.drawRect(rect);
          }
        }
      }
    }
  }

  if (paint_negotiation_pixels_ && !negotiation_pixels_.empty()) {
    const int sw = negotiation_site_width_;
    for (int gy = 0; gy < negotiation_grid_h_; ++gy) {
      const int y_lo = negotiation_die_ylo_ + negotiation_row_y_dbu_[gy];
      const int y_hi = negotiation_die_ylo_ + negotiation_row_y_dbu_[gy + 1];
      for (int gx = 0; gx < negotiation_grid_w_; ++gx) {
        const auto state = negotiation_pixels_[gy * negotiation_grid_w_ + gx];
        gui::Painter::Color c;
        switch (state) {
          case NegotiationPixelState::kNoRow:
            c = gui::Painter::kDarkGray;
            c.a = 60;
            break;
          case NegotiationPixelState::kFree:
            c = gui::Painter::kGreen;
            c.a = 100;
            break;
          case NegotiationPixelState::kOccupied:
            c = gui::Painter::kWhite;
            c.a = 100;
            break;
          case NegotiationPixelState::kOveruse:
            c = gui::Painter::kRed;
            c.a = 150;
            break;
          case NegotiationPixelState::kBlocked:
            c = gui::Painter::kYellow;
            c.a = 80;
            break;
          case NegotiationPixelState::kInvalid:
            c = gui::Painter::kBlack;
            c.a = 200;
            break;
          case NegotiationPixelState::kDrcViolation:
            c = gui::Painter::Color{255, 140, 0, 200};  // orange
            break;
        }
        painter.setPen(c);
        painter.setBrush(c);
        odb::Rect rect(negotiation_die_xlo_ + gx * sw,
                       y_lo,
                       negotiation_die_xlo_ + (gx + 1) * sw,
                       y_hi);
        painter.drawRect(rect);
      }
    }
  }

  if (!negotiation_search_windows_.empty()) {
    painter.setBrush(gui::Painter::kTransparent);
    for (const auto& sel : selection) {
      if (!sel.isInst()) {
        continue;
      }
      auto* inst = std::any_cast<odb::dbInst*>(sel.getObject());
      auto it = negotiation_search_windows_.find(inst);
      if (it == negotiation_search_windows_.end()) {
        continue;
      }
      const auto& [init_win, curr_win] = it->second;

      // Init-position search window
      auto init_color = gui::Painter::kCyan;
      painter.setPen(init_color, /* cosmetic */ true);
      painter.drawRect(init_win);

      // Current-position window (only when the cell is displaced).
      if (!curr_win.isInverted() && curr_win.area() > 0) {
        auto curr_color = gui::Painter::kWhite;
        curr_color.a = 200;
        painter.setPen(curr_color, /* cosmetic */ true);
        painter.drawRect(curr_win);
      }
    }
  }
}

void Graphics::setNegotiationPixels(
    const std::vector<NegotiationPixelState>& pixels,
    int grid_w,
    int grid_h,
    int die_xlo,
    int die_ylo,
    int site_width,
    const std::vector<int>& row_y_dbu)
{
  negotiation_pixels_ = pixels;
  negotiation_grid_w_ = grid_w;
  negotiation_grid_h_ = grid_h;
  negotiation_die_xlo_ = die_xlo;
  negotiation_die_ylo_ = die_ylo;
  negotiation_site_width_ = site_width;
  negotiation_row_y_dbu_ = row_y_dbu;
}

void Graphics::clearNegotiationPixels()
{
  negotiation_pixels_.clear();
  negotiation_row_y_dbu_.clear();
  negotiation_grid_w_ = 0;
  negotiation_grid_h_ = 0;
  negotiation_search_windows_.clear();
}

void Graphics::setNegotiationSearchWindow(odb::dbInst* inst,
                                          const odb::Rect& init_window,
                                          const odb::Rect& curr_window)
{
  negotiation_search_windows_[inst] = {init_window, curr_window};
}

void Graphics::clearNegotiationSearchWindows()
{
  negotiation_search_windows_.clear();
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}
}  // namespace dpl
