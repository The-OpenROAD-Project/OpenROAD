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

#pragma once

#include <memory>

#include "odb/db.h"

namespace ram {

////////////////////////////////////////////////////////////////

class Layout;

class Element
{
 public:
  Element(odb::dbInst* inst);
  Element(std::unique_ptr<Layout> layout);

  // Return the bbox of the positioned element
  odb::Rect position(odb::Point origin);

 private:
  odb::dbInst* inst_ = nullptr;
  std::unique_ptr<Layout> layout_;
};

class Layout
{
 public:
  Layout(odb::Orientation2D orientation);

  void addElement(std::unique_ptr<Element> element);

  // Return the bbox of the positioned layout
  odb::Rect position(odb::Point origin, int offset = 0);

  std::vector<std::unique_ptr<Element>>& getElements ();

 private:
  odb::Orientation2D orientation_;
  std::vector<std::unique_ptr<Element>> elements_;
};

class Cell {
public:

 Cell(odb::Point origin, int cell_width = 0, int cell_height = 0);

 int getWidth();

 int getHeight();

 void setWidth(int width);

 void  setHeight(int height);

 odb::Point getOrigin();

 void setOrigin(odb::Point origin);

 void addElement(std::unique_ptr<Element> element);

 odb::Rect placeCell(odb::Point place);



private:
 odb::Point origin_;
 std::vector<std::unique_ptr<Element>> elements_;
 int cell_width_;
 int cell_height_;

};

class GridLayout {
 public: 
  GridLayout(odb::Orientation2D orientation);

  void addLayout(std::unique_ptr<Layout> track);

  odb::Rect placeGrid(odb::Point origin);

  int getCellWidth();

  void setCellWidth(int new_width);

 private:
  std::vector<std::unique_ptr<Layout>> grid_;
  int cell_width_;
  odb::Orientation2D orientation_; //direction in which layout is built


};
}  // namespace ram
