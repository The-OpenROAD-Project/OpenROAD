// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <deque>

#include "odb/geom.h"

namespace odb {

// This stores shapes by level through addShape.  Once all the shapes
// have been added then searchStart/Next can be used for querying.
// Internally a simple tree of space bisections is generated for
// efficiency.
//
// The code uses an odd convention:
// is_via = 0 ==> wire
//        = 1 ==> via
//        = 2 ==> pin
class ShapeSearch
{
 public:
  struct Shape
  {
    int xMin() const { return bounds.xMin(); }
    int yMin() const { return bounds.yMin(); }
    int xMax() const { return bounds.xMax(); }
    int yMax() const { return bounds.yMax(); }

    Shape* next = nullptr;
    Rect bounds;
    int level = 0;
    int is_via = 0;
    int id = 0;
  };

  struct Bin
  {
    int xMin() const { return bounds.xMin(); }
    int yMin() const { return bounds.yMin(); }
    int xMax() const { return bounds.xMax(); }
    int yMax() const { return bounds.yMax(); }
    void reset();
    void add_shape(Shape* shape, bool update_bounds = true);

    Shape* shape_list = nullptr;
    Shape* last_shape = nullptr;
    Bin* left = nullptr;
    Bin* right = nullptr;
    Bin* parent = nullptr;
    Rect bounds;
    int num_shapes = 0;
  };

  ShapeSearch();

  void clear();
  void addShape(int level, const Rect& bounds, int is_via, int id);
  void searchStart(int level, const Rect& bounds, int is_via);
  bool searchNext(int* id);

 private:
  void sort();
  void sort_level(Bin* bin);

  // Use deque so that emplace_back doesn't move prior elements so pointer
  // into these structures are safe.
  std::deque<Shape> shapes_;
  std::deque<Bin> bins_;
  std::array<Bin*, 32> root_for_level_;

  // Used during searching
  Rect search_box_;
  int search_via_{0};
  Bin* search_bin_{nullptr};
  Shape* search_shape_{nullptr};

  // Sorting happens after all the shapes have been added and the
  // first searchStart happens
  bool sorted_{false};

  static constexpr int kSortThreshold = 1024;
};

}  // namespace odb