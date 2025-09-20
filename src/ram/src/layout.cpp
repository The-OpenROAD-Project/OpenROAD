// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "layout.h"

#include "ram/ram.h"
#include "utl/Logger.h"

namespace ram {

using odb::dbBTerm;
using odb::dbInst;
using odb::dbNet;
using odb::dbOrientType;
using odb::Point;
using odb::Rect;

using utl::RAM;

//////////////////////////////////////////////////////////////

Cell::Cell() : origin_(0, 0), orient_(dbOrientType::R0)
{
}

Cell::Cell(Point position, dbOrientType orient)
    : origin_(position), orient_(orient)
{
}

void Cell::addInst(dbInst* inst)
{
  insts_.push_back(inst);
}

void Cell::cellInit()
{
  for (auto& inst : insts_) {
    Rect inst_box = inst->getBBox()->getBox();
    width += inst_box.dx();
    height = inst_box.dy();
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

void Cell::setOrigin(Point position)
{
  origin_ = position;
}

const int Cell::getHeight()
{
  return height;
}

const int Cell::getWidth()
{
  return width;
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
      if (cell_height < cells_[i]->getHeight()) {
        cell_height = cells_[i]->getHeight();
      }
      if (cell_width < cells_[i]->getWidth()) {
        cell_width = cells_[i]->getWidth();
      }
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
      position.addY(cell_height);
    } else {
      position.addX(cell_width);
    }
  }
}

void Layout::setOrigin(odb::Point position)
{
  origin_ = position;
}

const int Layout::getHeight()
{
  return cell_height;
}

const int Layout::getWidth()
{
  return cell_width;
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
    : orientation_(orientation), origin_(origin){};

void Grid::addLayout(std::unique_ptr<Layout> layout)
{
  layouts_.push_back(std::move(layout));
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
  cell_height = layouts_[0]->getHeight();
  cell_width = layouts_[0]->getWidth();
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

const int Grid::getHeight()
{
  return cell_height;
}

const int Grid::getWidth()
{
  return cell_width;
}

const int Grid::numLayouts()
{
  return layouts_.size();
}

const int Grid::getRowWidth()
{
  int row_width = 0;
  for (auto& layout : layouts_) {
    row_width += layout->getWidth();
  }
  return row_width;
}

}  // namespace ram
