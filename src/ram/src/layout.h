// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

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
