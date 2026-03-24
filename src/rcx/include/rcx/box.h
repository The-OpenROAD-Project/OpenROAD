// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "odb/geom.h"
#include "odb/util.h"

namespace rcx {

class Box
{
 public:
  Box();
  Box(int x1, int y1, int x2, int y2, int units = 1);

  uint32_t getDir() const;
  int getYhi(int bound) const;
  int getXhi(int bound) const;
  int getXlo(int bound) const;
  int getYlo(int bound) const;
  uint32_t getWidth(uint32_t* dir) const;
  uint32_t getDX() const;
  uint32_t getDY() const;
  uint32_t getLength() const;
  uint32_t getOwner() const;
  uint32_t getLayer() const { return _layer; }
  uint32_t getId() const { return _id; }
  odb::Rect getRect() const { return _rect; }
  void setRect(const odb::Rect& rect) { _rect = rect; }

  void invalidateBox();
  void set(Box* bb);
  void set(int x1, int y1, int x2, int y2, int units = 1);
  void setLayer(uint32_t layer) { _layer = layer; }

 private:
  uint32_t _layer : 4;
  uint32_t _valid : 1;
  uint32_t _id : 27;

  odb::Rect _rect;
};

}  // namespace rcx
