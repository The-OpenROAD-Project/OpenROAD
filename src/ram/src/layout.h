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

class Cell {
public:
  Cell();

  Cell(odb::Point position, odb::dbOrientType orient);

  void addInst(odb::dbInst* inst);

  void cellInit();

  void placeCell();

  void setOrient(odb::dbOrientType orient);

  void setOrigin(odb::Point position);

  const int getHeight();

  const int getWidth();

private:
  odb::Point origin_;
  odb::dbOrientType orient_;
  int height;
  int width;
  std::vector<odb::dbInst*> insts_;
};

class Layout {
public:
  Layout(odb::Orientation2D orientation);

  Layout(odb::Orientation2D orientation, odb::Point origin);

  void addCell(std::unique_ptr<Cell> cell);

  void layoutInit();

  void placeLayout();

  void setOrigin(odb::Point position);

  const int getHeight();

  const int getWidth();
private:
  odb::Orientation2D orientation_;
  odb::Point origin_;
  int cell_height;
  int cell_width;
  std::vector<std::unique_ptr<Cell>> cells_;
};

class Grid {
public:

  Grid(odb::Orientation2D orientation);

  Grid(odb::Orientation2D orientation, int tracks);

  Grid(odb::Orientation2D orientation, odb::Point origin);

  void addLayout(std::unique_ptr<Layout> layout);

  void addCell(std::unique_ptr<Cell> cell, int track);

  void gridInit();

  void placeGrid();

  void setOrigin(odb::Point position);

  const int getHeight();

  const int getWidth();

  const int numLayouts();

  const int getRowWidth();
private:
  odb::Orientation2D orientation_;
  odb::Point origin_;
  int cell_height;
  int cell_width;
  std::vector<std::unique_ptr<Layout>> layouts_;



};



}  // namespace ram
