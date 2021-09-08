///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// This is a geometric search structure.  It wraps up Boost's
// rtree.  OpenDB also has some code for this purpose but I
// find it confusing so just made a simpler solution for now.
//
// Currently this class is static once built and doesn't follow
// db changes.  TODO: this should be into an observer of OpenDB.
class Search
{
  template <typename T>
  class MinSizePredicate;

  template <typename T>
  class MinHeightPredicate;

 public:
  using Point = bg::model::d2::point_xy<int, bg::cs::cartesian>;
  using Box = bg::model::box<Point>;
  using Polygon
      = bg::model::polygon<Point, false>;  // counterclockwise(clockwise=false)
  template <typename T>
  using Value = std::tuple<Box, Polygon, T>;

  template <typename T>
  using Rtree = bgi::rtree<Value<T>, bgi::quadratic<16>>;

  // This is an iterator range for return values
  template <typename T>
  class Range
  {
   public:
    using Iterator = typename Rtree<T>::const_query_iterator;

    Range() = default;
    Range(const Iterator& begin, const Iterator& end) : begin_(begin), end_(end)
    {
    }

    Iterator begin() { return begin_; }
    Iterator end() { return end_; }

   private:
    Iterator begin_;
    Iterator end_;
  };
  using InstRange = Range<odb::dbInst*>;
  using ShapeRange = Range<odb::dbNet*>;
  using FillRange = Range<odb::dbFill*>;
  using ObstructionRange = Range<odb::dbObstruction*>;
  using BlockageRange = Range<odb::dbBlockage*>;

  // Build the structure for the given block.
  void init(odb::dbBlock* block);

  // Find all shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  ShapeRange searchShapes(odb::dbTechLayer* layer,
                          int x_lo,
                          int y_lo,
                          int x_hi,
                          int y_hi,
                          int min_size = 0);

  // Find all fills in the given bounds on the given layer which
  // are at least min_size in either dimension.
  FillRange searchFills(odb::dbTechLayer* layer,
                        int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_size = 0);

  // Find all instances in the given bounds with height of at least min_height
  InstRange searchInsts(int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_height = 0);

  // Find all blockages in the given bounds with height of at least min_height
  BlockageRange searchBlockages(int x_lo,
                                int y_lo,
                                int x_hi,
                                int y_hi,
                                int min_height = 0);

  // Find all obstructions in the given bounds on the given layer which
  // are at least min_size in either dimension.
  ObstructionRange searchObstructions(odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size = 0);

  void clear();

 private:
  void addSNet(odb::dbNet* net);
  void addNet(odb::dbNet* net);
  void addVia(odb::dbNet* net, odb::dbShape* shape, int x, int y);
  void addInst(odb::dbInst* inst);
  void addBlockage(odb::dbBlockage* blockage);
  void addObstruction(odb::dbObstruction* obstruction);

  // The net is used for filter shapes by net type
  std::map<odb::dbTechLayer*, Rtree<odb::dbNet*>> shapes_;
  std::map<odb::dbTechLayer*, Rtree<odb::dbFill*>> fills_;
  Rtree<odb::dbInst*> insts_;
  Rtree<odb::dbBlockage*> blockages_;
  std::map<odb::dbTechLayer*, Rtree<odb::dbObstruction*>> obstructions_;
};

}  // namespace gui
