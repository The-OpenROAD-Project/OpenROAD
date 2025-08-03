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

Cell::Cell(odb::dbInst* inst) {
 if (inst){
   insts_.push_back(inst);
 }
}


void Cell::setOrigin(odb::Point global_pos) {
 origin_ = global_pos;
}

void Cell::setInstPosition() {
 bbox = Rect(origin_, origin_);
 Point global_pos = origin_;
 for (auto& inst: insts_) {
    inst->setLocation(global_pos.getX(), global_pos.getY());
    Rect inst_rect = inst->getBBox()->getBox();
    bbox.merge(inst_rect);
    global_pos = bbox.lr();
 }
}

Rect Cell::placeCell () {
 for (auto& inst : insts_){
   inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
 }
 return bbox;
}

void Cell::addInst (dbInst* inst) {
 insts_.push_back(inst);
}

int Cell::getWidth() {
 return bbox.dx();
}

int Cell::getHeight() {
 return bbox.dy();
}

CellLayout::CellLayout(odb::Orientation2D orientation) : 
	orientation_(orientation) {}

void CellLayout::setOrigin(odb::Point position) {
 origin_ = position;
}

odb::Rect CellLayout::placeLayout() {
 bbox = Rect(origin_, origin_);
 Point global_pos = origin_;
 for (auto& cell : cells_) {
   cell->setOrigin(global_pos);
   cell->setInstPosition();
   bbox.merge(cell->placeCell());
   if (orientation_ == odb::vertical) {
     global_pos = bbox.ul();
   } else {
     global_pos = bbox.lr();
   }
 }

 return bbox;
}

void CellLayout::addCell (std::unique_ptr<Cell> cell) {
  cells_.push_back(std::move(cell));
}

int CellLayout::getWidth() {
 if (orientation_ == odb::horizontal) {
   return bbox.dx() / cells_.size();
 } else {
   return bbox.dx();
 }
}

int CellLayout::getHeight() {
 if (orientation_ == odb::vertical) {
   return bbox.dy() / cells_.size();
 } else {
   return bbox.dy();
 }
}


}  // namespace ram
