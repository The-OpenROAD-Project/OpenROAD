// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <deque>

#include "odb/geom.h"

namespace odb {

inline constexpr int kMaxRoutingLevels = 32;

// This stores shapes by level through addShape.  Once all the shapes
// have been added then searchStart/Next can be used for querying.
// Internally a simple tree of space bisections is generated for
// efficiency.
class ShapeSearch
{
 public:
  enum class Type
  {
    kWire,
    kVia,
    kPin
  };

  struct Shape : public Rect
  {
    Shape(const Rect& bounds, Type type, int id)
        : Rect(bounds), type(type), id(id)
    {
    }

    const Type type;
    const int id;
    Shape* next{nullptr};
  };

  struct Bin
  {
    void init(Bin* parent, Bin* left = nullptr, Bin* right = nullptr);
    void addShape(Shape* shape, bool update_bounds = true);
    void wrap();

    Shape* first_shape = nullptr;
    Shape* last_shape = nullptr;
    Bin* left = nullptr;
    Bin* right = nullptr;
    Bin* parent = nullptr;
    Rect bounds;
    int num_shapes = 0;
  };

  ShapeSearch();

  void clear();
  void addShape(int level, const Rect& bounds, Type type, int id);
  void searchStart(int level, const Rect& bounds, Type type);
  bool searchNext(int* id);

 private:
  void splitBins();
  void splitBin(Bin* bin);

  // Use deque so that emplace_back doesn't move prior elements so pointer
  // into these structures are safe.
  std::deque<Shape> shapes_;
  std::deque<Bin> bins_;
  std::array<Bin*, kMaxRoutingLevels> root_for_level_;

  // Used during searching
  Rect search_box_;
  Type search_type_{Type::kWire};
  Bin* search_bin_{nullptr};
  Shape* search_shape_{nullptr};

  // Splitting happens after all the shapes have been added and the
  // first searchStart happens
  bool bins_are_split_{false};

  static constexpr int kSortThreshold = 1024;
};

}  // namespace odb
