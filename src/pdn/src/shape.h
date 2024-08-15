///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class Shape;
class Via;

using ShapePtr = std::shared_ptr<Shape>;
using ViaPtr = std::shared_ptr<Via>;

using ShapeVectorMap = std::map<odb::dbTechLayer*, std::vector<ShapePtr>>;

class Grid;
class GridComponent;
class VoltageDomain;

// Basic class that contains a single shape for the power grid, keeping track of
// via connections, iterm connections, and bterm connections.
class Shape
{
 public:
  struct ObstructionHalo
  {
    int left;
    int top;
    int right;
    int bottom;
  };
  enum ShapeType
  {
    SHAPE,
    GRID_OBS,
    BLOCK_OBS,
    MACRO_OBS,
    OBS,
    FIXED
  };
  struct RectIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(const ShapePtr& t) const { return t->getRect(); }
  };
  struct ObstructionIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(const ShapePtr& t) const
    {
      return t->getObstruction();
    }
  };

  using ShapeTree
      = bgi::rtree<ShapePtr, bgi::quadratic<16>, Shape::RectIndexableGetter>;
  using ObstructionTree = bgi::
      rtree<ShapePtr, bgi::quadratic<16>, Shape::ObstructionIndexableGetter>;

  using ShapeTreeMap = std::map<odb::dbTechLayer*, ShapeTree>;
  using ObstructionTreeMap = std::map<odb::dbTechLayer*, ObstructionTree>;

  Shape(odb::dbTechLayer* layer,
        odb::dbNet* net,
        const odb::Rect& rect,
        const odb::dbWireShapeType& type = odb::dbWireShapeType::NONE);
  Shape(odb::dbTechLayer* layer, const odb::Rect& rect, ShapeType shape_type);
  virtual ~Shape();

  odb::dbTechLayer* getLayer() const { return layer_; }
  odb::dbNet* getNet() const { return net_; }
  void setNet(odb::dbNet* net) { net_ = net; }
  void setRect(const odb::Rect& rect) { rect_ = rect; }
  const odb::Rect& getRect() const { return rect_; }
  odb::dbWireShapeType getType() const { return type_; }

  utl::Logger* getLogger() const;

  ShapeType shapeType() const { return shape_type_; }
  void setShapeType(ShapeType type) { shape_type_ = type; }

  void setLocked() { is_locked_ = true; }
  void clearLocked() { is_locked_ = false; }
  bool isLocked() const { return is_locked_; }

  int getLength() const { return rect_.maxDXDY(); }
  int getWidth() const { return rect_.minDXDY(); }
  // true if the shape is aligned against the preferred direction of the layer
  bool isWrongWay() const;

  // check if shape is valid for the given layer
  bool isValid() const;

  const odb::Rect& getObstruction() const { return obs_; }
  // generates the obstruction box needed to avoid DRC violations with
  // surrounding shapes
  void generateObstruction();
  void setObstruction(const odb::Rect& rect) { obs_ = rect; }
  ObstructionHalo getObstructionHalo() const;
  odb::Rect getRectWithLargestObstructionHalo(
      const ObstructionHalo& halo) const;

  bool isHorizontal() const { return rect_.dx() > rect_.dy(); }
  bool isSquare() const { return rect_.dx() == rect_.dy(); }
  bool isVertical() const { return rect_.dx() < rect_.dy(); }

  // true if shape can be removed by trimming
  virtual bool isRemovable() const;
  // true if shape can be modified (cut or shortened) by trimming
  virtual bool isModifiable() const;

  void clearVias() { vias_.clear(); }
  void addVia(const ViaPtr& via) { vias_.push_back(via); }
  const std::vector<ViaPtr>& getVias() const { return vias_; }
  void removeVia(const ViaPtr& via);

  void addITermConnection(const odb::Rect& iterm)
  {
    iterm_connections_.insert(iterm);
  }
  void removeITermConnection(const odb::Rect& iterm)
  {
    iterm_connections_.erase(iterm);
  }
  void clearITermConnections() { iterm_connections_.clear(); }
  const std::set<odb::Rect>& getItermConnections() const
  {
    return iterm_connections_;
  }
  void addBTermConnection(const odb::Rect& bterm)
  {
    bterm_connections_.insert(bterm);
  }
  void removeBTermConnection(const odb::Rect& bterm)
  {
    bterm_connections_.erase(bterm);
  }
  const std::set<odb::Rect>& getBtermConnections() const
  {
    return bterm_connections_;
  }
  // after shape is modified, remove any term connections that are no longer
  // connected
  virtual void updateTermConnections();
  bool hasTermConnections() const;
  bool hasITermConnections() const { return !iterm_connections_.empty(); }
  bool hasBTermConnections() const { return !bterm_connections_.empty(); };

  // returns the smallest shape possible when attempting to trim
  virtual odb::Rect getMinimumRect() const;
  int getNumberOfConnections() const;
  int getNumberOfConnectionsBelow() const;
  int getNumberOfConnectionsAbove() const;

  Shape* extendTo(
      const odb::Rect& rect,
      const ObstructionTree& obstructions,
      const std::function<bool(const ShapePtr&)>& obs_filter
      = [](const ShapePtr&) { return true; }) const;

  virtual bool cut(const ObstructionTree& obstructions,
                   const Grid* ignore_grid,
                   std::vector<Shape*>& replacements) const;

  // return a copy of the shape
  virtual Shape* copy() const;
  // merge this shape with another
  virtual void merge(Shape* shape);

  void setGridComponent(GridComponent* component)
  {
    grid_component_ = component;
  }
  GridComponent* getGridComponent() const { return grid_component_; }

  // returns the text used by the renderer to identify the shape
  std::string getDisplayText() const;

  std::string getReportText() const;
  static std::string getRectText(const odb::Rect& rect, double dbu_to_micron);

  void writeToDb(odb::dbSWire* swire,
                 bool add_pins,
                 bool make_rect_as_pin) const;
  // copy existing shapes into the map
  static void populateMapFromDb(odb::dbNet* net, ShapeVectorMap& map);

  bool allowsNonPreferredDirectionChange() const
  {
    return allow_non_preferred_change_;
  }
  virtual void setAllowsNonPreferredDirectionChange()
  {
    allow_non_preferred_change_ = true;
  }

  static ShapeTreeMap convertVectorToTree(ShapeVectorMap& vec);
  static ObstructionTreeMap convertVectorToObstructionTree(ShapeVectorMap& vec);

 protected:
  bool cut(const ObstructionTree& obstructions,
           std::vector<Shape*>& replacements,
           const std::function<bool(const ShapePtr&)>& obs_filter) const;

 private:
  odb::dbTechLayer* layer_;
  odb::dbNet* net_;
  odb::Rect rect_;
  odb::dbWireShapeType type_;
  ShapeType shape_type_;
  bool allow_non_preferred_change_;
  bool is_locked_;

  odb::Rect obs_;

  GridComponent* grid_component_;

  std::vector<ViaPtr> vias_;
  std::set<odb::Rect> iterm_connections_;
  std::set<odb::Rect> bterm_connections_;

  // add rect as bterm to database
  void addBPinToDb(const odb::Rect& rect) const;

  void updateIBTermConnections(std::set<odb::Rect>& terms);

  bool hasDBConnectivity() const;
};

class FollowPinShape : public Shape
{
 public:
  FollowPinShape(odb::dbTechLayer* layer,
                 odb::dbNet* net,
                 const odb::Rect& rect);

  void addRow(odb::dbRow* row) { rows_.insert(row); }

  odb::Rect getMinimumRect() const override;
  Shape* copy() const override;
  void merge(Shape* shape) override;
  void updateTermConnections() override;

  // followpins cannot be removed
  bool isRemovable() const override { return false; }

  void setAllowsNonPreferredDirectionChange() override {}

  bool cut(const ObstructionTree& obstructions,
           const Grid* ignore_grid,
           std::vector<Shape*>& replacements) const override;

 private:
  std::set<odb::dbRow*> rows_;
};

class GridObsShape : public Shape
{
 public:
  GridObsShape(odb::dbTechLayer* layer,
               const odb::Rect& rect,
               const Grid* grid);
  ~GridObsShape() override = default;

  bool belongsTo(const Grid* grid) const { return grid == grid_; }
  const Grid* getGrid() const { return grid_; }

 private:
  const Grid* grid_;
};

}  // namespace pdn
