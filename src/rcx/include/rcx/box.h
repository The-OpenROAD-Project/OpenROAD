// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstring>

#include "odb/geom.h"
#include "odb/util.h"

namespace rcx {

using uint = unsigned int;

class Box
{
 public:
  Box();
  Box(int x1, int y1, int x2, int y2, int units = 1);

  uint getDir() const;
  int getYhi(int bound) const;
  int getXhi(int bound) const;
  int getXlo(int bound) const;
  int getYlo(int bound) const;
  uint getWidth(uint* dir) const;
  uint getDX() const;
  uint getDY() const;
  uint getLength() const;
  uint getOwner() const;
  uint getLayer() const { return _layer; }
  uint getId() const { return _id; }
  odb::Rect getRect() const { return _rect; }
  void setRect(const odb::Rect& rect) { _rect = rect; }

  void invalidateBox();
  void set(Box* bb);
  void set(int x1, int y1, int x2, int y2, int units = 1);
  void setLayer(uint layer) { _layer = layer; }

 private:
  uint _layer : 4;
  uint _valid : 1;
  uint _id : 27;

  odb::Rect _rect;
};

}  // namespace rcx
