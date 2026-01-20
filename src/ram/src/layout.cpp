// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "layout.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "ram/ram.h"
#include "utl/Logger.h"

namespace ram {

using odb::dbInst;
using odb::dbOrientType;
using odb::Point;
using odb::Rect;

//////////////////////////////////////////////////////////////

Cell::Cell(const Point& position, dbOrientType orient)
    : origin_(position), orient_(orient)
{
}

void Cell::addInst(dbInst* inst)
{
  insts_.push_back(inst);
}

void Cell::cellInit()
{
  width_ = 0;
  height_ = 0;
  for (auto& inst : insts_) {
    Rect inst_box = inst->getBBox()->getBox();
    width_ += inst_box.dx();
    height_ = inst_box.dy();
    inst->setOrient(orient_);
  }
}

void Cell::placeCell()
{
  Point global_pos = origin_;
  for (auto& inst : insts_) {
    inst->setLocation(global_pos.getX(), global_pos.getY());
    Rect inst_box = inst->getBBox()->getBox();
    global_pos.addX(inst_box.dx());
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}

void Cell::setOrient(odb::dbOrientType orient)
{
  orient_ = orient;
}

void Cell::setOrigin(const Point& position)
{
  origin_ = position;
}

int Cell::getHeight() const
{
  return height_;
}

int Cell::getWidth() const
{
  return width_;
}

/////////////////////////////////////////////////////////////

Layout::Layout(odb::Orientation2D orientation) : orientation_(orientation)
{
}

Layout::Layout(odb::Orientation2D orientation, Point origin)
    : orientation_(orientation), origin_(origin)
{
}

void Layout::addCell(std::unique_ptr<Cell> cell)
{
  cells_.push_back(std::move(cell));
}

void Layout::layoutInit()
{
  for (int i = 0; i < cells_.size(); ++i) {
    if (cells_[i]) {
      // set orientation first before initializing
      // need something to flip entire track if horizontal
      if (i % 2 == 1) {
        cells_[i]->setOrient(odb::dbOrientType::MX);
      }
      cells_[i]->cellInit();
      cell_height_ = std::max(cell_height_, cells_[i]->getHeight());
      cell_width_ = std::max(cell_width_, cells_[i]->getWidth());
    }
  }
}

void Layout::placeLayout()
{
  Point position = origin_;
  for (auto& cell : cells_) {
    if (cell) {
      cell->setOrigin(position);
      cell->placeCell();
    }
    if (orientation_ == odb::vertical) {
      position.addY(cell_height_);
    } else {
      position.addX(cell_width_);
    }
  }
}

void Layout::setOrigin(const odb::Point& position)
{
  origin_ = position;
}

int Layout::getHeight() const
{
  return cell_height_;
}

int Layout::getWidth() const
{
  return cell_width_;
}

//////////////////////////////////////////////////////////////

Grid::Grid(odb::Orientation2D orientation) : orientation_(orientation)
{
}

Grid::Grid(odb::Orientation2D orientation, int tracks)
    : orientation_(orientation)
{
  for (int i = 0; i < tracks; ++i) {
    if (orientation_ == odb::horizontal) {
      layouts_.push_back(std::make_unique<Layout>(odb::vertical));
    }
  }
}

Grid::Grid(odb::Orientation2D orientation, Point origin)
    : orientation_(orientation), origin_(origin)
{
}

void Grid::addLayout(std::unique_ptr<Layout> layout)
{
  layouts_.push_back(std::move(layout));
}

bool Grid::insertLayout(std::unique_ptr<Layout> layout, int index)
{
  if (index == layouts_.size()) {
    layouts_.push_back(std::move(layout));
  } else if (index < layouts_.size()) {
    layouts_.insert(layouts_.begin() + index, std::move(layout));
  } else if (index > layouts_.size()) {
    return false;
  }
  return true;
}

void Grid::addCell(std::unique_ptr<Cell> cell, int track)
{
  if (track >= layouts_.size()) {
    for (int size = layouts_.size(); size <= track; ++size) {
      if (orientation_ == odb::horizontal) {
        layouts_.push_back(std::make_unique<Layout>(odb::vertical));
      } else {
        layouts_.push_back(std::make_unique<Layout>(odb::horizontal));
      }
    }
  }
  layouts_[track]->addCell(std::move(cell));
}

void Grid::gridInit()
{
  for (auto& layout : layouts_) {
    layout->layoutInit();
  }

  // sets the height and width of the storage + read ports
  cell_height_ = layouts_[0]->getHeight();
  cell_width_ = layouts_[0]->getWidth();
}

void Grid::placeGrid()
{
  Point position = origin_;
  for (auto& layout : layouts_) {
    layout->setOrigin(position);
    layout->placeLayout();

    if (orientation_ == odb::horizontal) {
      position.addX(layout->getWidth());
    } else {
      position.addX(layout->getHeight());
    }
  }
}

void Grid::setOrigin(odb::Point position)
{
  origin_ = position;
}

int Grid::getHeight() const
{
  return cell_height_;
}

int Grid::getWidth() const
{
  return cell_width_;
}

int Grid::numLayouts() const
{
  return layouts_.size();
}

int Grid::getLayoutWidth(int index) const
{
  return layouts_[index]->getWidth();
}

int Grid::getLayoutHeight(int index) const
{
  return layouts_[index]->getHeight();
}

int Grid::getRowWidth() const
{
  int row_width = 0;
  for (auto& layout : layouts_) {
    row_width += layout->getWidth();
  }
  return row_width;
}

}  // namespace ram
