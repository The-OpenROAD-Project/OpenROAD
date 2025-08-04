/////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
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
///////////////////////////////////////////////////////////////////////////////

#include "layout.h"

#include "ram/ram.h"
#include "utl/Logger.h"

namespace ram {

using odb::dbBTerm;
using odb::dbInst;
using odb::dbNet;
using odb::Point;
using odb::Rect;

using utl::RAM;

////////////////////////////////////////////////////////////////

Element::Element(odb::dbInst* inst) : inst_(inst)
{
}

Element::Element(std::unique_ptr<Layout> layout) : layout_(std::move(layout))
{
}

Rect Element::position(Point origin)
{
  if (inst_) { 
    inst_->setLocation(origin.getX(), origin.getY());
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst_->getBBox()->getBox();
  }

  return layout_->position(origin);
}

////////////////////////////////////////////////////////////////

Layout::Layout(odb::Orientation2D orientation) : orientation_(orientation)
{
}
// this is the method that affects position without layout
Rect Layout::position(Point origin, int offset)
{
  Rect bbox(origin, origin);
  //layout logic, iterating over elements vector, moving origin accrodingly
  if (orientation_ == odb::horizontal) {
    for (auto& elem : elements_) {
      //position is in relation to the layout
      auto bounds = elem->position(origin);
      bbox.merge(bounds);
      origin = bbox.lr();
      origin.setX(origin.getX() + offset);
    }
  } else {
    for (auto& elem : elements_) {
      auto bounds = elem->position(origin);
      bbox.merge(bounds);
      origin = bbox.ul();
      origin.setY(origin.getY() + offset);
    }
  }
  return bbox;
}

std::vector<std::unique_ptr<Element>>& Layout::getElements() {
  return elements_; 
}

void Layout::addElement(std::unique_ptr<Element> element)
{
  elements_.push_back(std::move(element));
}

//////////////////////////////////////////////////////////////


Cell::Cell() : origin_(0,0) {}

Cell::Cell(Point position) : origin_(position) {}

void Cell::addInst(dbInst* inst){
   insts_.push_back(inst);
}

void Cell::cellInit() {
   for (auto& inst : insts_) {
      Rect inst_box = inst->getBBox()->getBox();
      width += inst_box.dx();
      height = inst_box.dy(); 
   }

}

void Cell::placeCell() {
   Point global_pos = origin_; 
  for (auto& inst : insts_) {
      inst->setLocation(global_pos.getX(), global_pos.getY());
      Rect inst_box = inst->getBBox()->getBox();
      global_pos.addX(inst_box.dx());
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

}

void Cell::setOrigin(Point position) {
   origin_ = position;
}

const int Cell::getHeight() {
   return height;
}

const int Cell::getWidth() {
   return width;
}

/////////////////////////////////////////////////////////////

CellLayout::CellLayout(odb::Orientation2D orientation) : orientation_ (orientation){}

CellLayout::CellLayout(odb::Orientation2D orientation, Point origin) : 
	orientation_(orientation), origin_(origin) {}

void CellLayout::addCell(std::unique_ptr<Cell> cell){
    cells_.push_back(std::move(cell));

}

void CellLayout::layoutInit() {
    for (auto& cell : cells_) {
       if (cell) {
          cell->cellInit();
       }
    }

    cell_height = cells_[0]->getHeight();
    cell_width = cells_[0]->getWidth();

}

void CellLayout::placeLayout() {
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

void CellLayout::setOrigin(odb::Point position) {
   origin_ = position;
}

const int CellLayout::getHeight() { 
   return cell_height; 
}

const int CellLayout::getWidth() {
   return cell_width; 
}

//////////////////////////////////////////////////////////////

Grid::Grid(odb::Orientation2D orientation) : orientation_(orientation) {}

Grid::Grid(odb::Orientation2D orientation, int tracks) : orientation_(orientation) {
   for (int i = 0; i < tracks; ++i) {
       if (orientation_ == odb::horizontal) {
           layouts_.push_back(std::make_unique<CellLayout> (odb::vertical));
       }
   }

}

Grid::Grid (odb::Orientation2D orientation, Point origin) : 
	orientation_(orientation), origin_(origin) {};

void Grid::addLayout(std::unique_ptr<CellLayout> layout) {
   layouts_.push_back(std::move(layout));

}

void Grid::addCell(std::unique_ptr<Cell> cell, int track) {
   if (track >= layouts_.size()) {
       for (int size = layouts_.size(); size <= track; ++size) {
           if (orientation_ == odb::horizontal){
	       layouts_.push_back(std::make_unique<CellLayout>(odb::vertical));
	   } else {
	       layouts_.push_back(std::make_unique<CellLayout>(odb::horizontal));
	   }
       
       }
   }
   layouts_[track]->addCell(std::move(cell));
}

void Grid::gridInit() {
   for (auto& layout : layouts_) {
      layout->layoutInit();
   }

   cell_height = layouts_[0]->getHeight();
   cell_width = layouts_[0]->getWidth();

}

void Grid::placeGrid() {
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

void Grid::setOrigin(odb::Point position) {
   origin_ = position;
}

const int Grid::getHeight() {
   return cell_height;
}

const int Grid::getWidth() {
   return cell_width;
}

const int Grid::numLayouts() {
   return layouts_.size();
}





}  // namespace ram
