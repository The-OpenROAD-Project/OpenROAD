///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "db.h"

namespace gui {

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

// This is a geometric search structure.  It wraps up Boost's
// rtree.  OpenDB also has some code for this purpose but I
// find it confusing so just made a simpler solution for now.
//
// Currently this class is static once built and doesn't follow
// db changes.  TODO: this should be into an observer of OpenDB.
class Search
{
  using point_t = bg::model::d2::point_xy<int, bg::cs::cartesian>;
  using box_t   = bg::model::box<point_t>;

  template <typename T>
  using value_t = std::pair<box_t, T>;

  template <typename T>
  using rtree = bgi::rtree<value_t<T>, bgi::quadratic<16>>;

  template <typename T>
  class MinSizePredicate;

  template <typename T>
  class MinHeightPredicate;

 public:
  // This is an iterator range for return values
  template <typename T>
  class Range
  {
   public:
    using iterator = typename rtree<T>::const_query_iterator;

    Range() = default;
    Range(const iterator& begin, const iterator& end) : begin_(begin), end_(end)
    {
    }

    iterator begin() { return begin_; }
    iterator end() { return end_; }

   private:
    iterator begin_;
    iterator end_;
  };
  using InstRange  = Range<odb::dbInst*>;
  using ShapeRange = Range<int>;

  // Build the structure for the given block.
  void init(odb::dbBlock* block);

  // Find all shapes in the given bounds on the given layer which
  // are at least minSize in either dimension.
  ShapeRange search_shapes(odb::dbTechLayer* layer,
                           int               xLo,
                           int               yLo,
                           int               xHi,
                           int               yHi,
                           int               minSize = 0);

  // Find all instances in the given bounds with height of at least minHeight
  InstRange search_insts(int xLo, int yLo, int xHi, int yHi, int minHeight = 0);

 private:
  void addSNet(odb::dbNet* net);
  void addNet(odb::dbNet* net);
  void addVia(odb::dbShape* shape, int shapeId, int x, int y);
  void addInst(odb::dbInst* inst);

  // The int stored in shapes is the "shapeId" from OpenDB.  Not
  // being used yet but will be good for selection later.
  std::map<odb::dbTechLayer*, rtree<int>> shapes_;
  rtree<odb::dbInst*>                     insts_;
};

}  // namespace gui
