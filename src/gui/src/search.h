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

#include <QObject>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <mutex>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/geom.h"

namespace gui {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// This is a geometric search structure.  It wraps up Boost's
// rtree.  OpenDB also has some code for this purpose but I
// find it confusing so just made a simpler solution for now.
//
// Currently this class is static once built and doesn't follow
// db changes.  TODO: this should be into an observer of OpenDB.
class Search : public QObject, public odb::dbBlockCallBackObj
{
  Q_OBJECT

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
  using BoxValue = std::tuple<Box, T>;
  template <typename T>
  using SBoxValue = std::tuple<Box, odb::dbSBox*, T>;
  template <typename T>
  using PolygonValue = std::tuple<Box, Polygon, T>;

  template <typename T>
  using RtreeBox = bgi::rtree<BoxValue<T>, bgi::quadratic<16>>;
  template <typename T>
  using RtreeSBox = bgi::rtree<SBoxValue<T>, bgi::quadratic<16>>;
  template <typename T>
  using RtreePolygon = bgi::rtree<PolygonValue<T>, bgi::quadratic<16>>;

  // This is an iterator range for return values
  template <typename Tree>
  class Range
  {
   public:
    using Iterator = typename Tree::const_query_iterator;

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
  using InstRange = Range<RtreeBox<odb::dbInst*>>;
  using BoxRange = Range<RtreeBox<odb::dbNet*>>;
  using SBoxRange = Range<RtreeSBox<odb::dbNet*>>;
  using PolygonRange = Range<RtreePolygon<odb::dbNet*>>;
  using FillRange = Range<RtreeBox<odb::dbFill*>>;
  using ObstructionRange = Range<RtreeBox<odb::dbObstruction*>>;
  using BlockageRange = Range<RtreeBox<odb::dbBlockage*>>;
  using RowRange = Range<RtreeBox<odb::dbRow*>>;

  ~Search();

  // Build the structure for the given block.
  void setTopBlock(odb::dbBlock* block);

  // Find all box shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  BoxRange searchBoxShapes(odb::dbBlock* block,
                           odb::dbTechLayer* layer,
                           int x_lo,
                           int y_lo,
                           int x_hi,
                           int y_hi,
                           int min_size = 0);

  // Find all via sbox shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  SBoxRange searchViaSBoxShapes(odb::dbBlock* block,
                                odb::dbTechLayer* layer,
                                int x_lo,
                                int y_lo,
                                int x_hi,
                                int y_hi,
                                int min_size = 0);

  // Find all polgyon shapes in the given bounds on the given layer which
  // are at least min_size in either dimension.
  PolygonRange searchPolygonShapes(odb::dbBlock* block,
                                   odb::dbTechLayer* layer,
                                   int x_lo,
                                   int y_lo,
                                   int x_hi,
                                   int y_hi,
                                   int min_size = 0);

  // Find all fills in the given bounds on the given layer which
  // are at least min_size in either dimension.
  FillRange searchFills(odb::dbBlock* block,
                        odb::dbTechLayer* layer,
                        int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_size = 0);

  // Find all instances in the given bounds with height of at least min_height
  InstRange searchInsts(odb::dbBlock* block,
                        int x_lo,
                        int y_lo,
                        int x_hi,
                        int y_hi,
                        int min_height = 0);

  // Find all blockages in the given bounds with height of at least min_height
  BlockageRange searchBlockages(odb::dbBlock* block,
                                int x_lo,
                                int y_lo,
                                int x_hi,
                                int y_hi,
                                int min_height = 0);

  // Find all obstructions in the given bounds on the given layer which
  // are at least min_size in either dimension.
  ObstructionRange searchObstructions(odb::dbBlock* block,
                                      odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size = 0);

  // Find all rows in the given bounds with height of at least min_height.
  RowRange searchRows(odb::dbBlock* block,
                      int x_lo,
                      int y_lo,
                      int x_hi,
                      int y_hi,
                      int min_height = 0);

  void clearShapes();
  void clearFills();
  void clearInsts();
  void clearBlockages();
  void clearObstructions();
  void clearRows();

  // From dbBlockCallBackObj
  virtual void inDbNetDestroy(odb::dbNet* net) override;
  virtual void inDbInstDestroy(odb::dbInst* inst) override;
  virtual void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  virtual void inDbInstPlacementStatusBefore(
      odb::dbInst* inst,
      const odb::dbPlacementStatus& status) override;
  virtual void inDbPostMoveInst(odb::dbInst* inst) override;
  virtual void inDbBPinDestroy(odb::dbBPin* pin) override;
  virtual void inDbFillCreate(odb::dbFill* fill) override;
  virtual void inDbWireCreate(odb::dbWire* wire) override;
  virtual void inDbWireDestroy(odb::dbWire* wire) override;
  virtual void inDbSWireCreate(odb::dbSWire* wire) override;
  virtual void inDbSWireDestroy(odb::dbSWire* wire) override;
  virtual void inDbSWireAddSBox(odb::dbSBox* box) override;
  virtual void inDbSWireRemoveSBox(odb::dbSBox* box) override;
  virtual void inDbBlockSetDieArea(odb::dbBlock* block) override;
  virtual void inDbBlockageCreate(odb::dbBlockage* blockage) override;
  virtual void inDbObstructionCreate(odb::dbObstruction* obs) override;
  virtual void inDbObstructionDestroy(odb::dbObstruction* obs) override;
  virtual void inDbRegionAddBox(odb::dbRegion*, odb::dbBox*) override;
  virtual void inDbRegionDestroy(odb::dbRegion* region) override;
  virtual void inDbRowCreate(odb::dbRow* row) override;
  virtual void inDbRowDestroy(odb::dbRow* row) override;
  virtual void inDbWirePostModify(odb::dbWire* wire) override;

 signals:
  void modified();
  void newBlock(odb::dbBlock* block);

 private:
  struct BlockData;

  void addSNet(odb::dbNet* net);
  void addNet(odb::dbNet* net);
  void addVia(odb::dbNet* net, odb::dbShape* shape, int x, int y);
  void addInst(odb::dbInst* inst);
  void addBlockage(odb::dbBlockage* blockage);
  void addObstruction(odb::dbObstruction* obstruction);
  void addRow(odb::dbRow* row);

  void updateShapes(odb::dbBlock* block);
  void updateFills(odb::dbBlock* block);
  void updateInsts(odb::dbBlock* block);
  void updateBlockages(odb::dbBlock* block);
  void updateObstructions(odb::dbBlock* block);
  void updateRows(odb::dbBlock* block);

  Box convertRect(const odb::Rect& box) const;

  void clear();

  void announceModified(std::atomic_bool& flag);
  BlockData& getData(odb::dbBlock* block);

  odb::dbBlock* top_block_{nullptr};

  struct BlockData
  {
    // The net is used for filter shapes by net type
    std::map<odb::dbTechLayer*, RtreeBox<odb::dbNet*>> box_shapes_;
    // Special net vias may be large multi-cut vias.  It is more efficient
    // to store the dbSBox (ie the via) than all the cuts.  This is
    // particularly true when you have parallel straps like m1 & m2 in asap7.
    std::map<odb::dbTechLayer*, RtreeSBox<odb::dbNet*>> via_sbox_shapes_;
    std::map<odb::dbTechLayer*, RtreePolygon<odb::dbNet*>> polygon_shapes_;
    std::atomic_bool shapes_init_{false};
    std::mutex shapes_init_mutex_;
    std::map<odb::dbTechLayer*, RtreeBox<odb::dbFill*>> fills_;
    std::atomic_bool fills_init_{false};
    std::mutex fills_init_mutex_;
    RtreeBox<odb::dbInst*> insts_;
    std::atomic_bool insts_init_{false};
    std::mutex insts_init_mutex_;
    RtreeBox<odb::dbBlockage*> blockages_;
    std::atomic_bool blockages_init_{false};
    std::mutex blockages_init_mutex_;
    std::map<odb::dbTechLayer*, RtreeBox<odb::dbObstruction*>> obstructions_;
    std::atomic_bool obstructions_init_{false};
    std::mutex obstructions_init_mutex_;
    RtreeBox<odb::dbRow*> rows_;
    std::atomic_bool rows_init_{false};
    std::mutex rows_init_mutex_;
  };
  std::map<odb::dbBlock*, BlockData> child_block_data_;
  BlockData top_block_data_;
};

}  // namespace gui
