// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

namespace ram {

////////////////////////////////////////////////////////////////

class Cell
{
 public:
  Cell() = default;

  Cell(const odb::Point& position, odb::dbOrientType orient);

  void addInst(odb::dbInst* inst);

  void cellInit();

  void placeCell();

  void setOrient(odb::dbOrientType orient);

  void setOrigin(const odb::Point& position);

  int getHeight() const;

  int getWidth() const;

 private:
  odb::Point origin_;
  odb::dbOrientType orient_;
  int height_{0};
  int width_{0};
  std::vector<odb::dbInst*> insts_;
};

class Layout
{
 public:
  Layout(odb::Orientation2D orientation);

  Layout(odb::Orientation2D orientation, odb::Point origin);

  void addCell(std::unique_ptr<Cell> cell);

  void layoutInit();

  void placeLayout();

  void setOrigin(const odb::Point& position);

  int getHeight() const;

  int getWidth() const;

 private:
  odb::Orientation2D orientation_;
  odb::Point origin_;
  int cell_height_{0};
  int cell_width_{0};
  std::vector<std::unique_ptr<Cell>> cells_;
};

class Grid
{
 public:
  Grid(odb::Orientation2D orientation);

  Grid(odb::Orientation2D orientation, int tracks);

  Grid(odb::Orientation2D orientation, odb::Point origin);

  void addLayout(std::unique_ptr<Layout> layout);

  bool insertLayout(std::unique_ptr<Layout> layout, int index);

  void addCell(std::unique_ptr<Cell> cell, int track);

  void gridInit();

  void placeGrid();

  void setOrigin(odb::Point position);

  int getHeight() const;

  int getWidth() const;

  int numLayouts() const;

  int getLayoutWidth(int index) const;

  int getLayoutHeight(int index) const;

  int getRowWidth() const;

 private:
  odb::Orientation2D orientation_;
  odb::Point origin_;
  int cell_height_{0};
  int cell_width_{0};
  std::vector<std::unique_ptr<Layout>> layouts_;
};

}  // namespace ram
